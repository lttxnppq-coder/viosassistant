"""Test esp32_sender.py bang FakeSerial / mock dependency injection — KHONG can ESP32 that.

Chay:  python test_esp32_sender.py
"""

import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import serial

import esp32_sender as es


# ---------------------------------------------------------------------------
# FakeSerial: script theo so lan write
#   bucket[0] = dong co san khi mo COM (banner boot)
#   bucket[N] = dong tra sau write thu N
# ---------------------------------------------------------------------------

class FakeSerial:
    def __init__(self, script=(), port="FAKE", raise_on_write=False,
                 raise_on_readline=False, readline_fail_after=0):
        self.script = [list(b) for b in script]
        self.written = []
        self.writes = 0
        self.port = port
        self.raise_on_write = raise_on_write
        self.raise_on_readline = raise_on_readline
        self.readline_fail_after = readline_fail_after
        self.readline_count = 0
        self.closed = False
        self.dtr = True
        self.rts = True

    @property
    def in_waiting(self):
        return 1 if self._bucket() else 0

    def _bucket(self):
        if self.writes < len(self.script):
            return self.script[self.writes]
        return []

    def readline(self):
        self.readline_count += 1
        if self.raise_on_readline or (
            self.readline_fail_after
            and self.readline_count >= self.readline_fail_after
        ):
            raise serial.SerialException("fake readline fail")
        bucket = self._bucket()
        if bucket:
            return (bucket.pop(0) + "\n").encode("utf-8")
        return b""

    def write(self, data):
        if self.raise_on_write:
            raise serial.SerialException("fake write fail")
        self.written.append(data)
        self.writes += 1

    def flush(self):
        pass

    def close(self):
        self.closed = True


class FakeSerialFactory:
    def __init__(self, fake):
        self.fake = fake
        self.calls = 0

    def __call__(self, port, baudrate, timeout):
        self.calls += 1
        return self.fake


def fake_finder_ok(port="FAKE"):
    return lambda: port


def finder_system_exit():
    raise SystemExit("Khong tim thay ESP32")


def make_sender(fake, retries=1, resp_timeout=5.0, finder=None):
    factory = FakeSerialFactory(fake)
    sender = es.Esp32Sender(
        port_finder=finder or fake_finder_ok(),
        serial_factory=factory,
        resp_timeout=resp_timeout,
        retries=retries,
    )
    return sender, factory


# ---------------------------------------------------------------------------
# Mapping tests (pure)
# ---------------------------------------------------------------------------

def test_mapping_ac_on():
    assert es.command_to_uart(1) == "CMD:AC_ON"


def test_mapping_ac_off():
    assert es.command_to_uart(2) == "CMD:AC_OFF"


def test_mapping_temp_up():
    assert es.command_to_uart(4) == "CMD:TEMP_UP"


def test_mapping_temp_down():
    assert es.command_to_uart(5) == "CMD:TEMP_DOWN"


def test_mapping_fan_on():
    assert es.command_to_uart(6) == "CMD:FAN_ON"


def test_mapping_fan_off():
    assert es.command_to_uart(7) == "CMD:FAN_OFF"


def test_mapping_air_face():
    assert es.command_to_uart(8) == "CMD:AIR_FACE"


def test_mapping_air_foot():
    assert es.command_to_uart(9) == "CMD:AIR_FOOT"


def test_mapping_air_defrost():
    assert es.command_to_uart(10) == "CMD:AIR_DEFROST"


def test_mapping_air_auto():
    assert es.command_to_uart(11) == "CMD:AIR_AUTO"


def test_mapping_318_set_temp_18():
    assert es.command_to_uart(318) == "CMD:SET_TEMP:18"


def test_mapping_330_set_temp_30():
    assert es.command_to_uart(330) == "CMD:SET_TEMP:30"


def test_mapping_318_prefers_temperature():
    assert es.command_to_uart(318, temperature=25) == "CMD:SET_TEMP:25"


def test_mapping_invalid_codes():
    for code in (0, 3, 12, 300, 317, 331, 999):
        assert es.command_to_uart(code) is None, f"code {code} phai tra None"


def test_mapping_none():
    assert es.command_to_uart(None) is None


# ---------------------------------------------------------------------------
# Send tests (FakeSerial)
# ---------------------------------------------------------------------------

