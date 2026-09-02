"""Vosk + ESP32 + Piper TTS — pipeline:

    MIC -> Vosk STT -> normalize -> CMD:FORWARD -> ESP32 -> RESP:OK:... -> Piper -> SPEAKER

Protocol:
    Python -> ESP32: CMD:<COMMAND>\n
    ESP32  -> Python: RESP:OK:<COMMAND>:<message>\n | RESP:ERROR:UNKNOWN_COMMAND\n

    "ESP32 READY" chi la BOOT HANDSHAKE — luon luon bo qua.

State machine chong feedback loop:
    LISTENING -> WAITING_RESPONSE -> SPEAKING -> LISTENING
    Trong WAITING_RESPONSE/SPEAKING: microphone bi dung (stream.stop()),
    audio queue duoc xoa, recognizer duoc Reset().

Chay:
    python vosk_test.py [--port COMx] [--device N] [--energy-gate]
    python vosk_test.py --list-ports
    python vosk_test.py --list-devices
"""

import argparse
import json
import queue
import subprocess
import sys
import time
from pathlib import Path

import numpy
import serial
import sounddevice as sd
from vosk import Model, KaldiRecognizer

sys.stdout.reconfigure(encoding="utf-8")
sys.stderr.reconfigure(encoding="utf-8")

from esp32_port import find_esp32_port
from command_normalizer import normalize_command


# =========================================================
# CONFIG
# =========================================================

PROJECT_ROOT = Path(__file__).resolve().parent

BAUDRATE = 115200
SAMPLE_RATE = 16000
BLOCKSIZE = 4000

VOSK_MODEL_PATH = PROJECT_ROOT / "vosk-model-small-vn-0.4"

PIPER_MODEL = PROJECT_ROOT / "piper1-gpl-main" / "vi_VN-vais1000-medium.onnx"
PIPER_INPUT_FILE = PROJECT_ROOT / "piper_input.txt"
RESPONSE_WAV = PROJECT_ROOT / "response.wav"
PIPER_TIMEOUT = 120

RESP_TIMEOUT = 5.0
BOOT_WAIT = 5.0

ENERGY_THRESHOLD = 200


# =========================================================
# CLI ARGS
# =========================================================

