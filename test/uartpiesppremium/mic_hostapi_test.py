import sys
sys.stdout.reconfigure(encoding="utf-8")
import sounddevice as sd
import numpy as np
import time

for hostapi in range(len(sd.query_hostapis())):
    ha = sd.query_hostapis(hostapi)
    dev = None
    for d in ha["devices"]:
        info = sd.query_devices(d)
        if info["max_input_channels"] > 0:
            dev = d
            break
    if dev is None:
        print(f"hostapi[{hostapi}] {ha['name']}: no input device")
        continue
    info = sd.query_devices(dev)
    fs = int(info["default_samplerate"])
    if fs > 48000:
        fs = 48000
    blocks = []

    def cb(indata, frames, t, status):
        blocks.append(indata.copy())

    print(f"\n>>> PREPARE 2s: hostapi[{hostapi}] {ha['name']} dev[{dev}] {info['name']}")
    time.sleep(2)
    print(f">>> NÓI NGAY ({ha['name']}): 'tiến lên, lùi lại, rẽ trái, rẽ phải, dừng lại'...")
    try:
        with sd.InputStream(samplerate=fs, blocksize=4000, dtype="float32", channels=1, device=dev, callback=cb):
            time.sleep(6)
        data = np.concatenate(blocks) if blocks else np.zeros((0,), dtype=np.float32)
        print(f"<<< END: peak={float(np.abs(data).max()):.4f} rms={float(np.sqrt((data ** 2).mean())):.5f}")
    except Exception as e:
        print(f"<<< ERROR: {e}")