def test_send_ac_on_ack_success():
    fake = FakeSerial([["ESP32 READY"], ["RX: CMD:AC_ON", "DECODE: AC_ON", "RESP:OK:AC_ON"]])
    sender, _ = make_sender(fake)
    res = sender.send_command(1)
    assert res.success is True
    assert res.connected is True
    assert res.response == "RESP:OK:AC_ON"
    assert fake.written == [b"CMD:AC_ON\n"]


def test_send_set_temp_ack_success():
    fake = FakeSerial([["ESP32 READY"], ["RX: CMD:SET_TEMP:25", "DECODE: SET_TEMPERATURE=25", "RESP:OK:SET_TEMP:25"]])
    sender, _ = make_sender(fake)
    res = sender.send_command(325)
    assert res.success is True
    assert res.connected is True
    assert res.response == "RESP:OK:SET_TEMP:25"
    assert fake.written == [b"CMD:SET_TEMP:25\n"]


def test_send_debug_lines_before_resp_still_success():
    fake = FakeSerial([["ESP32 READY"], ["RX: CMD:AC_ON", "DECODE: AC_ON", "RESP:OK:AC_ON"]])
    sender, _ = make_sender(fake)
    res = sender.send_command(1)
    assert res.success is True
    assert res.response == "RESP:OK:AC_ON"


def test_send_air_auto_ack_success():
    fake = FakeSerial([["ESP32 READY"], ["RESP:OK:AIR_AUTO"]])
    sender, _ = make_sender(fake)
    res = sender.send_command(11)
    assert res.success is True
    assert res.connected is True
    assert res.response == "RESP:OK:AIR_AUTO"
    assert fake.written == [b"CMD:AIR_AUTO\n"]


def test_send_air_auto_debug_lines_before_resp_still_success():
    fake = FakeSerial([["ESP32 READY"], ["RX: CMD:AIR_AUTO", "DECODE: AIR_AUTO", "RESP:OK:AIR_AUTO"]])
    sender, _ = make_sender(fake)
    res = sender.send_command(11)
    assert res.success is True
    assert res.response == "RESP:OK:AIR_AUTO"
    assert fake.written == [b"CMD:AIR_AUTO\n"]


def test_send_new_debug_tags_before_resp_still_success():
    fake = FakeSerial([["ESP32 READY"], [
        "[UART_RX][100 ms] len=12 raw=CMD:AIR_AUTO",
        "[PARSE][100 ms] Command=AIR_AUTO",
        "[ACTION][101 ms] Handler=AIR_AUTO (ACK-only prototype)",
        "[UART_TX][101 ms] RESP:OK:AIR_AUTO",
        "RESP:OK:AIR_AUTO",
    ]])
    sender, _ = make_sender(fake)
    res = sender.send_command(11)
    assert res.success is True
    assert res.response == "RESP:OK:AIR_AUTO"
    assert fake.written == [b"CMD:AIR_AUTO\n"]


def test_send_resp_error_not_success():
    fake = FakeSerial([["ESP32 READY"], ["RESP:ERROR:UNKNOWN_COMMAND"]])
    sender, _ = make_sender(fake)
    res = sender.send_command(325)
    assert res.success is False
    assert res.connected is True
    assert res.response == "RESP:ERROR:UNKNOWN_COMMAND"
    assert res.error == ""


def test_send_timeout_then_retry_success():
    fake = FakeSerial([["ESP32 READY"], [], ["RESP:OK:AC_ON"]])
    sender, _ = make_sender(fake, retries=1, resp_timeout=0.05)
    res = sender.send_command(1)
    assert res.success is True
    assert res.response == "RESP:OK:AC_ON"
    assert fake.writes == 2
    assert fake.written == [b"CMD:AC_ON\n", b"CMD:AC_ON\n"]


def test_send_timeout_all_attempts_no_response():
    fake = FakeSerial([["ESP32 READY"], [], []])
    sender, _ = make_sender(fake, retries=1, resp_timeout=0.05)
    res = sender.send_command(1)
    assert res.success is False
    assert res.error == "NO_RESPONSE"
    assert res.connected is True
    assert fake.writes == 2


def test_send_esp32_not_found():
    sender = es.Esp32Sender(port_finder=finder_system_exit)
    res = sender.send_command(1)
    assert res.success is False
    assert res.connected is False
    assert res.error == "ESP32_NOT_FOUND"


def test_send_serial_exception_on_write():
    fake = FakeSerial([["ESP32 READY"]], raise_on_write=True)
    sender, _ = make_sender(fake)
    res = sender.send_command(1)
    assert res.success is False
    assert res.connected is False
    assert res.error == "SERIAL_ERROR"


