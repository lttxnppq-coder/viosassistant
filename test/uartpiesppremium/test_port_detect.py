"""Test logic esp32_port.py bằng port giả — KHÔNG cần ESP32 thật.

Chạy:  python test_port_detect.py
"""

import sys
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parent))

import esp32_port as ep


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def fake_port(
    device,
    description="",
    hwid="",
    vid=None,
    pid=None,
    serial_number=None,
    manufacturer=None,
):
    return SimpleNamespace(
        device=device,
        description=description,
        hwid=hwid,
        vid=vid,
        pid=pid,
        serial_number=serial_number,
        manufacturer=manufacturer,
    )


def to_info(port) -> ep.PortInfo:
    return ep.PortInfo.from_list_port(port)


def set_ports(ports) -> None:
    ep.enumerate_ports = lambda: [to_info(p) for p in ports]


def reset_ep() -> None:
    # Test KHÔNG được ghi/đọc config.json thật: luôn trả cache về trạng thái
    # "trống" (các test cache sẽ tự set ep.load_config riêng).
    ep.enumerate_ports = ep.__dict__.get("_orig_enumerate_ports")
    ep.load_config = lambda: {}
    ep.save_cache = lambda port: None
    ep.verify_esp32 = _verify_stub  # luôn trả về stub mặc định (không mở serial thật)


_verify_stub = lambda device: True  # mặc định: handshake thành công (mock)

# Lưu bản gốc enumerate_ports (không cache thật trong test).
ep._orig_enumerate_ports = ep.enumerate_ports
ep.load_config = lambda: {}
ep.save_cache = lambda port: None
ep._orig_verify_esp32 = ep.verify_esp32
ep.verify_esp32 = _verify_stub


# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

def test_no_ports():
    set_ports([])
    try:
        try:
            ep.find_esp32_port()
            assert False, "phai bao loi khi khong co COM nao"
        except SystemExit:
            pass
    finally:
        reset_ep()


def test_only_bluetooth():
    set_ports([
        fake_port("COM5", "Standard Serial over Bluetooth link",
                  "BTHENUM\\{00001101-0000-1000-8000-00805F9B34FB}_LOCALMFG&0000\\7&000000000000_00000002",
                  None, None, None, "Microsoft"),
        fake_port("COM6", "Standard Serial over Bluetooth link",
                  "BTHENUM\\{00001101-0000-1000-8000-00805F9B34FB}_LOCALMFG&0002\\7&000000000000_00000000",
                  None, None, None, "Microsoft"),
    ])
    try:
        try:
            ep.find_esp32_port()
            assert False, "khong duoc chon cong Bluetooth"
        except SystemExit:
            pass
    finally:
        reset_ep()


def test_bluetooth_detected_and_scored_zero():
    p = to_info(fake_port("COM6", "Standard Serial over Bluetooth link",
                          "BTHENUM\\...", None, None, None, "Microsoft"))
    assert p.is_bluetooth()
    assert ep.score_port(p)[0] == 0


def test_one_esp32_cp210x_auto_select():
    set_ports([
        fake_port("COM9", "Silicon Labs CP210x USB to UART Bridge",
                  "USB VID:PID=10C4:EA60 SER=ABC123",
                  0x10C4, 0xEA60, "ABC123", "Silicon Laboratories"),
    ])
    try:
        assert ep.find_esp32_port() == "COM9"
    finally:
        reset_ep()


def test_one_esp32_ch340_auto_select():
    set_ports([
        fake_port("COM11", "USB-SERIAL CH340",
                  "USB VID:PID=1A86:7523 SER=XYZ789",
                  0x1A86, 0x7523, "XYZ789", None),
    ])
    try:
        assert ep.find_esp32_port() == "COM11"
    finally:
        reset_ep()


