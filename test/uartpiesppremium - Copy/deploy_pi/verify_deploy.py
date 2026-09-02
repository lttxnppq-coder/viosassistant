"""verify_deploy.py - Kiem tra tinh hop le cua deploy package Raspberry Pi.

Khong chay Vosk/Piper inference. Chi kiem tra cau truc file.

Chay:  python3 verify_deploy.py
"""

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent

RUNTIME_FILES = [
    "main.py",
    "command_ai.py",
    "command_normalizer.py",
    "esp32_sender.py",
    "esp32_port.py",
    "config.json",
    "requirements-runtime.txt",
]

VOSK_MODEL_DIR = ROOT / "vosk-model-small-vn-0.4"
VOSK_REQUIRED_FILES = [
    "README",
    "am/final.mdl",
    "conf/mfcc.conf",
    "conf/model.conf",
    "graph/disambig_tid.int",
    "graph/Gr.fst",
    "graph/HCLr.fst",
    "graph/phones/word_boundary.int",
    "ivector/final.dubm",
    "ivector/final.ie",
    "ivector/final.mat",
    "ivector/global_cmvn.stats",
    "ivector/online_cmvn.conf",
    "ivector/splice.conf",
]

PIPER_ONNX = ROOT / "piper1-gpl-main" / "vi_VN-vais1000-medium.onnx"
PIPER_JSON = ROOT / "piper1-gpl-main" / "vi_VN-vais1000-medium.onnx.json"

FORBIDDEN = [
    ("audio", "thu muc WAV legacy"),
    ("vosk_test.py", "dev/test"),
    ("generate_audio_responses.py", "dev"),
    ("tts_backend.py", "legacy"),
    ("piper_test.py", "test"),
    ("tts_benchmark.py", "benchmark"),
    ("speech_test.py", "test"),
    ("cmd_test.py", "dev"),
    ("test.py", "test"),
    ("Uartpiessp", "firmware ESP32"),
    (".pio", "PlatformIO build"),
    (".venv", "virtualenv"),
    (".vscode", "IDE"),
    (".git", "git metadata"),
    (".gitignore", "git config"),
    ("workspace.code-workspace", "IDE file"),
    ("ui_test", "UI dev"),
]

passes = 0
fails = 0


def check(ok: bool, label: str, detail: str = "") -> None:
    global passes, fails
    if ok:
        passes += 1
        print(f"[OK] {label}")
    else:
        fails += 1
        print(f"[FAIL] {label} {detail}")


def main() -> int:
    print(f"Kiem tra deploy package: {ROOT}")
    print()

    for name in RUNTIME_FILES:
        check((ROOT / name).is_file(), name)

    check(VOSK_MODEL_DIR.is_dir(), "vosk-model-small-vn-0.4/ ton tai")
    if VOSK_MODEL_DIR.is_dir():
        for rel in VOSK_REQUIRED_FILES:
            check((VOSK_MODEL_DIR / rel).is_file(), f"vosk/{rel}")
        extra = [p for p in VOSK_MODEL_DIR.rglob("*") if p.is_file()]
        check(
            len(extra) == len(VOSK_REQUIRED_FILES),
            "vosk model khong thua/thieu file",
            f"(found {len(extra)}, expected {len(VOSK_REQUIRED_FILES)})",
        )

    check(PIPER_ONNX.is_file(), "piper .onnx")
    check(PIPER_JSON.is_file(), "piper .onnx.json")
    if PIPER_ONNX.is_file():
        check(PIPER_ONNX.stat().st_size > 50_000_000, "piper .onnx kich thuoc hop ly")

    wavs = list(ROOT.rglob("*.wav"))
    check(len(wavs) == 0, "khong co file .wav")

    pyc = list(ROOT.rglob("*.pyc")) + list(ROOT.rglob("__pycache__"))
    check(len(pyc) == 0, "khong co __pycache__ / *.pyc")

    test_files = [p for p in ROOT.rglob("*") if p.name.startswith("test_")]
    check(len(test_files) == 0, "khong co test_*.py")

    for name, reason in FORBIDDEN:
        check(not (ROOT / name).exists(), f"khong co {name}", f"({reason})")

    mic_files = [p for p in ROOT.glob("mic_*.py")]
    check(len(mic_files) == 0, "khong co mic_*.py")

    print()
    if fails == 0:
        print(f"DEPLOY PACKAGE: VALID ({passes} checks passed)")
        return 0
    print(f"DEPLOY PACKAGE: INVALID ({fails} checks failed, {passes} passed)")
    return 1


if __name__ == "__main__":
    sys.exit(main())