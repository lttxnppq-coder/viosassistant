"""speech_test.py - PHAN DO/TOL UU STT (KHONG ESP32, KHONG AI, KHONG Piper trong vong lap).

Pipeline:
    MIC -> sounddevice -> audio_queue -> Vosk -> [PARTIAL] / [FINAL] + [LATENCY]

CHI soa:
    python speech_test.py [--blocksize 800|1600|4000] [--device N] [--list-devices]

Dinh nghia LATENCY (trung thuc):
    Thoi gian tu luc chunk audio cuoi cung duoc lay khoi queue den luc Vosk
    tra ve FINAL. KHONG phai latency tong tu luc nguoi dung bat dau noi.
    Khong tu tao so do mic->queue (chua co timestamp tuong ung).

Chay:
    python speech_test.py --blocksize 800
"""

import argparse
import json
import queue
import sys
import time
from pathlib import Path

import sounddevice as sd
from vosk import Model, KaldiRecognizer

from command_ai import classify

sys.stdout.reconfigure(encoding="utf-8")
sys.stderr.reconfigure(encoding="utf-8")

# =========================================================
# CONFIG (KHONG doi: model, sample rate, dtype, channels)
# =========================================================

PROJECT_ROOT = Path(__file__).resolve().parent

SAMPLE_RATE = 16000
DEFAULT_BLOCKSIZE = 800

VOSK_MODEL_PATH = PROJECT_ROOT / "vosk-model-small-vn-0.4"

BACKLOG_THRESHOLD = 8  # canh bao khi queue vuot ~8 chunk

# =========================================================
# CLI ARGS
# =========================================================

parser = argparse.ArgumentParser(
    description="Speech recognition test (MIC -> Vosk -> PARTIAL/FINAL text)"
)
parser.add_argument(
    "--list-devices",
    action="store_true",
    help="In danh sach microphone/audio devices roi thoat",
)
parser.add_argument(
    "--device",
    type=int,
    default=None,
    help="Chi dinh microphone device index (xem --list-devices)",
)
parser.add_argument(
    "--blocksize",
    type=int,
    default=DEFAULT_BLOCKSIZE,
    help="So samples/chunk (default 800 = 50ms @16k). Test: 800/1600/4000",
)
args = parser.parse_args()

BLOCKSIZE = args.blocksize
CHUNK_MS = BLOCKSIZE / SAMPLE_RATE * 1000

# =========================================================
# MICROPHONE
# =========================================================

audio_queue = queue.Queue()
_last_backlog = 0


def audio_callback(indata, frames, time_info, status):
    if status:
        print(status, file=sys.stderr)
    audio_queue.put(bytes(indata))
    # Theo doi backlog: CHI canh bao khi qsize tang, khong spam.
    # TUYET DOI khong drain queue khi nguoi dung dang noi.
    global _last_backlog
    qsize = audio_queue.qsize()
    if qsize > BACKLOG_THRESHOLD and qsize > _last_backlog:
        print(f"[WARNING] AUDIO BACKLOG: {qsize} chunks")
        _last_backlog = qsize


def resolve_microphone(device_index) -> tuple[int, str]:
    """Xac dinh microphone: theo --device hoac default; tra (index, name)."""
    if device_index is not None:
        info = sd.query_devices(device=device_index, kind="input")
        return device_index, info["name"]

    default = sd.default.device
    if isinstance(default, (tuple, list)):
        default = default[0]
    info = sd.query_devices(device=default, kind="input")
    return default, info["name"]


def print_input_devices() -> None:
    print("Input devices:")
    devices = sd.query_devices()
    for i, d in enumerate(devices):
        if d["max_input_channels"] > 0:
            print(
                f"[{i}] {d['name']} "
                f"(channels={d['max_input_channels']}, "
                f"sr={int(d['default_samplerate'])}, "
                f"low_latency={d['default_low_input_latency']}, "
                f"high_latency={d['default_high_input_latency']})"
            )


# =========================================================
# PIPER (GIU ham nhung KHONG GOI trong pha test nay)
# =========================================================

def speak_with_piper(text: str) -> bool:
    """Khong duoc goi trong pha test nay. Chi giu lai cho cac pha sau."""
    return False


