"""test_command_ai.py - test command_ai.py (rule-based classifier).

Khong can microphone/Vosk/ESP32/Piper.

Chay:  python test_command_ai.py
"""

import sys
from pathlib import Path

sys.stdout.reconfigure(encoding="utf-8")
sys.stderr.reconfigure(encoding="utf-8")

sys.path.insert(0, str(Path(__file__).resolve().parent))

from command_ai import classify

# (input, expected_intent, expected_code, expected_temp)
CASES: list[tuple[str, str, int | None, int | None]] = [
    # ---- NHOM 1: AC_ON ----
    ("mở điều hòa", "AC_ON", 1, None),
    ("bat dieu hoa", "AC_ON", 1, None),
    ("mo may lanh", "AC_ON", 1, None),
    ("bật máy lạnh", "AC_ON", 1, None),
    ("cho dieu hoa chay", "AC_ON", 1, None),
    ("mo dieu hoa len", "AC_ON", 1, None),
    # ---- NHOM 2: AC_OFF ----
    ("tắt điều hòa", "AC_OFF", 2, None),
    ("tat may lanh", "AC_OFF", 2, None),
    ("đóng điều hòa", "AC_OFF", 2, None),
    ("ngat dieu hoa", "AC_OFF", 2, None),
    # ---- NHOM 3: TEMP_UP ----
    ("tang nhiet do", "TEMP_UP", 4, None),
    ("tăng lên một chút", "TEMP_UP", 4, None),
    ("cho nóng lên", "TEMP_UP", 4, None),
    ("cho am hon", "TEMP_UP", 4, None),
    # ---- NHOM 4: TEMP_DOWN ----
    ("giam nhiet do", "TEMP_DOWN", 5, None),
    ("cho lạnh hơn", "TEMP_DOWN", 5, None),
    ("hạ nhiệt độ", "TEMP_DOWN", 5, None),
    ("giam xuong mot chut", "TEMP_DOWN", 5, None),
    # ---- NHOM 5: FAN_ON ----
    ("mo quat", "FAN_ON", 6, None),
    ("bat quat", "FAN_ON", 6, None),
    # ---- NHOM 6: FAN_OFF ----
    ("tắt quạt", "FAN_OFF", 7, None),
    ("ngat quat", "FAN_OFF", 7, None),
    # ---- NHOM 7: AIR_FACE ----
    ("gio len mat", "AIR_FACE", 8, None),
    ("hướng gió lên mặt", "AIR_FACE", 8, None),
    ("thoi vao mat", "AIR_FACE", 8, None),
    # ---- NHOM 8: AIR_FOOT ----
    ("gio xuong chan", "AIR_FOOT", 9, None),
    ("hướng gió xuống chân", "AIR_FOOT", 9, None),
    ("thoi xuong chan", "AIR_FOOT", 9, None),
    # ---- NHOM 9: AIR_DEFROST ----
    ("gio vao kinh", "AIR_DEFROST", 10, None),
    ("thoi kinh", "AIR_DEFROST", 10, None),
    ("say kinh", "AIR_DEFROST", 10, None),
    ("huong gio suoi kinh chan gio", "AIR_DEFROST", 10, None),
    # ---- NHOM 10: SET_TEMPERATURE (chu so) ----
    ("đặt nhiệt độ 23 độ", "SET_TEMPERATURE", 323, 23),
    ("đặt nhiệt độ 25 độ", "SET_TEMPERATURE", 325, 25),
    ("dat nhiet do 25", "SET_TEMPERATURE", 325, 25),
    ("đặt nhiệt độ 30 độ", "SET_TEMPERATURE", 330, 30),
    ("chỉnh điều hòa xuống 24 độ", "SET_TEMPERATURE", 324, 24),
    ("cho may lanh 27 do", "SET_TEMPERATURE", 327, 27),
    ("để 28 độ", "SET_TEMPERATURE", 328, 28),
    ("toi muon 30 do", "SET_TEMPERATURE", 330, 30),
    # ---- NHOM 10b: SET_TEMPERATURE (so chu tieng Viet) ----
    ("đặt nhiệt độ hai mươi lăm độ", "SET_TEMPERATURE", 325, 25),
    ("dat nhiet do hai muoi ba do", "SET_TEMPERATURE", 323, 23),
    ("dat nhiet do ba muoi do", "SET_TEMPERATURE", 330, 30),
    # ---- NHOM 11: INVALID_TEMPERATURE ----
    ("đặt nhiệt độ 20 độ", "INVALID_TEMPERATURE", None, None),
    ("đặt nhiệt độ 35 độ", "INVALID_TEMPERATURE", None, None),
    ("dat nhiet do nay lam ho so thue", "INVALID_TEMPERATURE", None, None),
    ("cho 19 do", "INVALID_TEMPERATURE", None, None),
    # ---- NHOM 12: UNKNOWN ----
    ("điều hòa", "UNKNOWN", None, None),
    ("hom nay troi dep", "UNKNOWN", None, None),
    ("toi muon di choi", "UNKNOWN", None, None),
    ("chay", "UNKNOWN", None, None),
    ("abcxyz", "UNKNOWN", None, None),
    # ---- NHOM 13: HELLO ----
    ("xin chào", "HELLO", None, None),
    ("hello", "HELLO", None, None),
    # ---- NHOM 14: VOSK GARBAGE / FALSE POSITIVE -> UNKNOWN ----
    ("bất quá", "UNKNOWN", None, None),
    ("bat qua", "UNKNOWN", None, None),
    ("tổng quát", "UNKNOWN", None, None),
    ("hoặc quát", "UNKNOWN", None, None),
    ("do lên phát", "UNKNOWN", None, None),
    ("do lê hát", "UNKNOWN", None, None),
    ("góc nhìn", "UNKNOWN", None, None),
    ("góc nhiệt độ hoa ngữ làm ruộng xe", "UNKNOWN", None, None),
    ("bat den", "UNKNOWN", None, None),
    ("bat may", "UNKNOWN", None, None),
    ("mo cua", "UNKNOWN", None, None),
    ("mo den", "UNKNOWN", None, None),
    ("tat den", "UNKNOWN", None, None),
    ("tat may", "UNKNOWN", None, None),
    ("dong cua", "UNKNOWN", None, None),
    ("gió", "UNKNOWN", None, None),
    ("nhiệt độ", "UNKNOWN", None, None),
    ("dat nhiet do hoac ruot russia", "UNKNOWN", None, None),
    ("do lac o mot", "UNKNOWN", None, None),
    ("da dieu hoa phu", "UNKNOWN", None, None),
    ("moc quat", "UNKNOWN", None, None),
    ("dat quet", "UNKNOWN", None, None),
    ("giai nhiet doc", "UNKNOWN", None, None),
    ("doanh nghiep dia oc", "UNKNOWN", None, None),
    ("at deu hoa", "UNKNOWN", None, None),
    # ---- NHOM 15: NEGATION (phu dinh) -> UNKNOWN ----
    ("khong bat dieu hoa", "UNKNOWN", None, None),
    ("dung bat dieu hoa", "UNKNOWN", None, None),
    ("dung tat dieu hoa", "UNKNOWN", None, None),
    ("khong tat quat", "UNKNOWN", None, None),
    ("mở điều hòa không", "UNKNOWN", None, None),
    ("chua bat dieu hoa", "UNKNOWN", None, None),
    # ---- NHOM 16: CONTEXT REQUIRED (AC/FAN dung context) ----
    ("bat dieu hoa", "AC_ON", 1, None),
    ("mo dieu hoa", "AC_ON", 1, None),
    ("bat may lanh", "AC_ON", 1, None),
    ("tat dieu hoa", "AC_OFF", 2, None),
    ("dong dieu hoa", "AC_OFF", 2, None),
    ("tat may lanh", "AC_OFF", 2, None),
    ("bat quat", "FAN_ON", 6, None),
    ("mo quat", "FAN_ON", 6, None),
    ("tat quat", "FAN_OFF", 7, None),
    ("ngat quat", "FAN_OFF", 7, None),
]

