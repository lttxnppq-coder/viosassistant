# Smart AC - Raspberry Pi 32-bit Runtime

Runtime nhe cho Raspberry Pi 32-bit (armv7l, Python 3.11):
**Vosk ASR + Command AI + WAV playback (sounddevice) + UART -> ESP32-S3.**

KHONG dung Piper / ONNX / VieNeu / TTS.

## 1. Yeu cau he thong

- Raspberry Pi OS 32-bit (armhf, armv7l), Python 3.11
- ESP32-S3 dang chay firmware `Uartpiessp` (platformio.ini, env `esp32-s3`)
- Micro USB hoac bo mic USB

## 2. Cac buoc cai dat

```bash
sudo apt update
sudo apt install -y python3.11 python3.11-venv portaudio19-dev
python3.11 -m venv venv
source venv/bin/activate
pip install --upgrade pip
pip install -r requirements-runtime.txt
```

`requirements-runtime.txt` (chinh xac cho armhf):
```
vosk==0.3.45
sounddevice==0.5.5
pyserial>=3.5
numpy==1.26.4
```

> numpy==1.26.4 la ban cuoi cung co wheel armv7l cho Python 3.11.
> numpy 2.x KHONG co wheel 32-bit ARM (se tu bien dich, rat lau/co the fail).
> Vosk tu dong tai model tu internet khi chay dau tien NEU chua co model:
> chay `vosk_test.py` tai thu muc model -> neu khong co, se hoi tai
> `vosk-model-small-vn-0.4` tu https://alphacephei.com/vosk/models.

## 3. Cau truc package

```
smart-ac-pi-32bit/
├── vosk_test.py              # entry point (mic + ASR + command + WAV + UART)
├── command_ai.py             # nhan dien y dinh tieng Viet
├── command_normalizer.py     # normalize tieng Viet (bo dau)
├── esp32_sender.py           # UART sender + protocol
├── esp32_port.py             # auto-detect UART (dong bo cache config.json)
├── config.json               # cache last_port/hwid (tu dong invalidate)
├── requirements-runtime.txt
├── audio/                    # 23 file WAV 22050Hz/16-bit/mono
└── vosk-model-small-vn-0.4/  # model Vosk tieng Viet (14 file)
```

## 4. UART protocol (khong doi - source of truth)

PC -> ESP32 (115200 baud, `\n` terminated):
```
CMD:AC_ON
CMD:AC_OFF
CMD:TEMP_UP
CMD:TEMP_DOWN
CMD:FAN_ON
CMD:FAN_OFF
CMD:AIR_FACE
CMD:AIR_FOOT
CMD:AIR_DEFROST
CMD:AIR_AUTO
CMD:SET_TEMP:<18..30>
```

ESP32 -> PC:
```
RESP:OK:<CMD>
RESP:OK:SET_TEMP:<temp>
RESP:ERROR:<reason>
```
Boot banner `ESP32 READY` dung de phat hien port. Chi duoc gui temp 18-30;
17/31 -> INVALID_TEMPERATURE (khong gui ESP32).

## 5. Chay

```bash
source venv/bin/activate
python vosk_test.py
```

- Noi dung mic 16kHz mono (Vosk yeu cau), SDL/PortAudio qua sounddevice
- WAV playback qua `sd.play/sd.wait` (khong can aplay/Piper)
- Nhap tay `mic` de test mic, `uart` de test ket noi ESP32, `quit` de thoat

## 6. Ghi chu

- Khong can config TTS: WAV da duoc sinh san (23 file).
- `generate_audio_responses.py` (chi tren PC, can Piper) dung de sinh lai WAV neu doi noi dung.
- Neu OLED khong nhan: kiem tra `Uartpiessp` (OLED 0x3C/0x3D, SDA=41, SCL=42).