def test_unknown_usb_serial_not_selected():
    # USB serial không xác định (VID/PID lạ) -> không đoán mò, báo lỗi.
    set_ports([
        fake_port("COM7", "USB Serial Device",
                  "USB VID:PID=1234:5678", 0x1234, 0x5678, None, None),
    ])
    try:
        try:
            ep.find_esp32_port()
            assert False, "khong duoc chon USB serial khong nhan dien duoc"
        except SystemExit:
            pass
    finally:
        reset_ep()


def test_multiple_usb_serial_only_esp32_like_selected():
    set_ports([
        fake_port("COM8", "USB Serial Device",
                  "USB VID:PID=1234:5678", 0x1234, 0x5678, None, None),
        fake_port("COM9", "Silicon Labs CP210x USB to UART Bridge",
                  "USB VID:PID=10C4:EA60 SER=ABC123",
                  0x10C4, 0xEA60, "ABC123", "Silicon Laboratories"),
    ])
    try:
        assert ep.find_esp32_port() == "COM9"
    finally:
        reset_ep()


def test_multiple_esp32_requires_selection():
    set_ports([
        fake_port("COM5", "Silicon Labs CP210x USB to UART Bridge",
                  "USB VID:PID=10C4:EA60 SER=ESP1",
                  0x10C4, 0xEA60, "ESP1", "Silicon Laboratories"),
        fake_port("COM9", "Silicon Labs CP210x USB to UART Bridge",
                  "USB VID:PID=10C4:EA60 SER=ESP2",
                  0x10C4, 0xEA60, "ESP2", "Silicon Laboratories"),
    ])
    try:
        with patch("builtins.input", return_value="2"):
            assert ep.find_esp32_port() == "COM9"
        with patch("builtins.input", return_value="1"):
            assert ep.find_esp32_port() == "COM5"
    finally:
        reset_ep()


def test_forced_port_valid():
    set_ports([
        fake_port("COM11", "Silicon Labs CP210x USB to UART Bridge",
                  "USB VID:PID=10C4:EA60 SER=ABC123",
                  0x10C4, 0xEA60, "ABC123", "Silicon Laboratories"),
    ])
    try:
        assert ep.find_esp32_port(forced="COM11") == "COM11"
    finally:
        reset_ep()


def test_forced_port_missing():
    set_ports([
        fake_port("COM9", "Silicon Labs CP210x USB to UART Bridge",
                  "USB VID:PID=10C4:EA60 SER=ABC123",
                  0x10C4, 0xEA60, "ABC123", "Silicon Laboratories"),
    ])
    try:
        try:
            ep.find_esp32_port(forced="COM99")
            assert False, "port khong ton tai phai bao loi"
        except SystemExit:
            pass
    finally:
        reset_ep()


def test_forced_port_bluetooth_rejected():
    set_ports([
        fake_port("COM5", "Standard Serial over Bluetooth link",
                  "BTHENUM\\...", None, None, None, "Microsoft"),
    ])
    try:
        try:
            ep.find_esp32_port(forced="COM5")
            assert False, "khong duoc phep dung cong Bluetooth"
        except SystemExit:
            pass
    finally:
        reset_ep()


def test_cache_stale_port_missing_triggers_redetect():
    # Cache trỏ COM9 nhưng COM9 không còn -> auto-detect lại.
    set_ports([
        fake_port("COM11", "USB-SERIAL CH340",
                  "USB VID:PID=1A86:7523 SER=XYZ789",
                  0x1A86, 0x7523, "XYZ789", None),
    ])
    ep.load_config = lambda: {"last_port": "COM9",
                              "last_hwid": "USB VID:PID=10C4:EA60 SER=OLD"}
    try:
        assert ep.find_esp32_port() == "COM11"
    finally:
        reset_ep()


