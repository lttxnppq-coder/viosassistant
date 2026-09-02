"""Tự động tìm cổng COM của ESP32, không phụ thuộc số COM cố định.

Quy trình:
1. Enumerate tất cả COM port (non-verbose; verbose mode bị treo trên Windows).
2. Loại cổng Bluetooth (hwid bắt đầu BTHENUM hoặc description chứa "bluetooth").
3. Chấm điểm ứng viên theo VID/PID candidate + description + manufacturer.
   VID/PID CHỈ dùng để tìm candidate — KHÔNG kết luận "đây chắc chắn là ESP32".
4. Với mỗi candidate: HANDSHAKE — mở COM 115200, chờ banner boot "ESP32 READY"
   (KHÔNG gửi command test, KHÔNG reset nhiều lần).
5. 0 verified -> báo lỗi rõ ràng. 1 verified -> tự chọn.
   Nhiều verified -> hiển thị danh sách, yêu cầu người dùng chọn (không chọn bừa).
6. Cache last_port/last_hwid/last_serial vào config.json nhưng LUÔN validate
   (port còn tồn tại + hwid khớp + SN khớp nếu cả hai có) và HANDSHAKE lại
   trước khi dùng; không khớp -> auto-detect lại.
"""

import json
import os
import re
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

import serial
import serial.tools.list_ports

PROJECT_ROOT = Path(__file__).resolve().parent
CONFIG_FILE = PROJECT_ROOT / "config.json"

BAUDRATE = 115200
HANDSHAKE_TIMEOUT = 5.0
HANDSHAKE_BANNER = "ESP32 READY"

# ---------------------------------------------------------------------------
# KNOWN CANDIDATES - UNVERIFIED
#
# Đây là danh sách chip bridge USB->UART phổ biến đi kèm ESP32 DevKit.
# ĐÂY CHƯA PHẢI hardware identity đã được xác minh của thiết bị cụ thể.
# Để xác nhận VID/PID thực tế, cắm ESP32 và chạy:
#
#   python -c "import serial.tools.list_ports as p; [print(x.device,'|',x.description,'|','VID=%04X PID=%04X'%(x.vid or 0,x.pid or 0),'|',x.hwid,'| SN:',x.serial_number,'| MFR:',x.manufacturer) for x in p.comports()]"
#
# Sau khi xác nhận có thể khóa VID/PID thật vào config.json
# (verified_vid_pid) để tăng độ tin cậy.
# ---------------------------------------------------------------------------
KNOWN_BRIDGE_VID_PID = {
    (0x10C4, 0xEA60): "Silicon Labs CP210x USB to UART Bridge (candidate, UNVERIFIED)",
    (0x1A86, 0x7523): "WCH CH340 USB-Serial (candidate, UNVERIFIED)",
}

# ESP32-S3 native USB (VID:PID = 303A:1001) — LÀ ỨNG VIÊN HỢP LỆ:
# firmware hiện tại bật ARDUINO_USB_CDC_ON_BOOT, Serial chạy qua USB CDC
# native (board chi co 1 cong USB, khong co bridge CP2102/CH340).
# Cung VID/PID cho ca USB-Serial/JTAG (boot) va USB CDC (app), nen bat buoc
# verify bang handshake "ESP32 READY" truoc khi chot.
NATIVE_USB_VID_PID = (0x303A, 0x1001)

_VID_PID_RE = re.compile(r"VID:PID=([0-9A-Fa-f]{4}):([0-9A-Fa-f]{4})")


@dataclass
class PortInfo:
    device: str
    description: str
    hwid: str
    vid: Optional[int]
    pid: Optional[int]
    serial_number: Optional[str]
    manufacturer: Optional[str]
    score: int = 0
    reason: str = ""

    @classmethod
    def from_list_port(cls, p) -> "PortInfo":
        return cls(
            device=p.device,
            description=p.description or "",
            hwid=p.hwid or "",
            vid=p.vid,
            pid=p.pid,
            serial_number=p.serial_number,
            manufacturer=p.manufacturer,
        )

    def is_bluetooth(self) -> bool:
        hw = self.hwid.upper()
        desc = self.description.lower()
        return hw.startswith("BTHENUM") or ("bluetooth" in desc)

    def is_native_usb(self) -> bool:
        return (self.vid, self.pid) == NATIVE_USB_VID_PID


