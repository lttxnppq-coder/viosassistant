"""Test Piper TTS tieng Viet: tao WAV tu text roi phat am thanh.

Chay:  python piper_test.py

CLI thuc te (piper-tts 1.6.1):
    python -m piper -m <model.onnx> -i <input utf-8 file> -f <output.wav>

KHONG dung --voice (khong ton tai trong 1.6.1).
"""

import subprocess
import sys
from pathlib import Path

import winsound

PROJECT_ROOT = Path(__file__).resolve().parent
PIPER_MODEL = PROJECT_ROOT / "piper1-gpl-main" / "vi_VN-vais1000-medium.onnx"
INPUT_FILE = PROJECT_ROOT / "piper_input.txt"
OUTPUT_FILE = PROJECT_ROOT / "test_voice.wav"

TEXT = "Xin chào, tôi là trợ lý của bạn."

PIPER_TIMEOUT = 120


def run_piper(text: str, output_file: Path) -> int:
    if not PIPER_MODEL.exists():
        print(f"Loi: khong tim thay Piper model: {PIPER_MODEL}")
        sys.exit(1)

    INPUT_FILE.write_text(text, encoding="utf-8")

    command = [
        sys.executable,
        "-m",
        "piper",
        "-m",
        str(PIPER_MODEL),
        "-i",
        str(INPUT_FILE),
        "-f",
        str(output_file),
    ]

    print("Dang tao am thanh bang Piper...")

    try:
        result = subprocess.run(
            command,
            timeout=PIPER_TIMEOUT,
            capture_output=True,
        )
    except subprocess.TimeoutExpired:
        print(f"Loi: Piper timeout sau {PIPER_TIMEOUT} giay.")
        sys.exit(1)

    return result.returncode


def main() -> None:
    returncode = run_piper(TEXT, OUTPUT_FILE)

    if returncode != 0:
        print("Piper bi loi:")
        print("Kiem tra: python -m piper --help")
        print("Goi y: model bi loi, thieu espeak-ng data, hoac CLI sai.")
        sys.exit(1)

    if not OUTPUT_FILE.exists() or OUTPUT_FILE.stat().st_size == 0:
        print("Loi: Piper khong tao duoc WAV hoac WAV rong.")
        sys.exit(1)

    print(f"Da tao: {OUTPUT_FILE} ({OUTPUT_FILE.stat().st_size} bytes)")

    print("Dang phat am thanh...")

    try:
        winsound.PlaySound(str(OUTPUT_FILE), winsound.SND_FILENAME)
    except Exception as e:
        print("Khong phat duoc am thanh:", e)

    print("Hoan tat!")


if __name__ == "__main__":
    main()