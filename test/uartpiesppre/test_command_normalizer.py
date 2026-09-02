"""Test command_normalizer.py — KHONG can ESP32/Piper/Vosk.

Chay:  python test_command_normalizer.py
"""

import sys
from pathlib import Path

sys.stdout.reconfigure(encoding="utf-8")
sys.stderr.reconfigure(encoding="utf-8")

sys.path.insert(0, str(Path(__file__).resolve().parent))

from command_normalizer import normalize_command

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


def run_all() -> None:
    failed = 0

    for text, expected in VALID_CASES:
        got = normalize_command(text)
        if got == expected:
            print(f"PASS: {text!r} -> {got}")
        else:
            failed += 1
            print(f"FAIL: {text!r} -> {got!r} (expected {expected!r})")

    for text in INVALID_CASES:
        got = normalize_command(text)
        if got is None:
            print(f"PASS: {text!r} -> None")
        else:
            failed += 1
            print(f"FAIL: {text!r} -> {got!r} (expected None)")

    total = len(VALID_CASES) + len(INVALID_CASES)
    print(f"\n{total - failed}/{total} passed")
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    run_all()