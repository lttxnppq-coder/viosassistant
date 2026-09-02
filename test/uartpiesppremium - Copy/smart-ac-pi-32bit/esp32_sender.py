"""esp32_sender.py — gui AC command tu Python qua UART toi ESP32 (optional mode).

Reuse esp32_port.find_esp32_port() cho auto COM detection — KHONG duplicate logic.

Mapping (protocol da chot):
    1..11       -> CMD:AC_ON / CMD:AC_OFF / ... / CMD:AIR_DEFROST / CMD:AIR_AUTO
    318..330    -> CMD:SET_TEMP:<18..30>   (uu tien `temperature` hop le, fallback code-300)
    None / code ngoai contract -> khong gui (INVALID_COMMAND)

KHONG BAO GIO gui CMD:318..CMD:330 — firmware chi hieu SET_TEMP semantic.

ESP32 tra (firmware STEP 1):
    RX: CMD:...              (debug — bo qua)
    DECODE: ...              (debug — bo qua)
    RESP:OK:<CMD>            -> success=True
    RESP:ERROR:<reason>      -> success=False

LOW-LATENCY ASYNC (STEP 6):
    - send_async(): chi WRITE UART, KHONG cho ACK. UART_WRITE_OK la moc feedback.
    - start_ack_listener(): daemon thread doc serial doc lap, update status
      (ACK_OK / ACK_ERROR / ACK_TIMEOUT) va promote pending queue.
    - MAX_INFLIGHT = 5: vuot qua -> pending queue (FIFO, MAX_PENDING = 5).
    - Pending day: coalesce duplicate SAFE (mode/last-write-wins), neu khong
      duoc -> DROP NEWEST + log (khong bao gio drop command cu / khong reorder).
    - ACK_TIMEOUT: giam inflight, log, KHONG tu dong retry (TEMP_UP khong idempotent).
    - ACK correlation limitation: protocol khong co command ID; RESP chi xac dinh
      theo command NAME (oldest instance match). Khong doi protocol trong STEP 6.

    WAV feedback (o caller) la legacy wording ("Da chuyen...") va KHONG phai
    protocol confirmation — trang thai protocol rieng: UART_WRITE_OK / ACK_OK /
    ACK_ERROR / ACK_TIMEOUT.

ESP32 khong cam / loi serial / timeout -> tra result loi ro rang, KHONG crash.
"""

import threading
import time
from collections import deque
from dataclasses import dataclass, field
from typing import Callable, Optional

import serial

from esp32_port import find_esp32_port, reset_esp32

BAUDRATE = 115200
READY_BANNER = "ESP32 READY"
READY_TIMEOUT = 3.0

SET_TEMP_BASE = 300
TEMP_MIN = 18
TEMP_MAX = 30

MAX_INFLIGHT = 5   # UART_WRITE_OK nhung chua ACK
MAX_PENDING = 5    # pending queue khi inflight day
ACK_TIMEOUT = 5.0  # giay; qua han -> ACK_TIMEOUT, khong giu inflight vinh vien

# Coalescing chi ap dung cho command "mode/target" (last-write-wins).
# Delta/toggle (AC_ON/OFF, TEMP_UP/DOWN, FAN_ON/OFF) KHONG gop:
# 2x TEMP_UP != 1x TEMP_UP (mat side effect).
COALESCE_SAFE_CODES = frozenset({8, 9, 10, 11})  # AIR_FACE/FOOT/AUTO/DEFROST

UART_COMMAND_MAP = {
    1: "AC_ON",
    2: "AC_OFF",
    4: "TEMP_UP",
    5: "TEMP_DOWN",
    6: "FAN_ON",
    7: "FAN_OFF",
    8: "AIR_FACE",
    9: "AIR_FOOT",
    10: "AIR_DEFROST",
    11: "AIR_AUTO",
}


@dataclass
class SendResult:
    """Ket qua gui command toi ESP32.

    success=True  chi khi command duoc CHAP NHAN de gui:
                  state SENT/QUEUED/COALESCED (= UART_WRITE_OK hoac da vao hang).
                  KHONG dong nghia ESP32 da xac nhan.
    connected     trang thai ket noi hien tai (False neu ESP32 khong cam / loi serial).
    response      dong RESP: day du nhan duoc (chi trong path sync send_command).
    error         ma loi: ESP32_NOT_FOUND / NO_RESPONSE / SERIAL_ERROR /
                  INVALID_COMMAND / QUEUE_FULL.
    state         lifecycle: SENT / QUEUED / COALESCED / DROPPED /
                  SEND_FAILED / NOT_CONNECTED / INVALID / "" (legacy path).
    """

    success: bool
    connected: bool
    response: str = ""
    error: str = ""
    state: str = ""