def test_send_serial_exception_on_open():
    def factory(port, baudrate, timeout):
        raise serial.SerialException("fake open fail")

    sender = es.Esp32Sender(port_finder=fake_finder_ok(), serial_factory=factory)
    res = sender.send_command(1)
    assert res.success is False
    assert res.connected is False
    assert res.error == "SERIAL_ERROR"


def test_invalid_command_does_not_open_serial():
    fake = FakeSerial([["ESP32 READY"]])
    sender, factory = make_sender(fake)
    res = sender.send_command(None)
    assert res.success is False
    assert res.error == "INVALID_COMMAND"
    assert factory.calls == 0
    assert fake.written == []


def test_invalid_command_no_write_when_connected():
    fake = FakeSerial([["ESP32 READY"]])
    sender, _ = make_sender(fake)
    assert sender.connect() is True
    res = sender.send_command(999)
    assert res.success is False
    assert res.error == "INVALID_COMMAND"
    assert fake.written == []


def test_connect_and_close():
    fake = FakeSerial([["ESP32 READY"]])
    sender, _ = make_sender(fake)
    assert sender.connect() is True
    assert sender.connected is True
    sender.close()
    assert sender.connected is False
    assert fake.closed is True


def test_handshake_banner_missing_connect_fails():
    es.READY_TIMEOUT = 0.05
    try:
        fake = FakeSerial([["(no banner)"]])
        sender, _ = make_sender(fake)
        assert sender.connect() is False
        assert sender.connected is False
    finally:
        es.READY_TIMEOUT = 3.0


# ---------------------------------------------------------------------------
# ASYNC ACK (STEP 6)
# ---------------------------------------------------------------------------

def wait_until(predicate, timeout=2.0) -> bool:
    deadline = time.time() + timeout
    while time.time() < deadline:
        if predicate():
            return True
        time.sleep(0.01)
    return False


def make_async_sender(fake, **kwargs):
    factory = FakeSerialFactory(fake)
    sender = es.Esp32Sender(
        port_finder=fake_finder_ok(),
        serial_factory=factory,
        resp_timeout=0.05,
        retries=0,
        **kwargs,
    )
    assert sender.connect() is True
    sender.start_ack_listener()
    return sender, factory


def test_async_write_returns_before_ack():
    fake = FakeSerial([["ESP32 READY"], ["RESP:OK:AIR_AUTO"]])
    sender, _ = make_async_sender(fake)
    res = sender.send_async(11)
    assert res.success is True
    assert res.state == "SENT"          # UART_WRITE_OK, chua can ACK
    assert res.error == ""
    assert fake.written == [b"CMD:AIR_AUTO\n"]
    assert len(sender._inflight) == 1
    sender.close()


def test_async_ack_ok_updates_status():
    fake = FakeSerial([["ESP32 READY"], ["RX: CMD:AIR_AUTO", "DECODE: AIR_AUTO", "RESP:OK:AIR_AUTO"]])
    sender, _ = make_async_sender(fake)
    res = sender.send_async(11)
    assert res.state == "SENT"
    ok = wait_until(lambda: sender._last_ack == "RESP:OK:AIR_AUTO")
    assert ok, "khong nhan ACK_OK"
    assert len(sender._inflight) == 0    # inflight da giai phong
    sender.close()


def test_async_ack_error_sets_error():
    fake = FakeSerial([["ESP32 READY"], ["RESP:ERROR:UNKNOWN_COMMAND"]])
    sender, _ = make_async_sender(fake)
    res = sender.send_async(11)
    assert res.state == "SENT"
    ok = wait_until(lambda: len(sender._inflight) == 0)
    assert ok, "ACK_ERROR khong giai phong inflight"
    sender.close()


def test_async_inflight_limit_queues_6th():
    fake = FakeSerial([["ESP32 READY"]])   # khong co ACK -> inflight giu nguyen
    sender, _ = make_async_sender(fake)
    for _ in range(5):
        assert sender.send_async(11).state == "SENT"
    assert len(sender._inflight) == 5
    res = sender.send_async(9)            # linh 6 -> pending
    assert res.state == "QUEUED"
    assert res.success is True
    assert len(sender._pending) == 1
    assert fake.writes == 5               # KHONG spam UART
    sender.close()


