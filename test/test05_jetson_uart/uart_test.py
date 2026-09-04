#!/usr/bin/env python3
"""
uart_test.py — Jetson side real UART test for ESP32 9600 ASCII ACK
Usage: python3 uart_test.py --device /dev/ttyTHS1 --baud 9600 --cmd 101
Or:   python3 uart_test.py --device /dev/ttyUSB0 --baud 9600

Sends: "101\r"
Expects: "ACK 101\r" within 1s
Prints raw TX/RX and PASS/FAIL
"""

import argparse
import serial
import time
import sys

def main():
    parser = argparse.ArgumentParser(description="Jetson UART test 9600 ASCII")
    parser.add_argument("--device", type=str, default="/dev/ttyTHS1", help="UART device (e.g. /dev/ttyTHS1, /dev/ttyUSB0) — SET CORRECTLY, no auto guess")
    parser.add_argument("--baud", type=int, default=9600, help="Baud 9600")
    parser.add_argument("--cmd", type=str, default="101", help="Command to send (e.g. 101, 324)")
    parser.add_argument("--timeout", type=float, default=1.0, help="ACK timeout seconds")
    args = parser.parse_args()

    # PLACEHOLDER: if you don't know device, set --device to your Jetson's UART connected to ESP32 GPIO17/18
    # ESP32 TX17 -> Jetson RX, ESP32 RX18 <- Jetson TX, GND common
    print("=== UART TEST JETSON ===")
    print(f"DEVICE: {args.device}")
    print(f"BAUD: {args.baud}")
    print(f"FORMAT: ASCII TERM: CR")
    print("========================")

    try:
        ser = serial.Serial(args.device, args.baud, timeout=0.1)
    except Exception as e:
        print(f"[ERROR] Cannot open {args.device}: {e}")
        print("HINT: Check Jetson UART device with: ls /dev/ttyTHS* /dev/ttyUSB* ; dmesg | grep tty")
        sys.exit(1)

    # Clear buffers
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    time.sleep(0.2)

    raw_tx = f"{args.cmd}\r"
    print(f"[UART TX] RAW: {args.cmd}")
    try:
        ser.write(raw_tx.encode('ascii'))
        ser.flush()
        print(f"[UART TX] Sent: {repr(raw_tx)}")
    except Exception as e:
        print(f"[ERROR] TX failed: {e}")
        sys.exit(1)

    # Wait for ACK
    start = time.time()
    rx_buf = ""
    ack_ok = False
    expected = f"ACK {args.cmd}"

    print(f"[UART RX] Waiting for: {repr(expected + chr(13))} (timeout {args.timeout}s)")
    while time.time() - start < args.timeout:
        if ser.in_waiting > 0:
            data = ser.read(ser.in_waiting).decode('ascii', errors='ignore')
            rx_buf += data
            # Check if expected ACK with CR is in buffer
            if expected in rx_buf:
                # Find full line ending with \r
                if f"{expected}\r" in rx_buf:
                    print(f"[UART RX] RAW: {repr(expected + chr(13))}")
                    print(f"[RESULT] ACK RECEIVED: {expected}")
                    ack_ok = True
                    break
                # Also accept without \r for robustness
                print(f"[UART RX] RAW: {repr(rx_buf)}")
        time.sleep(0.02)

    if ack_ok:
        print("[RESULT] PASS — Jetson TX -> ESP32 RX -> ESP32 ACK -> Jetson RX verified")
        print(f"RX: {args.cmd}  TX: ACK {args.cmd}")
    else:
        print(f"[UART RX] RAW buffer: {repr(rx_buf)}")
        if rx_buf == "":
            print("[RESULT] FAIL — No ACK received (TIMEOUT) — check wiring 17/18 GND and ESP32 test firmware")
        else:
            print(f"[RESULT] FAIL — Expected {repr(expected + chr(13))} not found")

    ser.close()
    sys.exit(0 if ack_ok else 1)

if __name__ == "__main__":
    main()
