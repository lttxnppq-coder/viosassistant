"""Vosk + Command AI + WAV playback pipeline — tro ly giong noi dieu hoa offline:

    MIC -> Vosk STT -> command_ai.classify() -> CODE -> audio/*.wav -> SPEAKER

Khong TTS runtime, khong Piper, khong VieNeu, khong ONNX:
toan bo phan hoi la file WAV da tao san trong audio/ (xem generate_audio_responses.py).

State machine chong feedback loop:
    LISTENING -> PLAYING_AUDIO -> LISTENING
    Trong PLAYING_AUDIO: microphone bi dung (stream.stop()),
    audio queue duoc xoa, recognizer duoc Reset().

ESP32 async (STEP 6):
    classify -> send_async() -> UART_WRITE_OK (feedback ngay, khong cho ACK)
    ACK_OK / ACK_ERROR / ACK_TIMEOUT doc boi ACK listener thread doc lap,
    KHONG block microphone/Vosk/audio. WAV feedback la legacy wording,
    protocol state rieng: UART_WRITE_OK / ACK_OK / ACK_ERROR / ACK_TIMEOUT.

Do performance tung cau:
    [PERF] STT=...ms AI=...ms WAV_START=...ms WAV=...ms

Chay:
    python vosk_test.py [--device N] [--energy-gate]
    python vosk_test.py --list-devices
"""

import argparse
import json
import queue
import sys
import time
import wave
from pathlib import Path

import numpy
import sounddevice as sd
from vosk import Model, KaldiRecognizer

sys.stdout.reconfigure(encoding="utf-8")
sys.stderr.reconfigure(encoding="utf-8")

from command_ai import classify
from command_normalizer import canonicalize
from esp32_sender import Esp32Sender, command_to_uart


# =========================================================
# CONFIG
# =========================================================

PROJECT_ROOT = Path(__file__).resolve().parent

SAMPLE_RATE = 16000
BLOCKSIZE = 800  # 800/16000 = 50ms/chunk (cau hinh test cu, do tre thap)

VOSK_MODEL_PATH = PROJECT_ROOT / "vosk-model-small-vn-0.4"

ENERGY_THRESHOLD = 200

# =========================================================
# CODE -> WAV (file da sinh san bang generate_audio_responses.py)
# =========================================================

CODE_TO_AUDIO = {
    1: "audio/ac_on.wav",
    2: "audio/ac_off.wav",
    4: "audio/temp_up.wav",
    5: "audio/temp_down.wav",
    6: "audio/fan_on.wav",
    7: "audio/fan_off.wav",
    8: "audio/face.wav",
    9: "audio/foot.wav",
    10: "audio/defrost.wav",
    11: "audio/air_auto.wav",
    # SET_TEMPERATURE 18-30: tat ca phat chung 1 WAV xac nhan
    318: "audio/temp_set_success.wav",
    319: "audio/temp_set_success.wav",
    320: "audio/temp_set_success.wav",
    321: "audio/temp_set_success.wav",
    322: "audio/temp_set_success.wav",
    323: "audio/temp_set_success.wav",
    324: "audio/temp_set_success.wav",
    325: "audio/temp_set_success.wav",
    326: "audio/temp_set_success.wav",
    327: "audio/temp_set_success.wav",
    328: "audio/temp_set_success.wav",
    329: "audio/temp_set_success.wav",
    330: "audio/temp_set_success.wav",
    "HELLO": "audio/hello.wav",
    "GOODBYE": "audio/goodbye.wav",
    "INVALID_TEMPERATURE": "audio/invalid_temperature.wav",
    "UNKNOWN": "audio/unknown.wav",
}

# Chi nhung command code thuoc AC contract moi gui toi ESP32.
AC_COMMAND_CODES = {1, 2, 4, 5, 6, 7, 8, 9, 10, 11, *range(318, 331)}


def play_audio(path: str) -> None:
    """Phat file WAV co san (BLOCKING) bang sounddevice - khong TTS runtime.

    WAV -> wave -> numpy int16 -> sounddevice (sd.play + sd.wait).
    Tuong thich Linux/Raspberry Pi (PortAudio).
    """
    wav_path = PROJECT_ROOT / path
    if not wav_path.exists():
        print(f"WAV khong ton tai: {path}, fallback unknown.wav")
        wav_path = PROJECT_ROOT / "audio/unknown.wav"
    try:
        with wave.open(str(wav_path), "rb") as w:
            framerate = w.getframerate()
            nchannels = w.getnchannels()
            sampwidth = w.getsampwidth()
            frames = w.readframes(w.getnframes())
        if not frames:
            print(f"Loi phat WAV {path}: file rong (0 frames)")
            return
        if sampwidth != 2:
            print(f"Loi phat WAV {path}: chi ho tro 16-bit PCM (sampwidth={sampwidth})")
            return
        samples = numpy.frombuffer(frames, dtype=numpy.int16)
        if nchannels > 1:
            samples = samples.reshape(-1, nchannels)
        sd.play(samples, framerate)
        sd.wait()
    except Exception as e:  # noqa: BLE001
        print(f"Loi phat WAV {path}: {e}")


# =========================================================
# CLI ARGS
# =========================================================

parser = argparse.ArgumentParser(description="Vosk STT + WAV playback pipeline")
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
args = parser.parse_args()

