"""voice_assistant.py - OFFLINE VOICE ASSISTANT TEST (sandbox doc lap).

    MIC -> Vosk STT -> command_ai.classify() -> AI response -> TTS -> Speaker

Domain = command_ai.py HIEN TAI (dieu hoa / FAN / nhiet do).
KHONG phai classifier xe chinh thuc. KHONG dung ESP32 / UART.

Day la sandbox doc lap de test pipeline:
    Microphone -> Vosk -> command_ai -> Piper/VieNeu TTS -> Speaker
KHONG thay the vosk_test.py (pipeline chinh van la Vosk -> UART -> ESP32-S3 -> TTS).

TTS backend tai su dung tu: ..\\uartpiesppremium\\tts_backend.py (TTSEngine).
KHONG copy lai code Piper/VieNeu.

State machine chong feedback loop:
    LISTENING -> FINAL ASR -> CLASSIFY -> SPEAKING -> LISTENING
Trong luc TTS: MIC = OFF (stream.stop()), queue drain, recognizer.Reset().
KHONG dung sleep() de chong feedback.

Chay:
    python voice_assistant.py [--device N] [--energy-gate] [--tts piper|vieneu]
    python voice_assistant.py --list-devices
"""

import argparse
import json
import queue
import sys
import time
from pathlib import Path

import numpy
import sounddevice as sd
from vosk import Model, KaldiRecognizer

sys.stdout.reconfigure(encoding="utf-8")
sys.stderr.reconfigure(encoding="utf-8")

# =========================================================
# TTS BACKEND REUSE (tts_backend.py o thu muc anh em)
# =========================================================

PROJECT_ROOT = Path(__file__).resolve().parent
TTS_PROJECT = PROJECT_ROOT.parent / "uartpiesppremium"
TTS_BACKEND_FILE = TTS_PROJECT / "tts_backend.py"

if not TTS_BACKEND_FILE.exists():
    print(
        "[ERROR] Khong tim thay TTS backend tai: "
        f"{TTS_BACKEND_FILE}\n"
        "[ERROR] Can thu muc anh em 'uartpiesppremium/' co tts_backend.py."
    )
    sys.exit(1)

sys.path.insert(0, str(TTS_PROJECT))

from tts_backend import TTSEngine, load_config  # noqa: E402

# =========================================================
# CONFIG
# =========================================================

SAMPLE_RATE = 16000
DEFAULT_BLOCKSIZE = 4000

VOSK_MODEL_PATH = PROJECT_ROOT / "vosk-model-small-vn-0.4"

ENERGY_THRESHOLD = 200

# =========================================================
# CLI ARGS
# =========================================================

parser = argparse.ArgumentParser(description="Offline voice assistant test (no ESP32)")
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
    "--energy-gate",
    action="store_true",
    help="Bat energy gate: bo qua block am thanh qua im lang (mac dinh OFF)",
)
parser.add_argument(
    "--tts",
    choices=["piper", "vieneu"],
    default="piper",
    help="Backend TTS (mac dinh: piper)",
)
args = parser.parse_args()

ENERGY_GATE = args.energy_gate


def build_tts_engine(tts_name: str) -> TTSEngine:
    """TTSEngine voi config override (KHONG sua config.json)."""
    cfg = dict(load_config())
    cfg["tts_backend"] = tts_name
    cfg["tts_fallback"] = "none"
    return TTSEngine(cfg)


# =========================================================
# AUDIO
# =========================================================

audio_queue = queue.Queue()


def audio_callback(indata, frames, time_info, status):
    if status:
        print(status, file=sys.stderr)
    if ENERGY_GATE:
        samples = numpy.frombuffer(indata, dtype=numpy.int16).astype(numpy.float32)
        rms = float(numpy.sqrt(numpy.mean(samples * samples)))
        if rms < ENERGY_THRESHOLD:
            return
    audio_queue.put(bytes(indata))


def drain_audio_queue() -> None:
    """Xoa toan bo audio cu trong queue (khong xu ly lai)."""
    while True:
        try:
            audio_queue.get_nowait()
        except queue.Empty:
            break


def resolve_microphone(device_index) -> tuple[int, str]:
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
                f"(channels={d['max_input_channels']}, sr={int(d['default_samplerate'])})"
            )


# =========================================================
# MAIN
# =========================================================

