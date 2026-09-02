"""raw_probe.py - diagnostics read-only cho ESP32-S3 COM10 (Pha 1).

Modes:
  read : mo COM @115200, KHONG gui byte nao, doc raw trong `seconds` giay
  hello: mo COM @115200, gui DUNG MOT LAN b"CMD:HELLO\\n", doc toi da 5s

--ctl none    : dtr=False, rts=False (han che reset do DTR/RTS)
--ctl default : pyserial mac dinh (dtr=None, rts=None -> Windows assert ca hai)

Script chi doc/ghi raw, khong reset, khong dong/mo lien tuc.
In repr(data) de khong loi encoding, timestamp tu luc mo port.
"""

import argparse
import sys
import time

import serial
import serial.tools.list_ports

sys.stdout.reconfigure(encoding="utf-8", errors="replace")


def list_ports(tag: str) -> None:
    print(f"=== [PORTS {tag}] ===")
    for p in serial.tools.list_ports.comports():
        if p.vid:
            vid_pid = f"VID={p.vid:04X} PID={p.pid:04X}"
        else:
            vid_pid = "VID=None PID=None"
        print(f"{p.device} | {p.description} | {p.hwid} | {vid_pid} | SN={p.serial_number}")
    print("=== [END PORTS] ===")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", required=True, help="COM port (VD: COM10)")
    ap.add_argument("--mode", choices=["read", "hello"], required=True)
    ap.add_argument("--seconds", type=float, default=15.0, help="thoi gian doc (giay)")
    ap.add_argument("--ctl", choices=["none", "default"], default="none")
    ap.add_argument("--delay-write", type=float, default=0.0, help="cho N giay sau khi mo truoc khi write (giay)")
    args = ap.parse_args()

    list_ports("BEFORE OPEN")

    kwargs: dict = {"port": args.port, "baudrate": 115200, "timeout": 1}
    print(f"OPEN {args.port} baud=115200 ctl={args.ctl} mode={args.mode}")
    try:
        ser = serial.Serial(**kwargs)
    except serial.SerialException as e:
        print(f"OPEN FAILED: {e!r}")
        return
    print(f"OPENED (defaults). dtr={ser.dtr} rts={ser.rts}")
    if args.ctl == "none":
        try:
            ser.dtr = False
            ser.rts = False
        except serial.SerialException as e:
            print(f"SET dtr/rts=False EXCEPTION: {e!r}")
        print(f"AFTER SET. dtr={ser.dtr} rts={ser.rts}")

    t0 = time.time()

    if args.delay_write > 0:
        print(f"[{0:7.2f}s] WAIT {args.delay_write}s before write...")
        while time.time() - t0 < args.delay_write:
            try:
                n = ser.in_waiting
            except serial.SerialException as e:
                print(f"[{time.time() - t0:7.2f}s] SERIALEXCEPTION in_waiting: {e!r}")
                break
            if n:
                try:
                    data = ser.read(n)
                except serial.SerialException as e:
                    print(f"[{time.time() - t0:7.2f}s] SERIALEXCEPTION read: {e!r}")
                    break
                print(f"[{time.time() - t0:7.2f}s] RX {data!r}")
            else:
                time.sleep(0.05)

    if args.mode == "hello":
        payload = b"CMD:HELLO\n"
        print(f"[{0:7.2f}s] WRITE {payload!r}")
        try:
            ser.write(payload)
            ser.flush()
        except serial.SerialException as e:
            print(f"[{0:7.2f}s] WRITE EXCEPTION: {e!r}")

    deadline = t0 + args.seconds
    try:
        while time.time() < deadline:
            try:
                n = ser.in_waiting
            except serial.SerialException as e:
                print(f"[{time.time() - t0:7.2f}s] SERIALEXCEPTION in_waiting: {e!r}")
                break
            if n:
                try:
                    data = ser.read(n)
                except serial.SerialException as e:
                    print(f"[{time.time() - t0:7.2f}s] SERIALEXCEPTION read: {e!r}")
                    break
                print(f"[{time.time() - t0:7.2f}s] RX {data!r}")
            else:
                time.sleep(0.05)
    except KeyboardInterrupt:
        print("INTERRUPTED")
    finally:
        try:
            ser.close()
        except serial.SerialException as e:
            print(f"CLOSE EXCEPTION: {e!r}")
        print(f"CLOSED after {time.time() - t0:.2f}s")
        list_ports("AFTER CLOSE")


if __name__ == "__main__":
    main()
