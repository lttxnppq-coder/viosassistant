"""PC-only integration test — kiem tra toan bo pipeline logic tren may tinh:

    text gia lap
    -> command_normalizer (normalize/canonicalize trong classify)
    -> command_ai.classify()
    -> CODE
    -> CODE_TO_AUDIO (import tu vosk_test.py - KHONG nhan doi mapping)
    -> audio/*.wav (file ton tai + WAV hop le)

Khong can ESP32, micro, speaker. Khong phat am thanh, khong upload firmware.

Chay:
    python test_pipeline_pc.py
"""

import sys
import wave
from pathlib import Path

sys.stdout.reconfigure(encoding="utf-8")
sys.stderr.reconfigure(encoding="utf-8")

from command_ai import classify
from command_normalizer import canonicalize
from vosk_test import CODE_TO_AUDIO

PROJECT_ROOT = Path(__file__).resolve().parent

# (input, expected_intent, expected_code)
CASES = [
    # ---- Nhom 1: Dieu hoa ----
    ("bật điều hòa", "AC_ON", 1),
    ("tắt điều hòa", "AC_OFF", 2),
    # ---- Nhom 2: Nhiet do ----
    ("tăng nhiệt độ", "TEMP_UP", 4),
    ("giảm nhiệt độ", "TEMP_DOWN", 5),
    ("đặt điều hòa 23 độ", "SET_TEMPERATURE", 323),
    ("đặt điều hòa 24 độ", "SET_TEMPERATURE", 324),
    ("đặt điều hòa 25 độ", "SET_TEMPERATURE", 325),
    ("đặt điều hòa 26 độ", "SET_TEMPERATURE", 326),
    ("đặt điều hòa 27 độ", "SET_TEMPERATURE", 327),
    ("đặt điều hòa 28 độ", "SET_TEMPERATURE", 328),
    ("đặt điều hòa 29 độ", "SET_TEMPERATURE", 329),
    ("đặt điều hòa 30 độ", "SET_TEMPERATURE", 330),
    # ---- Nhom 3: Quat ----
    ("bật quạt", "FAN_ON", 6),
    ("tắt quạt", "FAN_OFF", 7),
    # ---- Nhom 4: Huong gio ----
    ("hướng gió lên mặt", "AIR_FACE", 8),
    ("hướng gió xuống chân", "AIR_FOOT", 9),
    # ---- Nhom 5: Suoi kinh ----
    ("bật chế độ sưởi kính chắn gió", "AIR_DEFROST", 10),
]

# Ca UNKNOWN: cau khong lien quan, KHONG chua tu chao/lenh.
# (Luu y: "xin chào hom nay troi dep" -> HELLO vi chua "xin chao", khong phai UNKNOWN.)
UNKNOWN_CASE = ("hôm nay trời đẹp quá", "UNKNOWN", None)


def wav_valid(path: Path) -> tuple[bool, str]:
    try:
        with wave.open(str(path), "rb") as w:
            frames = w.getnframes()
            channels = w.getnchannels()
            rate = w.getframerate()
            if frames <= 0:
                return False, f"0 frames"
            return True, f"{channels}ch {rate}Hz {frames} frames"
    except Exception as e:  # noqa: BLE001
        return False, str(e)


def resolve_audio(intent: str, code: int | None) -> str | None:
    if code is not None:
        return CODE_TO_AUDIO.get(code)
    return CODE_TO_AUDIO.get(intent)


def run_case(text: str, expected_intent: str, expected_code: int | None) -> bool:
    ok = True

    ai = classify(text)
    code = ai.command_code
    audio_rel = resolve_audio(ai.intent, code)

    print(f"INPUT: {text}")
    print(f"NORMALIZED: {ai.normalized_text}")
    print(f"CANONICALIZED: {canonicalize(ai.normalized_text)}")
    print(f"INTENT: {ai.intent} (expected {expected_intent})")
    print(f"CODE: {code} (expected {expected_code})")
    print(f"AUDIO: {audio_rel}")

    if ai.intent != expected_intent or code != expected_code:
        ok = False
        print("INTENT/CODE: FAIL")
    else:
        print("INTENT/CODE: PASS")

    if audio_rel is None:
        ok = False
        print(f"CODE IN MAPPING: FAIL (khong ton tai trong CODE_TO_AUDIO)")
    else:
        print("CODE IN MAPPING: PASS")

        wav_path = PROJECT_ROOT / audio_rel
        if wav_path.exists():
            print("FILE EXISTS: PASS")
            valid, detail = wav_valid(wav_path)
            print(f"WAV VALID: {'PASS' if valid else 'FAIL'} ({detail})")
            if not valid:
                ok = False
        else:
            print("FILE EXISTS: FAIL")
            ok = False

    print()
    return ok


def main() -> int:
    total = 0
    failed = 0
    command_ok = 0

    print(f"=== PC INTEGRATION TEST — {len(CASES)} lenh + 1 UNKNOWN ===")
    print()

    for text, intent, code in CASES:
        total += 1
        if run_case(text, intent, code):
            command_ok += 1
        else:
            failed += 1

    total += 1
    if run_case(UNKNOWN_CASE[0], UNKNOWN_CASE[1], UNKNOWN_CASE[2]):
        unknown_ok = True
    else:
        unknown_ok = False
        failed += 1

    print(f"COMMAND COVERAGE: {command_ok}/{len(CASES)}")
    print(f"UNKNOWN CASE: {'PASS' if unknown_ok else 'FAIL'}")
    print(f"TOTAL: {total - failed}/{total} passed, {failed} failed")
    print("========================================")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())