def command_to_uart(command_code, temperature=None) -> Optional[str]:
    """Chuyen command code Python sang UART command (protocol da chot).

    - 1..11 -> "CMD:<NAME>"
    - 318..330 -> "CMD:SET_TEMP:<n>" (uu tien `temperature` neu hop le, khong thi code-300)
    - None / code ngoai contract -> None (KHONG gui)
    """
    if command_code is None:
        return None

    name = UART_COMMAND_MAP.get(command_code)
    if name:
        return "CMD:" + name

    if SET_TEMP_BASE + TEMP_MIN <= command_code <= SET_TEMP_BASE + TEMP_MAX:
        if temperature is not None and TEMP_MIN <= temperature <= TEMP_MAX:
            temp = temperature
        else:
            temp = command_code - SET_TEMP_BASE
        return f"CMD:SET_TEMP:{temp}"

    return None


class Esp32Sender:
    """Gui AC command qua UART toi ESP32 — optional mode (khong crash khi khong co ESP32).

    Dependency injection (port_finder / serial_factory) de test hoan toan khong can ESP32 that.
    """

    def __init__(
        self,
        port_finder: Callable = find_esp32_port,
        serial_factory: Callable = serial.Serial,
        baudrate: int = BAUDRATE,
        resp_timeout: float = 5.0,
        retries: int = 1,
        max_inflight: int = MAX_INFLIGHT,
        max_pending: int = MAX_PENDING,
        ack_timeout: float = ACK_TIMEOUT,
    ):
        self._port_finder = port_finder
        self._serial_factory = serial_factory
        self._baudrate = baudrate
        self._resp_timeout = resp_timeout
        self._retries = retries
        self._max_inflight = max_inflight
        self._max_pending = max_pending
        self._ack_timeout = ack_timeout
        self._ser = None
        self._port = None
        self._error = ""
        self.connected = False

        # ---- ASYNC STATE (STEP 6): owner = sender, lock = _lock ----
        self._lock = threading.RLock()
        self._inflight: dict[int, dict] = {}      # instance_id -> info (SENT)
        self._pending: deque = deque()            # FIFO {cmd, code, temp}
        self._inst_seq = 0
        self._stop = threading.Event()
        self._listener = None
        self._last_ack = ""
        self._timeout_log: list[str] = []

    # ------------------------------------------------------------------
    # Connection
    # ------------------------------------------------------------------

    def connect(self) -> bool:
        """Auto-detect COM (find_esp32_port) + mo serial + cho boot banner.

        ESP32-S3 reset khi DTR transition (da audit), nen sau khi mo COM phai
        cho "ESP32 READY" truoc khi gui command dau tien.
        Tra False (khong crash) khi: khong tim thay ESP32, loi serial, handshake timeout.
        """
        if self.connected and self._ser is not None:
            return True

        try:
            port = self._port_finder()
        except SystemExit:
            self._error = "ESP32_NOT_FOUND"
            return False

        try:
            ser = self._serial_factory(port, self._baudrate, timeout=1)
        except serial.SerialException:
            self._error = "SERIAL_ERROR"
            return False
        except ValueError:
            self._error = "SERIAL_ERROR"
            return False

        try:
            ser.dtr = False
            ser.rts = False
            reset_esp32(ser)
        except serial.SerialException:
            ser.close()
            self._error = "SERIAL_ERROR"
            return False

        if not self._wait_for_banner(ser):
            try:
                ser.close()
            except serial.SerialException:
                pass
            self._error = "SERIAL_ERROR"
            return False

        self._ser = ser
        self._port = port
        self._error = ""
        self.connected = True
        return True

    def _wait_for_banner(self, ser) -> bool:
        deadline = time.time() + READY_TIMEOUT
        while time.time() < deadline:
            try:
                if ser.in_waiting:
                    line = ser.readline().decode("utf-8", errors="replace")
                    if READY_BANNER in line:
                        return True
            except serial.SerialException:
                return False
            time.sleep(0.01)
        return False

    # ------------------------------------------------------------------
    # ASYNC ACK (STEP 6): reader thread doc doc lap, KHONG block caller
    # ------------------------------------------------------------------

    def start_ack_listener(self) -> None:
        """Bat daemon thread doc RESP tu ESP32 (idempotent).

        Reader KHONG block: microphone / Vosk / classifier / audio playback.
        Writer (send_async) KHONG cho reader truoc khi tra feedback.
        """
        if self._listener is not None and self._listener.is_alive():
            return
        self._stop.clear()
        self._listener = threading.Thread(
            target=self._ack_listener_loop, name="esp32-ack-listener", daemon=True
        )
        self._listener.start()

    def _ack_listener_loop(self) -> None:
        while not self._stop.is_set():
            ser = self._ser
            if ser is not None:
                try:
                    if ser.in_waiting:
                        line = ser.readline().decode("utf-8", errors="replace").strip()
                        if line.startswith("RESP:"):
                            self._handle_resp(line)
                except serial.SerialException:
                    print("[UART] Serial error trong ACK listener — drop connection.")
                    self._drop_connection()
            self._sweep_timeouts()
            time.sleep(0.01)

    def _handle_resp(self, line: str) -> None:
        with self._lock:
            if line.startswith("RESP:OK:"):
                name = line[len("RESP:OK:"):]
                inst_id = self._find_inflight_oldest("CMD:" + name)
                if inst_id is not None:
                    info = self._inflight.pop(inst_id)
                    latency_ms = (time.monotonic() - info["sent_ts"]) * 1000
                    self._last_ack = line
                    print(f"[ACK_OK] {line} (latency={latency_ms:.0f}ms)")
                    self._promote_locked()
            else:
                # RESP:ERROR:<reason> khong gan ten command cu the -> ACK_ERROR
                # cho instance in-flight OLD NHAT (correlation limitation).
                if self._inflight:
                    oldest = min(self._inflight)
                    info = self._inflight.pop(oldest)
                    info["status"] = "ACK_ERROR"
                    print(f"[ACK_ERROR] {line} ({info['cmd']})")
                    self._promote_locked()

    def _find_inflight_oldest(self, cmd: str) -> Optional[int]:
        for inst_id, info in self._inflight.items():
            if info["cmd"] == cmd:
                return inst_id
        return None

    def _sweep_timeouts(self) -> None:
        """ACK_TIMEOUT: giam inflight, log, promote pending. KHONG retry."""
        with self._lock:
            now = time.monotonic()
            expired = [
                i for i, info in self._inflight.items()
                if now - info["sent_ts"] > self._ack_timeout
            ]
            for inst_id in expired:
                info = self._inflight.pop(inst_id)
                info["status"] = "ACK_TIMEOUT"
                self._timeout_log.append(info["cmd"])
                print(f"[ACK_TIMEOUT] {info['cmd']} (khong retry)")
            if expired:
                self._promote_locked()

    def _promote_locked(self) -> None:
        """Gui command pending tiep theo khi con slot inflight (da giu lock)."""
        while len(self._inflight) < self._max_inflight and self._pending:
            item = self._pending.popleft()
            try:
                self._ser.write((item["cmd"] + "\n").encode("utf-8"))
                self._ser.flush()
            except serial.SerialException:
                print(f"[UART_WRITE_FAIL] {item['cmd']} — serial error, drop connection.")
                self._drop_connection()
                return
            self._inst_seq += 1
            self._inflight[self._inst_seq] = {
                "cmd": item["cmd"], "code": item["code"],
                "temperature": item["temperature"],
                "sent_ts": time.monotonic(), "status": "SENT",
            }
            print(f"[PROMOTED] {item['cmd']}")

    def _is_coalesce_safe(self, command_code) -> bool:
        if command_code in COALESCE_SAFE_CODES:
            return True
        return SET_TEMP_BASE + TEMP_MIN <= command_code <= SET_TEMP_BASE + TEMP_MAX

    def _write_locked(self, cmd: str, command_code, temperature) -> SendResult:
        try:
            self._ser.write((cmd + "\n").encode("utf-8"))
            self._ser.flush()
        except serial.SerialException:
            self._drop_connection()
            return SendResult(success=False, connected=False,
                              error="SERIAL_ERROR", state="SEND_FAILED")
        self._inst_seq += 1
        self._inflight[self._inst_seq] = {
            "cmd": cmd, "code": command_code, "temperature": temperature,
            "sent_ts": time.monotonic(), "status": "SENT",
        }
        return SendResult(success=True, connected=True, error="", state="SENT")

    def _enqueue_locked(self, cmd: str, command_code, temperature) -> SendResult:
        if len(self._pending) >= self._max_pending:
            # 1) Coalesce chi voi TAIL khi cung command VA safe (last-write-wins).
            if self._pending and self._pending[-1]["cmd"] == cmd \
                    and self._is_coalesce_safe(command_code):
                print(f"[QUEUE_FULL] Coalesced duplicate: {cmd}")
                return SendResult(success=True, connected=True, state="COALESCED")
            # 2) KHONG drop command cu: DROP NEWEST + log ro rang.
            print(f"[QUEUE_FULL] Dropped newest command: {cmd}")
            return SendResult(success=False, connected=True,
                              error="QUEUE_FULL", state="DROPPED")
        self._pending.append({"cmd": cmd, "code": command_code, "temperature": temperature})
        print(f"[QUEUED] {cmd}")
        return SendResult(success=True, connected=True, state="QUEUED")

    def send_async(self, command_code, temperature=None) -> SendResult:
        """WRITE ngay (UART_WRITE_OK) hoac vao pending — KHONG cho ESP32 ACK.

        feedback user lay moc UART_WRITE_OK, KHONG phai ESP_ACK.
        """
        cmd = command_to_uart(command_code, temperature)
        if cmd is None:
            return SendResult(success=False, connected=self.connected,
                              error="INVALID_COMMAND", state="INVALID")
        if not self.connected:
            if not self.connect():
                return SendResult(success=False, connected=False,
                                  error=self._error or "ESP32_NOT_FOUND",
                                  state="NOT_CONNECTED")
        with self._lock:
            if len(self._inflight) >= self._max_inflight:
                return self._enqueue_locked(cmd, command_code, temperature)
            return self._write_locked(cmd, command_code, temperature)

    # ------------------------------------------------------------------
    # Send (LEGACY sync path — giu cho backward compat + tests hien tai)
    # ------------------------------------------------------------------

    def send_command(self, command_code, temperature=None) -> SendResult:
        """Gui 1 AC command toi ESP32 va cho RESP (timeout + retry)."""
        cmd = command_to_uart(command_code, temperature)
        if cmd is None:
            return SendResult(success=False, connected=self.connected, error="INVALID_COMMAND")

        if not self.connected:
            if not self.connect():
                return SendResult(success=False, connected=False, error=self._error or "ESP32_NOT_FOUND")

        ser = self._ser
        if ser is None:
            return SendResult(success=False, connected=False, error="ESP32_NOT_FOUND")

        for _ in range(self._retries + 1):
            try:
                ser.write((cmd + "\n").encode("utf-8"))
                ser.flush()
            except serial.SerialException:
                self._drop_connection()
                return SendResult(success=False, connected=False, error="SERIAL_ERROR")

            try:
                resp = self._read_resp(ser)
            except serial.SerialException:
                self._drop_connection()
                return SendResult(success=False, connected=False, error="SERIAL_ERROR")

            if resp is not None:
                if resp.startswith("RESP:OK:"):
                    return SendResult(success=True, connected=True, response=resp)
                return SendResult(success=False, connected=True, response=resp)

        return SendResult(success=False, connected=self.connected, error="NO_RESPONSE")

    def _read_resp(self, ser) -> Optional[str]:
        """Doc tung dong, BO QUA dong debug (RX:/DECODE:/ESP32 READY), chi tra dong RESP:."""
        deadline = time.time() + self._resp_timeout
        while time.time() < deadline:
            if ser.in_waiting:
                line = ser.readline().decode("utf-8", errors="replace").strip()
                if line.startswith("RESP:"):
                    return line
            else:
                time.sleep(0.01)
        return None

    # ------------------------------------------------------------------
    # Close / internal
    # ------------------------------------------------------------------

    def _drop_connection(self) -> None:
        with self._lock:
            if self._ser is not None:
                try:
                    self._ser.close()
                except serial.SerialException:
                    pass
            self._ser = None
            self._port = None
            self.connected = False
            self._inflight.clear()
            self._pending.clear()

    def close(self) -> None:
        """Dong serial an toan (dung ACK listener truoc)."""
        self._stop.set()
        if self._listener is not None and self._listener.is_alive():
            self._listener.join(timeout=2.0)
        self._drop_connection()
