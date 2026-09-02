# Smart AC — Raspberry Pi Deploy Package

Pipeline: MIC -> Vosk STT -> command_ai -> UART ESP32 -> Piper TTS (in-process) -> Speaker

Không cần chuyển toàn bộ project Windows sang Pi. Package nay chi chua runtime toi thieu.

## Cau truc

```
deploy_pi/
├── main.py
├── command_ai.py
├── command_normalizer.py
├── esp32_sender.py
├── esp32_port.py
├── config.json
├── requirements-runtime.txt
├── verify_deploy.py
├── README_PI.md
├── vosk-model-small-vn-0.4/     (14 file, bat buoc du)
└── piper1-gpl-main/
    ├── vi_VN-vais1000-medium.onnx
    └── vi_VN-vais1000-medium.onnx.json
```

## A. Cai dependency he thong

```bash
sudo apt update
sudo apt install -y python3-venv libportaudio2
```

## B. Quyen UART

```bash
sudo usermod -aG dialout $USER
```

Sau do logout/login hoac reboot de quyen co hieu luc.
`/dev/ttyACM0` (ESP32-S3 native USB) se duoc auto-detect, khong can sua COM thu cong.

## C. Tao virtual environment

```bash
cd deploy_pi
python3 -m venv venv
source venv/bin/activate
```

## D. Cai Python dependencies

```bash
pip install -r requirements-runtime.txt
```

- `piper-tts` tu dong cai kèm `onnxruntime` + `espeak-ng-data` (khong can copy).
- `vosk` tu dong cai kèm cffi/requests/srt/tqdm/websockets.

## E. Kiem tra microphone

```bash
python3 main.py --list-devices
```

Ghi nhan device index cua micro (vi du `[1]`) neu khong phai default.

## F. Chay

```bash
python3 main.py                 # dung micro default
python3 main.py --device 1      # chi dinh micro theo index
```

Neu ESP32 khong cam: chuong trinh chay local (khong gui UART).

## G. Kiem tra package truoc khi chay

```bash
python3 verify_deploy.py
```

Ky vong: `DEPLOY PACKAGE: VALID`

## Ghi chu

- Khong xoa bat ky file nao trong `vosk-model-small-vn-0.4/` — Vosk can du 14 file.
- `config.json` chua cache `last_port` cu (Windows: COM10) — tu dong invalidate va auto-detect tren Pi.
- RAM: model Vosk + Piper + ONNX Runtime duoc tai vao bo nho. Khuyen nghi Pi 4 (>= 2 GB RAM).