def enumerate_ports() -> list[PortInfo]:
    """Liệt kê các cổng serial hiện có (không dùng verbose - dễ bị treo)."""
    return [PortInfo.from_list_port(p) for p in serial.tools.list_ports.comports()]


def score_port(port: PortInfo) -> tuple[int, str]:
    """Chấm điểm độ khả dĩ là ESP32 (càng cao càng chắc chắn)."""
    if port.is_bluetooth():
        return 0, "cong Bluetooth (loai)"
    if port.is_native_usb():
        return 100, "ESP32-S3 native USB (USB CDC / USB-Serial-JTAG)"

    score = 0
    reasons: list[str] = []
    key = (port.vid, port.pid)

    if key in KNOWN_BRIDGE_VID_PID:
        score += 100
        reasons.append(f"VID/PID khớp candidate {KNOWN_BRIDGE_VID_PID[key]}")

    desc = port.description.lower()
    if "cp210" in desc:
        score += 30
        reasons.append("description chua 'CP210x'")
    if "ch340" in desc or "ch341" in desc:
        score += 30
        reasons.append("description chua 'CH340'")
    if port.manufacturer and "silicon labs" in port.manufacturer.lower():
        score += 10
        reasons.append("manufacturer Silicon Labs")

    if score <= 0:
        return 0, "khong phai USB serial bridge da biet (khong doan mo)"

    return score, "; ".join(reasons)


# ---------------------------------------------------------------------------
# Cache (config.json)
# ---------------------------------------------------------------------------

def load_config() -> dict:
    try:
        return json.loads(CONFIG_FILE.read_text(encoding="utf-8"))
    except (FileNotFoundError, json.JSONDecodeError):
        return {}


def save_cache(port: PortInfo) -> None:
    cfg = load_config()
    cfg["last_port"] = port.device
    cfg["last_hwid"] = port.hwid
    cfg["last_serial"] = port.serial_number
    CONFIG_FILE.write_text(
        json.dumps(cfg, indent=2, ensure_ascii=False), encoding="utf-8"
    )


def _hwids_match(a: str, b: str) -> bool:
    """So khớp phần VID:PID trong hwid (không tin số COM, chỉ tin identity)."""
    a_up, b_up = a.strip().upper(), b.strip().upper()
    if not a_up or not b_up:
        return False
    ma, mb = _VID_PID_RE.search(a_up), _VID_PID_RE.search(b_up)
    if ma and mb:
        return ma.group(0) == mb.group(0)
    return a_up == b_up


def _find_port_by_device(device: str) -> Optional[PortInfo]:
    for p in enumerate_ports():
        if p.device.lower() == device.lower():
            return p
    return None


def _serials_match(a: Optional[str], b: Optional[str]) -> bool:
    """So khớp serial number nếu cả hai đều tồn tại; thiếu một bên thì bỏ qua."""
    if not a or not b:
        return True
    return a.strip().upper() == b.strip().upper()


def _cache_is_valid(port: PortInfo, cfg: dict) -> bool:
    expected = cfg.get("last_hwid")
    if not expected or not _hwids_match(port.hwid, expected):
        return False
    return _serials_match(port.serial_number, cfg.get("last_serial"))


# ---------------------------------------------------------------------------
# HANDSHAKE: xác minh COM thực sự là ESP32 firmware của project
# ---------------------------------------------------------------------------