def test_cache_hwid_mismatch_triggers_redetect():
    # COM9 vẫn tồn tại nhưng giờ là CH340 (hwid khác) -> cache không hợp lệ.
    set_ports([
        fake_port("COM9", "USB-SERIAL CH340",
                  "USB VID:PID=1A86:7523 SER=XYZ789",
                  0x1A86, 0x7523, "XYZ789", None),
    ])
    ep.load_config = lambda: {"last_port": "COM9",
                              "last_hwid": "USB VID:PID=10C4:EA60 SER=OLD"}
    try:
        assert ep.find_esp32_port() == "COM9"  # tìm lại qua auto-detect (CH340)
    finally:
        reset_ep()


def test_cache_valid_used():
    set_ports([
        fake_port("COM9", "Silicon Labs CP210x USB to UART Bridge",
                  "USB VID:PID=10C4:EA60 SER=ABC123",
                  0x10C4, 0xEA60, "ABC123", "Silicon Laboratories"),
    ])
    ep.load_config = lambda: {"last_port": "COM9",
                              "last_hwid": "USB VID:PID=10C4:EA60 SER=ABC123"}
    try:
        assert ep.find_esp32_port() == "COM9"
    finally:
        reset_ep()


def test_native_usb_cdc_selected():
    # ESP32-S3 native USB (303A:1001) giờ là ứng viên hợp lệ (USB CDC on boot),
    # chọn được qua auto-detect (handshake verify "ESP32 READY").
    p = to_info(fake_port("COM12", "USB JTAG/serial debug unit",
                          "USB VID:PID=303A:1001 SER=123",
                          0x303A, 0x1001, "123", "Espressif"))
    assert p.is_native_usb()
    assert ep.score_port(p)[0] > 0
    set_ports([fake_port("COM12", "USB JTAG/serial debug unit",
                         "USB VID:PID=303A:1001 SER=123",
                         0x303A, 0x1001, "123", "Espressif")])
    try:
        assert ep.find_esp32_port() == "COM12"
    finally:
        reset_ep()


# ---------------------------------------------------------------------------
# Handshake tests (mock verify_esp32)
# ---------------------------------------------------------------------------

def test_handshake_fail_single_candidate_errors():
    # 1 candidate nhưng handshake FAIL -> KHÔNG được in "Found ESP32"/chọn.
    set_ports([
        fake_port("COM9", "Silicon Labs CP210x USB to UART Bridge",
                  "USB VID:PID=10C4:EA60 SER=ABC123",
                  0x10C4, 0xEA60, "ABC123", "Silicon Laboratories"),
    ])
    ep.verify_esp32 = lambda device: False
    try:
        try:
            ep.find_esp32_port()
            assert False, "handshake fail phai bao loi"
        except SystemExit:
            pass
    finally:
        reset_ep()


def test_handshake_fail_all_candidates_errors():
    set_ports([
        fake_port("COM5", "Silicon Labs CP210x USB to UART Bridge",
                  "USB VID:PID=10C4:EA60 SER=ESP1",
                  0x10C4, 0xEA60, "ESP1", "Silicon Laboratories"),
        fake_port("COM9", "USB-SERIAL CH340",
                  "USB VID:PID=1A86:7523 SER=XYZ789",
                  0x1A86, 0x7523, "XYZ789", None),
    ])
    ep.verify_esp32 = lambda device: False
    try:
        try:
            ep.find_esp32_port()
            assert False, "tat ca handshake fail phai bao loi"
        except SystemExit:
            pass
    finally:
        reset_ep()


def test_handshake_fail_forced_port_warns_but_continues():
    # --port là explicit override: handshake fail -> cảnh báo, vẫn dùng.
    set_ports([
        fake_port("COM11", "Silicon Labs CP210x USB to UART Bridge",
                  "USB VID:PID=10C4:EA60 SER=ABC123",
                  0x10C4, 0xEA60, "ABC123", "Silicon Laboratories"),
    ])
    ep.verify_esp32 = lambda device: False
    try:
        assert ep.find_esp32_port(forced="COM11") == "COM11"
    finally:
        reset_ep()