parser = argparse.ArgumentParser(description="Vosk + ESP32 + Piper TTS")
parser.add_argument(
    "--port",
    help="Override auto-detection: COM port cua ESP32 (VD: COM11)",
)
parser.add_argument(
    "--list-ports",
    action="store_true",
    help="In chi tiet toan bo serial ports roi thoat",
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
# PIPER TTS
# =========================================================

def speak(text: str) -> bool:
    """Piper: text -> response.wav -> winsound (BLOCKING = SPEAKING state).

    Tra True neu noi thanh cong; False khong crash chuong trinh.
    """

    if not text:
        return False

    print("Piper:", text)

    if not PIPER_MODEL.exists():
        print(f"TTS ERROR: khong tim thay Piper model: {PIPER_MODEL}")
        return False

    try:
        PIPER_INPUT_FILE.write_text(text, encoding="utf-8")
    except OSError as e:
        print("TTS ERROR: khong ghi duoc input file:", e)
        return False

    command = [
        sys.executable,
        "-m",
        "piper",
        "-m",
        str(PIPER_MODEL),
        "-i",
        str(PIPER_INPUT_FILE),
        "-f",
        str(RESPONSE_WAV),
    ]

    try:
        result = subprocess.run(
            command,
            timeout=PIPER_TIMEOUT,
            capture_output=True,
        )
    except subprocess.TimeoutExpired:
        print(f"TTS ERROR: Piper timeout sau {PIPER_TIMEOUT} giay.")
        return False

    if result.returncode != 0:
        print("Piper ERROR:")
        print(result.stderr.decode("utf-8", errors="replace"))
        return False

    if not RESPONSE_WAV.exists() or RESPONSE_WAV.stat().st_size == 0:
        print("TTS ERROR: WAV khong duoc tao hoac rong.")
        return False

    try:
        import winsound

        winsound.PlaySound(str(RESPONSE_WAV), winsound.SND_FILENAME)
    except Exception as e:
        print("TTS ERROR: khong phat duoc am thanh:", e)
        return False

    return True


# =========================================================
# RESPONSE PARSER (protocol RESP:OK:... / RESP:ERROR:...)
# =========================================================

def parse_resp(line: str):
    """Phan tich response tu ESP32.

    RESP:OK:FORWARD:Đã tiến lên.     -> ("OK", "FORWARD", "Đã tiến lên.")
    RESP:ERROR:UNKNOWN_COMMAND       -> ("ERROR", None, "UNKNOWN_COMMAND")
    "ESP32 READY" / rac / empty      -> None (bo qua)
    """

    line = line.strip()

    if not line.startswith("RESP:"):
        return None

    parts = line.split(":", 3)

    if len(parts) >= 3 and parts[1] == "ERROR":
        return ("ERROR", None, parts[2].strip())

    if len(parts) == 4 and parts[1] == "OK":
        return ("OK", parts[2].strip(), parts[3].strip())

    return None


def read_esp32_response(ser, timeout: float = RESP_TIMEOUT):
    """Doc response cho toi khi gap RESP:... hoac het timeout.

    BO QUA: ESP32 READY, cac dong garbage (khong phai RESP:).
    Khong gui lai command. Tra None neu het timeout.
    """

    start = time.time()

    while time.time() - start < timeout:

        if ser.in_waiting:

            line = (
                ser.readline()
                .decode("utf-8", errors="replace")
                .strip()
            )

            if line:
                print("ESP32:", line)

                parsed = parse_resp(line)
                if parsed is not None:
                    return parsed

        time.sleep(0.01)

    return None


# =========================================================
# MAIN
# =========================================================

def main() -> int:

    if args.list_devices:
        print_input_devices()
        return 0

    if args.list_ports:
        from esp32_port import print_ports_table

        print_ports_table()
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

    # ----- COM AUTO-DETECTION + HANDSHAKE -----

    print("Dang tim ESP32...")

    port = find_esp32_port(forced=args.port)

    try:
        ser = serial.Serial(port, BAUDRATE, timeout=1)
    except serial.SerialException as e:
        print("COM port is unavailable:", e)
        print("Run --list-ports to inspect available ports.")
        return 1

    print("Da ket noi:", port)

    # Mở COM có thể reset ESP32 -> chờ banner boot lần nữa (chỉ để xác nhận,
    # "ESP32 READY" KHÔNG phải response command).
    try:
        boot_deadline = time.time() + BOOT_WAIT
        verified = False
        while time.time() < boot_deadline:
            if ser.in_waiting:
                line = ser.readline().decode("utf-8", errors="replace").strip()
                if line:
                    print("ESP32:", line)
                    if "ESP32 READY" in line:
                        verified = True
                        break
            time.sleep(0.01)
        if not verified:
            print("Canh bao: khong nhan duoc 'ESP32 READY' sau khi mo COM.")
    except serial.SerialException:
        print("ESP32 disconnected. Please reconnect the board and try again.")
        ser.close()
        return 1

    # ----- MICROPHONE -----

    try:
        mic_index, mic_name = resolve_microphone(args.device)
    except Exception as e:
        print("Loi: khong xac dinh duoc microphone:", e)
        print("Chay 'python vosk_test.py --list-devices' de xem cac device.")
        ser.close()
        return 1

    print("Using microphone:", mic_name)

    # ----- STATE MACHINE -----

    print()
    print("======================================")
    print("   VOSK + ESP32 + PIPER TTS")
    print("======================================")
    print()
    print("Hay noi vao microphone. Vi du:")
    print("  xin chao / tien len / lui lai / re trai / re phai / dung lai")
    print("Nhan Ctrl+C de dung.")
    print()

    state = "LISTENING"
    speak_text = None

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

                    data = audio_queue.get()

                    if recognizer.AcceptWaveform(data):

                        result = json.loads(recognizer.Result())
                        text = result.get("text", "").strip()

                        if not text:
                            continue

                        print()
                        print("Ban noi:", text)

                        command = normalize_command(text)

                        if command is None:
                            print("Khong hieu lenh:", text)
                            continue

                        print("Command:", command)

                        # STOP microphone -> khong nghe duoc TTS sau nay
                        stream.stop()

                        # Xoa audio cu + reset recognizer truoc khi gui lenh
                        drain_audio_queue()
                        recognizer.Reset()

                        try:
                            ser.write(("CMD:" + command + "\n").encode("utf-8"))
                        except serial.SerialException:
                            print("ESP32 disconnected. Please reconnect the board and try again.")
                            return 1

                        print("Da gui ESP32: CMD:" + command)

                        state = "WAITING_RESPONSE"

                elif state == "WAITING_RESPONSE":

                    try:
                        response = read_esp32_response(ser, timeout=RESP_TIMEOUT)
                    except serial.SerialException:
                        print("ESP32 disconnected. Please reconnect the board and try again.")
                        return 1

                    if response is None:
                        print("ESP32 khong phan hoi.")
                        speak_text = None
                    elif response[0] == "ERROR":
                        print("ESP32 ERROR:", response[2])
                        speak_text = None
                    else:
                        print("ESP32 response:", response[1], "->", response[2])
                        speak_text = response[2]

                    if speak_text:
                        state = "SPEAKING"
                    else:
                        drain_audio_queue()
                        recognizer.Reset()
                        stream.start()
                        state = "LISTENING"

                elif state == "SPEAKING":

                    speak(speak_text)

                    # Xoa audio TTS lot vao mic + reset recognizer
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

        ser.close()
        print("Da dong COM.")

    return 0


if __name__ == "__main__":
    sys.exit(main())