def reset_esp32(ser: serial.Serial, dtr_pulse_ms: float = 100.0) -> None:
    """Reset ESP32-S3 qua DTR True->False transition (de in banner "ESP32 READY").

    Windows: pyserial mo COM assert DTR=True, set dtr=False da tao transition
    True->False -> reset (giu nguyen hanh vi hien tai).
    Linux/POSIX: open ttyACM KHONG assert DTR -> can tu toggle True->False.
    """
    if os.name == "nt":
        return
    try:
        ser.dtr = True
        time.sleep(dtr_pulse_ms / 1000.0)
        ser.dtr = False
    except serial.SerialException:
        pass


def verify_esp32(device: str, timeout: float = HANDSHAKE_TIMEOUT) -> bool:
    """Mở COM 115200 và chờ banner boot "ESP32 READY" (tối đa `timeout` giây).

    KHÔNG gửi bất kỳ command test nào xuống ESP32.
    Mở/đóng COM một lần duy nhất (không reset nhiều lần để kiểm tra).

    Sau khi mở: set dtr=False/rts=False. Evidence thực tế (raw_probe/cmd_test):
    - pyserial mở COM mặc định giữ DTR=True: chip KHÔNG reset, banner READY
      chỉ in 1 lần lúc boot nên handshake không bắt được.
    - Chuyển DTR True->False: ESP32-S3 reset (rst:0x15 USB_UART_CHIP_RESET),
      boot lại, in "ESP32 READY" sau ~1.1s -> bắt được trong cửa sổ đọc.
    """

    try:
        ser = serial.Serial(device, BAUDRATE, timeout=1)
    except serial.SerialException:
        return False
    except ValueError:
        return False

    try:
        try:
            ser.dtr = False
            ser.rts = False
            reset_esp32(ser)
        except serial.SerialException:
            return False
        deadline = time.time() + timeout
        while time.time() < deadline:
            try:
                if ser.in_waiting:
                    line = ser.readline().decode("utf-8", errors="replace")
                    if HANDSHAKE_BANNER in line:
                        return True
            except serial.SerialException:
                return False
            time.sleep(0.01)
        return False
    finally:
        try:
            ser.close()
        except serial.SerialException:
            pass


# ---------------------------------------------------------------------------
# Auto-detection
# ---------------------------------------------------------------------------

def validate_forced_port(device: str) -> str:
    """Kiểm tra port người dùng chỉ định qua --port."""
    port = _find_port_by_device(device)
    if port is None:
        raise SystemExit(
            f"Loi: cong {device} khong ton tai.\n"
            "Chay 'python vosk_test.py --list-ports' de xem cac cong hien co."
        )
    if port.is_bluetooth():
        raise SystemExit(f"Loi: {device} la cong Bluetooth, khong the dung.")
    score, reason = score_port(port)
    if score <= 0:
        print(
            f"Canh bao: {device} ({port.description}) khong nhan dien duoc la ESP32 "
            f"({reason}). Tiep tuc vi ban chi dinh --port."
        )
    return port.device