def test_cache_serial_mismatch_triggers_redetect():
    # COM9 vẫn tồn tại, hwid khớp, nhưng SN khác cache -> không dùng cache.
    set_ports([
        fake_port("COM9", "Silicon Labs CP210x USB to UART Bridge",
                  "USB VID:PID=10C4:EA60 SER=NEWSN",
                  0x10C4, 0xEA60, "NEWSN", "Silicon Laboratories"),
    ])
    ep.load_config = lambda: {"last_port": "COM9",
                              "last_hwid": "USB VID:PID=10C4:EA60 SER=OLDSN",
                              "last_serial": "OLDSN"}
    try:
        assert ep.find_esp32_port() == "COM9"  # auto-detect lại (verify pass)
    finally:
        reset_ep()


def test_cache_serial_match_used():
    set_ports([
        fake_port("COM9", "Silicon Labs CP210x USB to UART Bridge",
                  "USB VID:PID=10C4:EA60 SER=ABC123",
                  0x10C4, 0xEA60, "ABC123", "Silicon Laboratories"),
    ])
    ep.load_config = lambda: {"last_port": "COM9",
                              "last_hwid": "USB VID:PID=10C4:EA60 SER=ABC123",
                              "last_serial": "ABC123"}
    try:
        assert ep.find_esp32_port() == "COM9"
    finally:
        reset_ep()


def test_cache_valid_but_handshake_fail_redetect():
    # Cache identity hợp lệ nhưng handshake fail trên COM cũ -> auto-detect
    # sang candidate khác (đã verify).
    set_ports([
        fake_port("COM9", "Silicon Labs CP210x USB to UART Bridge",
                  "USB VID:PID=10C4:EA60 SER=OLD",
                  0x10C4, 0xEA60, "OLD", "Silicon Laboratories"),
        fake_port("COM11", "USB-SERIAL CH340",
                  "USB VID:PID=1A86:7523 SER=XYZ789",
                  0x1A86, 0x7523, "XYZ789", None),
    ])
    ep.load_config = lambda: {"last_port": "COM9",
                              "last_hwid": "USB VID:PID=10C4:EA60 SER=OLD",
                              "last_serial": "OLD"}
    ep.verify_esp32 = lambda device: device == "COM11"
    try:
        assert ep.find_esp32_port() == "COM11"
    finally:
        reset_ep()


def test_two_cp210x_diff_serial_cache_picks_correct():
    # 2 adapter CP210x khác SN: cache trỏ đúng thiết bị cũ (COM5, SN ESP1),
    # không được nhầm sang COM9 (SN ESP2).
    set_ports([
        fake_port("COM5", "Silicon Labs CP210x USB to UART Bridge",
                  "USB VID:PID=10C4:EA60 SER=ESP1",
                  0x10C4, 0xEA60, "ESP1", "Silicon Laboratories"),
        fake_port("COM9", "Silicon Labs CP210x USB to UART Bridge",
                  "USB VID:PID=10C4:EA60 SER=ESP2",
                  0x10C4, 0xEA60, "ESP2", "Silicon Laboratories"),
    ])
    ep.load_config = lambda: {"last_port": "COM5",
                              "last_hwid": "USB VID:PID=10C4:EA60 SER=ESP1",
                              "last_serial": "ESP1"}
    try:
        assert ep.find_esp32_port() == "COM5"
    finally:
        reset_ep()


def test_cache_missing_serial_still_used():
    # Cache không có SN (config.json cũ) nhưng hwid khớp -> vẫn dùng được.
    set_ports([
        fake_port("COM9", "Silicon Labs CP210x USB to UART Bridge",
                  "USB VID:PID=10C4:EA60 SER=ABC123",
                  0x10C4, 0xEA60, "ABC123", "Silicon Laboratories"),
    ])
    ep.load_config = lambda: {"last_port": "COM9",
                              "last_hwid": "USB VID:PID=10C4:EA60 SER=ABC123"}
    try:
        assert ep.find_esp32_port() == "COM9"
    finally:
        reset_ep()


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