"""cmd_test.py - test protocol ESP32-S3 qua USB CDC (Pha 2E + 2F).

1. Mo COM @115200, dtr=False, rts=False (set sau khi open).
2. Doc cho toi khi gap "ESP32 READY" (toi da 10s).
3. Gui tung lenh CMD:<x>\n, doc raw response (toi da 5s/lenh), in repr().
"""

import argparse
import sys
import time

import serial

sys.stdout.reconfigure(encoding="utf-8", errors="replace")

COMMANDS = ["HELLO", "FORWARD", "BACKWARD", "LEFT", "RIGHT", "STOP", "ABC"]


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", required=True)
    args = ap.parse_args()

    ser = serial.Serial(args.port, 115200, timeout=1)
    ser.dtr = False
    ser.rts = False
    print(f"OPENED {args.port}. dtr={ser.dtr} rts={ser.rts}")

    t0 = time.time()
    ready = False
    while time.time() - t0 < 10:
        n = ser.in_waiting
        if n:
            data = ser.read(n)
            print(f"[{time.time() - t0:6.2f}s] RX {data!r}")
            if b"ESP32 READY" in data:
                ready = True
                break
        else:
            time.sleep(0.05)

    if not ready:
        print("READY NOT RECEIVED IN 10s -> STOP")
        ser.close()
        return

    for cmd in COMMANDS:
        payload = f"CMD:{cmd}\n".encode("utf-8")
        print(f"[SEND] {payload!r}")
        ser.write(payload)
        ser.flush()
        deadline = time.time() + 5
        buf = b""
        while time.time() < deadline:
            n = ser.in_waiting
            if n:
                buf += ser.read(n)
                if buf.endswith(b"\n"):
                    break
            else:
                time.sleep(0.05)
        print(f"[RESP] {buf!r}")
        print(f"       decoded: {buf.decode('utf-8', errors='replace').strip()!r}")

    ser.close()
    print("DONE")


if __name__ == "__main__":
    main()