def find_esp32_port(forced: Optional[str] = None) -> str:
    """Tìm và trả về tên cổng COM của ESP32, XÁC MINH bằng handshake.

    1. forced (--port): dùng nếu hợp lệ; handshake fail -> cảnh báo nhưng
       vẫn tiếp tục (explicit override).
    2. cache config.json: dùng nếu port còn tồn tại + hwid khớp + SN khớp
       (nếu cả hai có) + handshake lại thành công.
    3. auto-detect: verify từng candidate; 0 verified -> báo lỗi rõ ràng;
       1 verified -> tự chọn; nhiều verified -> hỏi người dùng.

    KHÔNG gửi command test trong verify_esp32() (chỉ đọc banner boot).
    """

    if forced:
        port = validate_forced_port(forced)
        if not verify_esp32(port):
            print(
                "ESP32 candidate found but handshake failed: khong nhan duoc "
                "'ESP32 READY'. Tiep tuc vi ban chi dinh --port."
            )
        return port

    # Cache có thể được dùng NHƯNG luôn validate (port tồn tại + identity khớp)
    # và phải HANDSHAKE lại trước khi tin dùng.
    cfg = load_config()
    cached = cfg.get("last_port")
    if cached:
        port = _find_port_by_device(cached)
        if port is not None and _cache_is_valid(port, cfg):
            if verify_esp32(port.device):
                print(f"Dung port da luu truoc: {port.device} ({port.description})")
                return port.device
            print(
                f"Cache khong xac minh duoc handshake tai {cached}; "
                "auto-detect lai."
            )

    print("Searching for ESP32...")

    candidates: list[PortInfo] = []
    for p in enumerate_ports():
        score, reason = score_port(p)
        if score > 0:
            p.score, p.reason = score, reason
            candidates.append(p)

    if not candidates:
        raise SystemExit(
            "Khong tim thay ESP32.\n"
            "  - Kiem tra day USB da cam vao cong USB native cua DevKitC-1.\n"
            "  - Kiem tra firmware da flash (phai co USB CDC / bat cdc_on_boot).\n"
            "  - Chay 'python vosk_test.py --list-ports' de xem chi tiet cac cong.\n"
            "  - Hoac chi dinh tay: python vosk_test.py --port COMx"
        )

    # Xác minh từng candidate bằng handshake (chỉ đọc banner, không gửi lệnh).
    verified: list[PortInfo] = []
    for p in candidates:
        print(f"Dang xac minh {p.device} ({p.description})...")
        if verify_esp32(p.device):
            verified.append(p)

    if not verified:
        raise SystemExit(
            "ESP32 candidate found but handshake failed.\n"
            "  - Kiem tra firmware da flash (phai in 'ESP32 READY' khi boot).\n"
            "  - Kiem tra COM khong bi Serial Monitor / chuong trinh khac chiem.\n"
            "  - Hoac chi dinh tay: python vosk_test.py --port COMx"
        )

    if len(verified) == 1:
        p = verified[0]
        print(f"Verified ESP32:\n{p.device}\n{p.description}")
        save_cache(p)
        return p.device

    print("Found multiple verified ESP32:")
    for i, p in enumerate(verified, 1):
        sn = f" (SN: {p.serial_number})" if p.serial_number else ""
        print(f"[{i}] {p.device}  {p.description}{sn}")

    while True:
        try:
            choice = input(f"Select ESP32 [1-{len(verified)}] (q to quit): ")
        except (EOFError, KeyboardInterrupt):
            raise SystemExit(1)
        choice = choice.strip().lower()
        if choice in ("q", "quit", ""):
            raise SystemExit(1)
        if choice.isdigit():
            idx = int(choice)
            if 1 <= idx <= len(verified):
                p = verified[idx - 1]
                save_cache(p)
                return p.device
        print("Lua chon khong hop le, thu lai.")


# ---------------------------------------------------------------------------
# Debug: --list-ports
# ---------------------------------------------------------------------------

def print_ports_table() -> None:
    """In chi tiết mọi serial port (dùng cho --list-ports)."""
    ports = enumerate_ports()
    if not ports:
        print("No serial ports found.")
        return
    for p in ports:
        print(f"{p.device}")
        print(f"  Description : {p.description or '(none)'}")
        print(f"  Manufacturer: {p.manufacturer or '(none)'}")
        if p.vid:
            print(f"  VID         : {p.vid:04X}")
        else:
            print("  VID         : (none)")
        if p.pid:
            print(f"  PID         : {p.pid:04X}")
        else:
            print("  PID         : (none)")
        print(f"  SN          : {p.serial_number or '(none)'}")
        print(f"  HWID        : {p.hwid or '(none)'}")
        if p.is_bluetooth():
            print("  Type        : BLUETOOTH (loai khoi auto-detection)")
        else:
            score, reason = score_port(p)
            if score > 0:
                print(f"  Type        : candidate ESP32 (score={score}: {reason})")
            else:
                print(f"  Type        : khong nhan dien ({reason})")
        print()


if __name__ == "__main__":
    print_ports_table()