GROUPS = [
    (1, "AC_ON", "AC_ON"),
    (2, "AC_OFF", "AC_OFF"),
    (3, "TEMP_UP", "TEMP_UP"),
    (4, "TEMP_DOWN", "TEMP_DOWN"),
    (5, "FAN_ON", "FAN_ON"),
    (6, "FAN_OFF", "FAN_OFF"),
    (7, "AIR_FACE", "AIR_FACE"),
    (8, "AIR_FOOT", "AIR_FOOT"),
    (9, "AIR_DEFROST", "AIR_DEFROST"),
    (10, "SET_TEMPERATURE", "SET_TEMPERATURE"),
    (11, "INVALID_TEMPERATURE", "INVALID_TEMPERATURE"),
    (12, "UNKNOWN", "UNKNOWN"),
    (13, "HELLO", "HELLO"),
    (14, "VOSK GARBAGE", "UNKNOWN"),
    (15, "NEGATION", "UNKNOWN"),
    (16, "AC/FAN CONTEXT", "AC_ON"),
]


def main() -> int:
    total = 0
    passed = 0
    failed = 0

    current_group: str | None = None
    for text, expected_intent, expected_code, expected_temp in CASES:
        group = next((name for _, name, intent in GROUPS if intent == expected_intent), "")
        if group != current_group:
            current_group = group
            print(f"\n=== NHOM {group} ===")

        result = classify(text)
        total += 1

        ok = (
            result.intent == expected_intent
            and result.command_code == expected_code
            and result.temperature == expected_temp
        )

        actual = (
            f"intent={result.intent}, code={result.command_code}, "
            f"temp={result.temperature}"
        )
        expected = (
            f"intent={expected_intent}, code={expected_code}, temp={expected_temp}"
        )

        if ok:
            passed += 1
            print(f"[PASS] input={text!r}")
            print(f"      expected: {expected}")
            print(f"      actual:   {actual}")
        else:
            failed += 1
            print(f"[FAIL] input={text!r}")
            print(f"      expected: {expected}")
            print(f"      actual:   {actual}")

    print("\n========================================")
    print(f"TOTAL: {total}")
    print(f"PASS : {passed}")
    print(f"FAIL : {failed}")
    print("========================================")

    return 1 if failed > 0 else 0


if __name__ == "__main__":
    sys.exit(main())