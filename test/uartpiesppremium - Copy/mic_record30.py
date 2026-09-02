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

print(f"[{DEV}] {info['name']} - RECORDING 30s, SPEAK CONTINUOUSLY & LOUDLY...")
with sd.InputStream(samplerate=fs, blocksize=4000, dtype="float32", channels=nch, device=DEV, callback=cb):
    time.sleep(30)

data = np.concatenate(blocks)
print("frames:", len(data))
pcm = (np.clip(data, -1, 1) * 32767).astype(np.int16)
import wave
with wave.open("rec_30s.wav", "wb") as wf:
    wf.setnchannels(nch)
    wf.setsampwidth(2)
    wf.setframerate(fs)
    wf.writeframes(pcm.tobytes())
print("saved rec_30s.wav (multi-channel)")

mono = pcm[:, 0] if pcm.ndim > 1 else pcm
block = fs // 4
print("time  peak  rms  (channel 0)")
for i in range(0, len(mono) - block, block):
    seg = mono[i:i + block]
    print(f"{i/fs:5.1f}s peak={int(np.abs(seg).max()):6d} rms={float(np.sqrt((seg.astype(np.float64)**2).mean())):7.1f}")
if pcm.ndim > 1:
    print("channel peaks:", [int(np.abs(pcm[:, c]).max()) for c in range(pcm.shape[1])])