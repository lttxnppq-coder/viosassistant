"""generate_audio_responses.py - sinh WAV phan hoi bang Piper (TTS cu cua project).

TTS cu: piper-tts (pip) + model vi_VN-vais1000-medium.onnx trong piper1-gpl-main/.
Model duoc tai su dung tu project cu (uartpiesppre / uartpiesppremiumvippro).

Chay:  python generate_audio_responses.py

Chi sinh cac WAV MOI can thiet (3 file). Cac WAV cu (ac_on, temp23-30, ...)
KHONG bi xoa, chi bao UNUSED neu runtime khong con dung.

KHONG phat am thanh ra loa, chi luu file.
"""

import subprocess
import sys
import tempfile
import wave
from pathlib import Path

sys.stdout.reconfigure(encoding="utf-8")
sys.stderr.reconfigure(encoding="utf-8")

PROJECT_ROOT = Path(__file__).resolve().parent

PIPER_MODEL = PROJECT_ROOT / "piper1-gpl-main" / "vi_VN-vais1000-medium.onnx"
PIPER_INPUT_FILE = Path(tempfile.gettempdir()) / "piper_input.txt"
PIPER_TIMEOUT = 120

AUDIO_DIR = PROJECT_ROOT / "audio"

# Chi 5 WAV moi, thay the cac cau cu:
# - tat ca SET_TEMPERATURE (18-30) dung chung temp_set_success.wav
# - INVALID_TEMPERATURE dung invalid_temperature.wav (khong roi vao unknown.wav)
# - HELLO dung hello.wav (khong roi vao unknown.wav)
# - GOODBYE dung goodbye.wav (intent moi)
# - AIR_AUTO dung air_auto.wav (intent moi, code 11)
RESPONSES: list[tuple[str, str]] = [
    ("temp_set_success.wav", "Điều hoà đã đặt theo yêu cầu của bạn."),
    ("invalid_temperature.wav", "Nhiệt độ cho phép từ 18 đến 30 độ."),
    ("hello.wav", "Xin chào! Tôi đang hoạt động."),
    ("goodbye.wav", "Tạm biệt! Hẹn gặp lại."),
    ("air_auto.wav", "Đã chuyển sang chế độ gió tự động."),
]


def validate_wav(path: Path) -> str | None:
    """Kiem tra WAV dung format runtime: PCM 22050 Hz / 16-bit / mono, khong rong."""
    try:
        with wave.open(str(path), "rb") as w:
            if w.getframerate() != 22050:
                return f"sai sample rate: {w.getframerate()}"
            if w.getnchannels() != 1:
                return f"sai channels: {w.getnchannels()}"
            if w.getsampwidth() != 2:
                return f"sai bit depth: {w.getsampwidth() * 8}"
            if w.getnframes() == 0:
                return "file rong (0 frames)"
    except wave.Error as e:
        return f"khong doc duoc WAV: {e}"
    return None


def generate_one(name: str, text: str) -> str | None:
    """Sinh 1 WAV bang Piper. Tra None neu OK, nguoc lai chuoi loi."""
    try:
        PIPER_INPUT_FILE.write_text(text, encoding="utf-8")
    except OSError as e:
        return f"khong ghi duoc input: {e}"

    out = AUDIO_DIR / name
    command = [
        sys.executable, "-m", "piper",
        "-m", str(PIPER_MODEL),
        "-i", str(PIPER_INPUT_FILE),
        "-f", str(out),
    ]
    try:
        result = subprocess.run(command, timeout=PIPER_TIMEOUT, capture_output=True)
    except subprocess.TimeoutExpired:
        return f"timeout sau {PIPER_TIMEOUT}s"
    except Exception as e:  # noqa: BLE001
        return f"subprocess error: {e}"

    if result.returncode != 0:
        return result.stderr.decode("utf-8", errors="replace")[:300]
    if not out.exists() or out.stat().st_size == 0:
        return "WAV khong duoc tao hoac rong"
    return validate_wav(out)


def main() -> int:
    if not PIPER_MODEL.exists():
        print(f"[ERROR] Khong tim thay model Piper: {PIPER_MODEL}")
        print("Model cu bi xoa khoi project nay; sao chep lai tu project cu:")
        print("  uartpiesppre/piper1-gpl-main/vi_VN-vais1000-medium.onnx")
        return 1

    AUDIO_DIR.mkdir(parents=True, exist_ok=True)

    failed = 0
    for name, text in RESPONSES:
        print(f"Generating: {name}")
        err = generate_one(name, text)
        if err is not None:
            failed += 1
            print(f"[ERROR] {name}: {err}")

    total = len(RESPONSES)
    if failed == 0:
        print(f"\nGenerated {total} audio files successfully (all validated).")
        return 0
    print(f"\n{total - failed}/{total} generated, {failed} failed.")
    return 1


if __name__ == "__main__":
    sys.exit(main())