# =========================================================
# MAIN LOOP
# =========================================================

def main() -> int:

    if args.list_devices:
        print_input_devices()
        return 0

    if not VOSK_MODEL_PATH.exists():
        print(f"Loi: khong tim thay Vosk model: {VOSK_MODEL_PATH}")
        print("Kiem tra thu muc vosk-model-small-vn-0.4/ ton tai.")
        return 1

    print("Dang tai Vosk...")

    try:
        model = Model(str(VOSK_MODEL_PATH))
    except Exception as e:
        print("Loi: khong load duoc Vosk model:", e)
        return 1

    recognizer = KaldiRecognizer(model, SAMPLE_RATE)

    print("Vosk OK")

    try:
        mic_index, mic_name = resolve_microphone(args.device)
    except Exception as e:
        print("Loi: khong xac dinh duoc microphone:", e)
        print("Chay 'python speech_test.py --list-devices' de xem cac device.")
        return 1

    print()
    print("========================================")
    print("   SPEECH RECOGNITION TEST")
    print("========================================")
    print()
    print("Model:", VOSK_MODEL_PATH.name)
    print("Sample rate:", SAMPLE_RATE, "Hz")
    print("Block size:", BLOCKSIZE)
    print(f"Chunk duration: {CHUNK_MS:g} ms")
    print("Microphone:", mic_name)
    print("Status: LISTENING")
    print("========================================")
    print()
    print("Đang nghe...")
    print("Nói thử một câu.")
    print()

    last_partial = ""

    try:
        with sd.RawInputStream(
            samplerate=SAMPLE_RATE,
            blocksize=BLOCKSIZE,
            dtype="int16",
            channels=1,
            device=mic_index,
            callback=audio_callback,
        ) as stream:

            while True:

                # Timestamp luc lay chunk khoi queue (dau diem do LATENCY)
                chunk_received = time.perf_counter()

                data = audio_queue.get()

                # Do thoi gian Vosk xu ly chunk
                chunk_start = time.perf_counter()
                ok = recognizer.AcceptWaveform(data)
                processing_ms = (time.perf_counter() - chunk_start) * 1000

                # Canh bao neu Vosk xu ly khong kip realtime
                if processing_ms > CHUNK_MS:
                    print(f"[WARNING] VOSK SLOW: processing={processing_ms:.1f} ms chunk={CHUNK_MS:g} ms")

                if ok:
                    # FINAL: in DUNG text Vosk tra ve, khong sua/khong normalize
                    result = json.loads(recognizer.Result())
                    text = result.get("text", "").strip()

                    final_time = time.perf_counter()
                    latency_ms = (final_time - chunk_received) * 1000
                    # LATENCY = chunk cuoi lay khoi queue -> Vosk tra FINAL.
                    # KHONG phai tong latency tu luc bat dau noi.

                    if text:
                        print(f"[FINAL] {text}")
                        print(f"[LATENCY] {latency_ms:.0f} ms")

                        # Rule-based classifier (command_ai.py): CHI phan loai sau FINAL,
                        # khong sua [FINAL], khong gui ESP32/UART, khong goi Piper.
                        ai = classify(text)
                        print("[AI]")
                        print("intent:", ai.intent)
                        if ai.temperature is not None:
                            print("temperature:", ai.temperature)
                        print("confidence:", ai.confidence)
                        print(
                            "command:",
                            ai.command_code if ai.command_code is not None else "NONE",
                        )
                        print("response:", ai.response)

                    last_partial = ""  # reset cho cau moi, khong noi voi cau cu

                else:
                    partial = (
                        json.loads(recognizer.PartialResult())
                        .get("partial", "")
                        .strip()
                    )
                    # CHI in khi partial thay doi (khong spam, khong sua text)
                    if partial and partial != last_partial:
                        print(f"[PARTIAL] {partial}")
                        last_partial = partial

    except KeyboardInterrupt:
        print()
        print("Đã dừng nhận diện giọng nói.")
    except sd.PortAudioError as e:
        print("Loi microphone:", e)
        print("Chay 'python speech_test.py --list-devices' de chon --device khac.")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())