ENERGY_GATE = args.energy_gate


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
        print(f"Loi: khong tim thay Vosk model: {VOSK_MODEL_PATH}")
        print("Kiem tra thu muc vosk-model-small-vn-0.4/ ton tai.")
        return 1

    # ----- VOSK -----

    print("Dang tai Vosk...")

    try:
        model = Model(str(VOSK_MODEL_PATH))
    except Exception as e:
        print("Loi: khong load duoc Vosk model:", e)
        return 1

    recognizer = KaldiRecognizer(model, SAMPLE_RATE)

    print("Vosk OK")

    # ----- MICROPHONE -----

    try:
        mic_index, mic_name = resolve_microphone(args.device)
    except Exception as e:
        print("Loi: khong xac dinh duoc microphone:", e)
        print("Chay 'python vosk_test.py --list-devices' de xem cac device.")
        return 1

    print("Using microphone:", mic_name)

    # ----- ESP32 (OPTIONAL) -----

    esp32_sender = Esp32Sender()
    if not esp32_sender.connect():
        esp32_sender = None
        print("ESP32 không kết nối — chỉ chạy local.")
    else:
        esp32_sender.start_ack_listener()

    # ----- STATE MACHINE -----

    print()
    print("======================================")
    print("   VOSK + COMMAND AI + WAV PLAYBACK")
    print("======================================")
    print()
    print("Hay noi vao microphone. Vi du:")
    print("  bat dieu hoa / tat dieu hoa / dat nhiet do 25 do")
    print("  mo quat / tang nhiet do / huong gio len mat / suoi kinh")
    print("Nhan Ctrl+C de dung.")
    print()

    state = "LISTENING"
    audio_path = None
    stt_ms = 0.0
    ai_ms = 0.0
    classify_done_t = 0.0

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

                if state == "LISTENING":

                    chunk_received = time.perf_counter()
                    data = audio_queue.get()

                    if recognizer.AcceptWaveform(data):

                        result = json.loads(recognizer.Result())
                        text = result.get("text", "").strip()

                        if not text:
                            continue

                        stt_ms = (time.perf_counter() - chunk_received) * 1000

                        print()
                        print("STT RAW:", text)

                        ai_start_t = time.perf_counter()
                        ai = classify(text)
                        ai_ms = (time.perf_counter() - ai_start_t) * 1000
                        classify_done_t = time.perf_counter()

                        code = (
                            ai.command_code
                            if ai.command_code is not None
                            else ai.intent
                        )
                        print("NORMALIZED:", canonicalize(ai.normalized_text))
                        print("INTENT:", ai.intent)
                        print("CODE:", code)
                        print("CONFIDENCE:", ai.confidence)
                        if ai.intent == "UNKNOWN":
                            # In text that để bổ sung mapping vào normalizer
                            print("[UNKNOWN] raw text de bo sung normalizer:", repr(text))

                        # ----- ESP32: UART WRITE truoc (async, khong cho ACK) -----
                        # UART_WRITE_OK la moc feedback. ACK_OK/ERROR/TIMEOUT in
                        # tu ACK listener thread — KHONG block voice loop.
                        if (
                            esp32_sender is not None
                            and isinstance(code, int)
                            and code in AC_COMMAND_CODES
                        ):
                            uart_start_t = time.perf_counter()
                            send_res = esp32_sender.send_async(
                                code, temperature=ai.temperature
                            )
                            uart_write_ms = (time.perf_counter() - uart_start_t) * 1000
                            uart_cmd = command_to_uart(
                                code, temperature=ai.temperature
                            )

                            if send_res.state == "SENT":
                                print(f"[UART_WRITE_OK] Đã gửi lệnh: {uart_cmd} "
                                      f"({uart_write_ms:.1f}ms)")
                            elif send_res.state == "QUEUED":
                                print(f"[QUEUED] Lệnh chờ ESP32: {uart_cmd}")
                            elif send_res.state == "COALESCED":
                                print(f"[QUEUED][COALESCED] Đã gộp lệnh trùng: "
                                      f"{uart_cmd}")
                            elif send_res.state == "DROPPED":
                                print(f"[QUEUE_FULL] Lệnh bị bỏ (hàng đợi đầy): "
                                      f"{uart_cmd}")
                            elif send_res.error == "SERIAL_ERROR":
                                print("ESP32 lỗi serial — không gửi lệnh.")
                            else:
                                print("ESP32 không kết nối — chỉ chạy local.")

                            if not send_res.success:
                                drain_audio_queue()
                                recognizer.Reset()
                                continue

                        # STOP microphone -> khong nghe duoc WAV dang phat
                        stream.stop()

                        # Xoa audio cu + reset recognizer truoc khi phat WAV
                        drain_audio_queue()
                        recognizer.Reset()

                        audio_path = CODE_TO_AUDIO.get(code, "audio/unknown.wav")

                        state = "PLAYING_AUDIO"

                elif state == "PLAYING_AUDIO":

                    wav_start_t = time.perf_counter()
                    wav_start_ms = (wav_start_t - classify_done_t) * 1000

                    play_audio(audio_path)

                    wav_ms = (time.perf_counter() - wav_start_t) * 1000
                    print(
                        f"[PERF] STT={stt_ms:.0f}ms AI={ai_ms:.1f}ms "
                        f"WAV_START={wav_start_ms:.1f}ms WAV={wav_ms:.0f}ms"
                    )

                    # ----- ESP32 ACK xu ly doc lap (ACK listener thread) -----
                    # KHONG block: khong wait ACK o day, khong retry.
                    # Xoa audio WAV lot vao mic + reset recognizer
                    drain_audio_queue()
                    recognizer.Reset()

                    stream.start()
                    state = "LISTENING"

                    print()

    except KeyboardInterrupt:

        print()
        print("Dang dung...")

    except sd.PortAudioError as e:

        print("Loi microphone:", e)
        print("Chay 'python vosk_test.py --list-devices' de chon --device khac.")
        return 1

    finally:

        if esp32_sender is not None:
            try:
                esp32_sender.close()
            except Exception:  # noqa: BLE001
                pass

    return 0


if __name__ == "__main__":
    sys.exit(main())