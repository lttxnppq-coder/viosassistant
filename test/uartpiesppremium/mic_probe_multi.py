import sounddevice as sd
import numpy as np
import time

candidates = [1, 5, 9, 20, 21, 22]
results = {}
for dev in candidates:
    info = sd.query_devices(dev)
    nch = min(info["max_input_channels"], 6)
    fs = int(info["default_samplerate"])
    if fs > 48000:
        fs = 48000
    blocks = []

    def cb(indata, frames, t, status):
        blocks.append(indata.copy())

    print(f"[{dev}] {info['name']} ch={nch} fs={fs} - RECORDING 5s, KEEP SPEAKING...")
    with sd.InputStream(samplerate=fs, blocksize=4000, dtype="float32", channels=nch, device=dev, callback=cb):
        time.sleep(5)
    data = np.concatenate(blocks) if blocks else np.zeros((0, nch), dtype=np.float32)
    per = float(np.abs(data).max())
    rms = float(np.sqrt((data ** 2).mean()))
    results[dev] = (per, rms)
    print(f"    -> peak={per:.4f} rms={rms:.5f}")

print()
print("SUMMARY (best first):")
for dev, (per, rms) in sorted(results.items(), key=lambda kv: -kv[1][0]):
    print(f"  dev {dev}: peak={per:.4f} rms={rms:.5f}")