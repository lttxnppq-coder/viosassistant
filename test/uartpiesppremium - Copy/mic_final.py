import sys
sys.stdout.reconfigure(encoding="utf-8")
import sounddevice as sd
import numpy as np
import time

DEV = 21
info = sd.query_devices(DEV)
fs = 16000
nch = min(info["max_input_channels"], 6)
blocks = []

def cb(indata, frames, t, status):
    blocks.append(indata.copy())

print("COUNTDOWN 5s...")
for i in range(5, 0, -1):
    print(f"  {i}...")
    time.sleep(1)
print(">>> NÓI NGAY TRONG 15 GIÂY: 'tiến lên', 'lùi lại', 'rẽ trái', 'rẽ phải', 'dừng lại' - từng câu, rõ ràng")
with sd.InputStream(samplerate=fs, blocksize=4000, dtype="float32", channels=1, device=DEV, callback=cb):
    t0 = time.time()
    while time.time() - t0 < 15:
        time.sleep(1)
        print(f"  {15 - int(time.time() - t0)}s còn lại...")
print("<<< KẾT THÚC GHI")

data = np.concatenate(blocks)
pcm = (np.clip(data, -1, 1) * 32767).astype(np.int16)
import wave
with wave.open("rec_final.wav", "wb") as wf:
    wf.setnchannels(nch)
    wf.setsampwidth(2)
    wf.setframerate(fs)
    wf.writeframes(pcm.tobytes())

mono = pcm if pcm.ndim == 1 else pcm[:, 0]
print("overall peak:", int(np.abs(mono).max()), "/ 32767")
block = fs // 4
high = [(i / fs, int(np.abs(mono[i:i + block]).max())) for i in range(0, len(mono) - block, block)]
for t, p in high:
    print(f"  {t:5.1f}s peak={p:6d}")