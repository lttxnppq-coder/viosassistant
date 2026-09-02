"""Test chat ESP32 qua Serial — protocol thong nhat CMD:/RESP:.

Chay:  python test_esp32.py [--port COMx] [--list-ports]

Nhap:  HELLO / FORWARD / BACKWARD / LEFT / RIGHT / STOP / exit
"""

import argparse
import sys
import time

import serial

sys.stdout.reconfigure(encoding="utf-8")
sys.stderr.reconfigure(encoding="utf-8")

parser = argparse.ArgumentParser(description="Test chat ESP32 qua Serial")
parser.add_argument(
    "--port",
    help="Override auto-detection: COM port cua ESP32 (VD: COM11)",
)
parser.add_argument(
    "--list-ports",
    action="store_true",
    help="In chi tiet toan bo serial ports roi thoat",
)
args = parser.parse_args()

if args.list_ports:
    from esp32_port import print_ports_table

    print_ports_table()
    sys.exit(0)

from esp32_port import find_esp32_port

# KHONG hard-code COM: cong ESP32 duoc tu dong phat hien (esp32_port.py),
# hoac override bang --port.
BAUDRATE = 115200
RESP_TIMEOUT = 5.0

print("Searching for ESP32...")
port = find_esp32_port(forced=args.port)

try:
    ser = serial.Serial(
        port=port,
        baudrate=BAUDRATE,
        timeout=1
    )
except serial.SerialException as e:
    print("COM port is unavailable:", e)
    print("Run --list-ports to inspect available ports.")
    sys.exit(1)

print("Connected to ESP32:", port)

# DTR True->False kích reset ESP32-S3 (rst:0x15) -> chip boot lai -> "ESP32 READY"
# xuat hien sau ~1.1s (evidence: raw_probe/cmd_test). Giu dtr/rts=False de
# write khong bi treo (Pha 1: giu DTR=True + write -> treo vo han).
ser.dtr = False
ser.rts = False

# Đợi ESP32 reset sau khi mở COM; "ESP32 READY" là boot banner (không phải response).
time.sleep(2)

try:
    boot_deadline = time.time() + 5
    while time.time() < boot_deadline:
        if ser.in_waiting:
            line = ser.readline().decode("utf-8", errors="replace").strip()
            if line:
                print("ESP32:", line)
        else:
            time.sleep(0.01)

    while True:

        command = input("Python -> ESP32 (HELLO/FORWARD/...): ").strip()

        if command.lower() == "exit":
            break

        if not command:
            continue

        line = command if command.startswith("CMD:") else "CMD:" + command

        ser.write((line + "\n").encode("utf-8"))

        deadline = time.time() + RESP_TIMEOUT
        got = False

        while time.time() < deadline:
            if ser.in_waiting:
                data = ser.readline().decode("utf-8", errors="replace").strip()
                if data:
                    print("ESP32:", data)
                    got = True
            else:
                time.sleep(0.01)

        if not got:
            print("(no response)")

except serial.SerialException:
    print("ESP32 disconnected. Please reconnect the board and try again.")
    sys.exit(1)

finally:
    ser.close()

print("Disconnected")