def main() -> int:

    if args.list_devices:
        print_input_devices()
        return 0

    if not VOSK_MODEL_PATH.exists():
        print(f"[ERROR] Khong tim thay Vosk model: {VOSK_MODEL_PATH}")
        return 1

    print("[MIC] Dang tai Vosk...")
    try:
        model = Model(str(VOSK_MODEL_PATH))
    except Exception as e:
        print("[ERROR] Khong load duoc Vosk model:", e)
        return 1
    recognizer = KaldiRecognizer(model, SAMPLE_RATE)

    print("[TTS] Dang khoi tao engine (backend=%s)..." % args.tts)
    try:
        engine = build_tts_engine(args.tts)
    except Exception as e:
        print("[ERROR] Khong khoi tao duoc TTSEngine:", e)
        return 1

    try:
        mic_index, mic_name = resolve_microphone(args.device)
    except Exception as e:
        print("[ERROR] Khong xac dinh duoc microphone:", e)
        print("[ERROR] Chay 'python voice_assistant.py --list-devices' de xem device.")
        return 1

    print()
    print("======================================")
    print("   OFFLINE VOICE ASSISTANT TEST")
    print("   Domain = command_ai.py hien tai")
    print("   (dieu hoa / FAN / nhiet do)")
    print("   KHONG phai classifier xe chinh thuc")
    print("======================================")
    print()
    print("[MIC] Microphone:", mic_name)
    print("[MIC] Energy gate:", "ON" if ENERGY_GATE else "OFF")
    print("[MIC] TTS backend:", engine.primary.name)
    print()
    print("Vi du cau lenh:")
    print("  bat dieu hoa / tat dieu hoa / dat nhiet do 25 do")
    print("  tang nhiet do / giam nhiet do / bat quat / xin chao")
    print("Nhan Ctrl+C de dung.")
    print()

    print("[STATE] LISTENING")
    last_partial = ""

    try:
        with sd.RawInputStream(
            samplerate=SAMPLE_RATE,
            blocksize=DEFAULT_BLOCKSIZE,
            dtype="int16",
            channels=1,
            device=mic_index,
            callback=audio_callback,
        ) as stream:

            while True:

                # Timestamp luc lay chunk khoi queue (diem do ASR latency)
                chunk_received = time.perf_counter()

                data = audio_queue.get()

                if recognizer.AcceptWaveform(data):
                    result = json.loads(recognizer.Result())
                    text = result.get("text", "").strip()
                    final_time = time.perf_counter()
                    asr_ms = (final_time - chunk_received) * 1000.0

                    if text:
                        print()
                        print(f'[ASR] "{text}"')
                        print(f"[PERFORMANCE] ASR last-chunk->final = {asr_ms:.0f} ms")

                        t0 = time.perf_counter()
                        ai = classify(text)
                        clf_ms = (time.perf_counter() - t0) * 1000.0
                        print(f"[CLASSIFIER] intent={ai.intent} confidence={ai.confidence}")
                        print(
                            f"[CLASSIFIER] command_code={ai.command_code} "
                            f"temperature={ai.temperature}"
                        )
                        print(f'[CLASSIFIER] response="{ai.response}"')
                        print(f"[PERFORMANCE] Classifier = {clf_ms:.2f} ms")

                        # ---- MIC OFF truoc TTS (chong feedback) ----
                        stream.stop()
                        drain_audio_queue()
                        recognizer.Reset()

                        print("[STATE] LISTENING -> SPEAKING")
                        print(f"[TTS] Backend={engine.primary.name}")
                        print("[TTS] START")
                        t1 = time.perf_counter()
                        ok = engine.speak(ai.response)
                        tts_ms = (time.perf_counter() - t1) * 1000.0
                        print("[TTS] END")
                        print(f"[PERFORMANCE] TTS wall time = {tts_ms:.0f} ms")
                        e2e_ms = (time.perf_counter() - final_time) * 1000.0
                        print(f"[PERFORMANCE] End-to-end FINAL->TTS_END = {e2e_ms:.0f} ms")
                        print("[PERFORMANCE] user-start->FINAL = NOT MEASURED")

                        if not ok:
                            print("[ERROR] TTS speak() that bai (van tiep tuc nghe)")

                        # ---- MIC ON lai, xoa audio cu ----
                        drain_audio_queue()
                        recognizer.Reset()
                        stream.start()

                        print("[STATE] SPEAKING -> LISTENING")
                        print()
                    else:
                        last_partial = ""
                else:
                    partial = (
                        json.loads(recognizer.PartialResult())
                        .get("partial", "")
                        .strip()
                    )
                    if partial and partial != last_partial:
                        print(f'[ASR] (partial) "{partial}"')
                        last_partial = partial

    except KeyboardInterrupt:
        print()
        print("Dang dung...")
    except sd.PortAudioError as e:
        print("[ERROR] Loi microphone:", e)
        print("[ERROR] Chay 'python voice_assistant.py --list-devices' de chon --device khac.")
        return 1

    return 0


if __name__ == "__main__":
    from command_ai import classify

    sys.exit(main())