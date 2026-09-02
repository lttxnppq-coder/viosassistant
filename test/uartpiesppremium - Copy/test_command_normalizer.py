"""Test command_normalizer.py — KHONG can ESP32/Piper/Vosk.

Chay:  python test_command_normalizer.py
"""

import sys
from pathlib import Path

sys.stdout.reconfigure(encoding="utf-8")
sys.stderr.reconfigure(encoding="utf-8")

sys.path.insert(0, str(Path(__file__).resolve().parent))

from command_normalizer import (
    canonicalize,
    correct_stt_errors,
    normalize_command,
    normalize_text,
    normalize_vietnamese_text,
)

VALID_CASES = [
    ("xin chào", "HELLO"),
    ("xin chao", "HELLO"),
    ("Xin Chào", "HELLO"),
    ("XIN CHÀO", "HELLO"),
    ("hello", "HELLO"),
    ("Hello", "HELLO"),
    ("tiến lên", "FORWARD"),
    ("tien len", "FORWARD"),
    ("Tiến Lên", "FORWARD"),
    ("lùi lại", "BACKWARD"),
    ("lui lai", "BACKWARD"),
    ("rẽ trái", "LEFT"),
    ("re trai", "LEFT"),
    ("rẽ phải", "RIGHT"),
    ("re phai", "RIGHT"),
    ("dừng", "STOP"),
    ("dừng lại", "STOP"),
    ("dung", "STOP"),
    ("dung lai", "STOP"),
    ("stop", "STOP"),
    ("Stop!", "STOP"),
    ("  tiến   lên  ", "FORWARD"),
    ("Tiến lên.", "FORWARD"),
    ("rẽ trái.", "LEFT"),
]

INVALID_CASES = [
    "",
    None,
    "tôi muốn tiến lên",
    "toi muon tien len",
    "tiến lên ngay",
    "tien len di",
    "lui",
    "tien",
    "re",
    "abcxyz",
    "xin chào mọi người",
    "tiến lên và dừng lại",
    "   ",
    ".",
]


# ---- TANG 1: normalize_text (robot) — KHONG map "đ" -> "d" ----
# "đứng" (stand) phai GIU khac "dung" (stop) — regression quan trong.
TIER1_CASES = [
    ("mở điều hòa", "mo đieu hoa"),
    ("bặt điều hòa", "bat đieu hoa"),
    ("điều hòa", "đieu hoa"),
    ("đứng", "đung"),
    ("dừng", "dung"),
]

# ---- TANG 1: normalize_vietnamese_text (pipeline AC) — co map "đ" -> "d" ----
TIER1_VN_CASES = [
    ("mở điều hòa", "mo dieu hoa"),
    ("mờ điều hòa", "mo dieu hoa"),
    ("bặt điều hòa", "bat dieu hoa"),
    ("bật điều hoà", "bat dieu hoa"),
    ("điều hòa", "dieu hoa"),
    ("điều hoà", "dieu hoa"),
    ("sưởi kính chắn gió", "suoi kinh chan gio"),
    ("hướng gió lên mặt", "huong gio len mat"),
    ("hướng gió xuống chân", "huong gio xuong chan"),
    ("nhiệt độ", "nhiet do"),
    ("hai lăm độ", "hai lam do"),
    ("hai tư độ", "hai tu do"),
    ("đặt 25 độ!", "dat 25 do"),
    ("", ""),
]

# ---- TANG 2: canonicalize (tu dong nghia, word boundary) ----
CANONICAL_CASES = [
    ("bat dieu hoa", "bat dieu hoa"),
    ("bat may lanh", "bat dieu hoa"),
    ("mo may dieu hoa", "mo dieu hoa"),
    ("tat may lanh len", "tat dieu hoa len"),
    ("bat may lanh di", "bat dieu hoa di"),
    # khong doi text khong co tu dong nghia
    ("xin chao", "xin chao"),
    ("25 do ngoai troi", "25 do ngoai troi"),
    # khong match substring ("may lanh" trong "may lanh sao" van la synonym - OK)
    ("may lang", "may lang"),
]

# ---- TANG 3: correct_stt_errors (loi STT/vung mien, gate thiet bi) ----
STT_ERROR_CASES = [
    # "bạc" -> "bật" chi khi co cum thiet bi
    ("bac dieu hoa", "bat dieu hoa"),
    ("bac may lanh", "bat may lanh"),
    ("bac quat", "bat quat"),
    ("bac suoi kinh", "bat suoi kinh"),
    # "bạc" don le khong doi (false positive gate)
    ("bac", "bac"),
    ("bac bac", "bac bac"),
    ("bac nghe", "bac nghe"),
    # "máy lạng" -> "máy lạnh" (n -> ng, giong Nam)
    ("mo may lang", "mo may lanh"),
    ("tat may lang", "tat may lanh"),
    # "máy lạng" don le van doi (phrase "may lang" khong ton tai ngoai AC)
    ("may lang", "may lanh"),
    # "tắc" -> "tắt" chi khi co cum thiet bi
    ("tac dieu hoa", "tat dieu hoa"),
    ("tac may lanh", "tat may lanh"),
    ("tac quat", "tat quat"),
    ("tac duong", "tac duong"),  # "tắc đường" khong doi
    ("tac", "tac"),
    # "quạc lên" -> "quạt lên"
    ("quac len", "quat len"),
    ("quac quac", "quac quac"),
    # text sach khong bi doi
    ("bat dieu hoa", "bat dieu hoa"),
    ("tat quat", "tat quat"),
    ("", ""),
]


def run_all() -> None:
    failed = 0
    total = 0

    def check(actual, expected, label):
        nonlocal failed, total
        total += 1
        if actual == expected:
            print(f"PASS: {label}: {actual!r}")
        else:
            failed += 1
            print(f"FAIL: {label}: {actual!r} (expected {expected!r})")

    for text, expected in VALID_CASES:
        check(normalize_command(text), expected, f"command({text!r})")

    for text in INVALID_CASES:
        check(normalize_command(text), None, f"command({text!r})->None")

    for text, expected in TIER1_CASES:
        check(normalize_text(text), expected, f"tier1({text!r})")

    for text, expected in TIER1_VN_CASES:
        check(normalize_vietnamese_text(text), expected, f"tier1vn({text!r})")

    for text, expected in CANONICAL_CASES:
        check(canonicalize(text), expected, f"canonicalize({text!r})")

    for text, expected in STT_ERROR_CASES:
        check(correct_stt_errors(text), expected, f"tier3({text!r})")

    print(f"\n{total - failed}/{total} passed")
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    run_all()