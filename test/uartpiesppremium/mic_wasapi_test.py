import sys
sys.stdout.reconfigure(encoding="utf-8")
import sounddevice as sd
import numpy as np
import time

for hostapi, dev, nch in [(2, 9, 4), (2, 1, 6), (2, 21, 6), (2, 22, 6)]:
    try:
        info = sd.query_devices(dev)
    except Exception as e:
        print(f"dev {dev}: {e}")
        continue
    fs = int(info["default_samplerate"])
    if fs > 48000:
        fs = 48000
    blocks = []

    def cb(indata, frames, t, status):
        blocks.append(indata.copy())

    print(f"\n>>> PREPARE 2s: WASAPI dev[{dev}] {info['name']} ch={nch} fs={fs}")
    time.sleep(2)
    print(f">>> NÓI NGAY (dev {dev}): 'tiến lên, lùi lại, rẽ trái, rẽ phải, dừng lại'...")
    try:
        with sd.InputStream(samplerate=fs, blocksize=4000, dtype="float32", channels=nch, device=dev, callback=cb):
            time.sleep(6)
        data = np.concatenate(blocks) if blocks else np.zeros((0, nch), dtype=np.float32)
        peak = float(np.abs(data).max())
        rms = float(np.sqrt((data ** 2).mean()))
        print(f"<<< END dev {dev}: peak={peak:.4f} rms={rms:.5f}")
        if data.ndim > 1:
            print("    per-channel peaks:", [f"{float(np.abs(data[:, c]).max()):.4f}" for c in range(data.shape[1])])
    except Exception as e:
        print(f"<<< ERROR: {e}")