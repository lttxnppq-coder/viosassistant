import sounddevice as sd
import numpy as np
import time
from vosk import Model, KaldiRecognizer
import json, wave, sys

sys.stdout.reconfigure(encoding="utf-8")

DEV = 21
info = sd.query_devices(DEV)
fs = 16000
blocks = []

def cb(indata, frames, t, status):
    blocks.append(indata.copy())

print(f"[{DEV}] {info['name']} - RECORDING 10s, SPEAK LOUDLY NOW: 'tien len, lui lai'...")
with sd.InputStream(samplerate=fs, blocksize=4000, dtype="float32", channels=1, device=DEV, callback=cb):
    time.sleep(10)

data = np.concatenate(blocks)
pcm = (np.clip(data, -1, 1) * 32767).astype(np.int16)
peak = float(np.abs(pcm).max())
print("peak(int16):", int(peak))

with wave.open("rec_10s.wav", "wb") as wf:
    wf.setnchannels(1)
    wf.setsampwidth(2)
    wf.setframerate(fs)
    wf.writeframes(pcm.tobytes())
print("saved rec_10s.wav")

model = Model(r"vosk-model-small-vn-0.4")
rec = KaldiRecognizer(model, fs)
with open("rec_10s.wav", "rb") as f:
    data = f.read()
    while True:
        chunk = data[:4000 * 2]
        if not chunk:
            break
        rec.AcceptWaveform(chunk)
        data = data[4000 * 2:]
res = json.loads(rec.FinalResult())
print("VOSK TEXT:", repr(res.get("text", "")))