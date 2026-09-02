import sys
sys.stdout.reconfigure(encoding="utf-8")
import sounddevice as sd
import numpy as np
import time

def find_input(hostapi, needle):
    ha = sd.query_hostapis(hostapi)
    for d in ha["devices"]:
        info = sd.query_devices(d)
        if info["max_input_channels"] > 0 and needle.lower() in info["name"].lower():
            return d, info
    return None, None

print("Host APIs:", [(i, h["name"]) for i, h in enumerate(sd.query_hostapis())])

targets = {}
for ha in range(len(sd.query_hostapis())):
    ha_name = sd.query_hostapis(ha)["name"]
    if ha_name in ("MME", "Windows WASAPI"):
        for needle in ("microphone array 2", "microphone array 1", "microphone array"):
            d, info = find_input(ha, needle)
            if d is not None:
                targets[ha_name] = (d, info)
                break

print("Targets:", {k: (v[0], v[1]["name"]) for k, v in targets.items()})

streams = []
for ha_name, (d, info) in targets.items():
    fs = int(info["default_samplerate"])
    nch = min(info["max_input_channels"], 4)
    blocks = []

    def make_cb(bl):
        def cb(indata, frames, t, status):
            bl.append(indata.copy())
        return cb

    try:
        s = sd.InputStream(samplerate=fs, blocksize=4000, dtype="float32",
                           channels=nch, device=d, callback=make_cb(blocks))
        s.start()
        streams.append((ha_name, s, blocks, fs, nch))
        print(f"opened {ha_name} dev[{d}] ch={nch} fs={fs}")
    except Exception as e:
        print(f"{ha_name} ERROR: {e}")

if not streams:
    print("NO STREAMS OPENED")
    sys.exit(1)

print("COUNTDOWN 5s...")
for i in range(5, 0, -1):
    print(f"  {i}...")
    time.sleep(1)
print(">>> NÓI NGAY 15s: 'tiến lên' 'lùi lại' 'rẽ trái' 'rẽ phải' 'dừng lại' — từng câu rõ ràng")
time.sleep(15)
print("<<< END")

for ha_name, s, blocks, fs, nch in streams:
    s.stop()
    s.close()
    data = np.concatenate(blocks) if blocks else np.zeros((0, nch), dtype=np.float32)
    peak = float(np.abs(data).max())
    rms = float(np.sqrt((data ** 2).mean()))
    print(f"{ha_name}: peak={peak:.4f} rms={rms:.5f}")