def test_async_pending_drained_after_ack():
    fake = FakeSerial([["ESP32 READY"], [], [], [], [], ["RESP:OK:AIR_AUTO"]])
    sender, _ = make_async_sender(fake)
    for _ in range(5):
        assert sender.send_async(11).state == "SENT"
    assert sender.send_async(11).state == "QUEUED"
    ok = wait_until(lambda: len(sender._pending) == 0 and fake.writes == 6)
    assert ok, "pending khong duoc promote sau ACK"
    assert len(sender._inflight) == 5
    sender.close()


def test_async_queue_full_drops_newest():
    fake = FakeSerial([["ESP32 READY"]])
    sender, _ = make_async_sender(fake, max_pending=5)
    for _ in range(5):
        assert sender.send_async(11).state == "SENT"
    for code in (9, 8, 10, 5, 7):         # 5 pending (FIFO)
        assert sender.send_async(code).state == "QUEUED"
    assert len(sender._pending) == 5
    res = sender.send_async(1)            # AC_ON != tail (FAN_OFF) -> DROP NEWEST
    assert res.success is False
    assert res.state == "DROPPED"
    assert res.error == "QUEUE_FULL"
    assert len(sender._pending) == 5      # command cu khong bi mat
    assert fake.writes == 5
    sender.close()


def test_async_queue_full_coalesces_safe_duplicate():
    fake = FakeSerial([["ESP32 READY"]])
    sender, _ = make_async_sender(fake, max_pending=5)
    for _ in range(5):
        assert sender.send_async(11).state == "SENT"
    for code in (9, 8, 10, 5, 8):         # pending FULL: FOOT FACE DEFROST DOWN FACE
        assert sender.send_async(code).state == "QUEUED"
    assert len(sender._pending) == 5
    res = sender.send_async(8)            # AIR_FACE == tail (8) + SAFE -> gop
    assert res.success is True
    assert res.state == "COALESCED"
    assert len(sender._pending) == 5      # khong tang queue
    res2 = sender.send_async(4)           # TEMP_UP != tail (AIR_FACE) -> drop
    assert res2.state == "DROPPED"
    assert len(sender._pending) == 5
    sender.close()


def test_async_ack_timeout_decrements_inflight():
    fake = FakeSerial([["ESP32 READY"]])   # ESP32 khong ACK bao gio
    sender, _ = make_async_sender(fake, ack_timeout=0.2)
    assert sender.send_async(11).state == "SENT"
    ok = wait_until(lambda: len(sender._inflight) == 0, timeout=2.0)
    assert ok, "ACK_TIMEOUT khong giai phong inflight"
    assert "CMD:AIR_AUTO" in sender._timeout_log
    sender.close()


def test_async_write_fail_serial_error():
    fake = FakeSerial([["ESP32 READY"]], raise_on_write=True)
    sender, _ = make_async_sender(fake)
    res = sender.send_async(11)
    assert res.success is False
    assert res.state == "SEND_FAILED"
    assert res.error == "SERIAL_ERROR"
    assert res.connected is False
    sender.close()


def test_async_reader_serial_error_drops_connection():
    # readline_count 1 = banner (connect), 2 = ACK read -> fail o lan doc ACK
    fake = FakeSerial([["ESP32 READY"], ["RESP:OK:AIR_AUTO"]], readline_fail_after=2)
    sender, _ = make_async_sender(fake)
    sender.send_async(11)
    ok = wait_until(lambda: sender.connected is False)
    assert ok, "reader serial error khong drop connection"
    assert len(sender._inflight) == 0
    sender.close()


def test_async_legacy_send_command_still_works():
    fake = FakeSerial([["ESP32 READY"], ["RESP:OK:AIR_AUTO"]])
    sender, _ = make_async_sender(fake)
    res = sender.send_command(11)          # legacy sync path khong doi
    assert res.success is True
    assert res.response == "RESP:OK:AIR_AUTO"
    sender.close()


# ---------------------------------------------------------------------------
# Runner
# ---------------------------------------------------------------------------

def run_all() -> None:
    tests = [
        (name, fn) for name, fn in globals().items()
        if name.startswith("test_") and callable(fn)
    ]
    tests.sort()
    failed = 0
    for name, fn in tests:
        try:
            fn()
            print(f"PASS: {name}")
        except AssertionError as e:
            failed += 1
            print(f"FAIL: {name}: {e}")
        except Exception as e:  # noqa: BLE001
            failed += 1
            print(f"ERROR: {name}: {e!r}")
    print(f"\n{len(tests) - failed}/{len(tests)} passed")
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    run_all()
