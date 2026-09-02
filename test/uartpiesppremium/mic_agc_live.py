import sys
sys.stdout.reconfigure(encoding="utf-8")
import sounddevice as sd
import numpy as np
import time
import json
from vosk import Model, KaldiRecognizer

def pick_mic():
    best = None
    for i, d in enumerate(sd.query_devices()):
        if d["max_input_channels"] <= 0:
            continue
        name = d["name"]
        score = 0
        if "array 2" in name.lower():
            score = 3
        elif "array 1" in name.lower():
            score = 2
        elif "microphone array" in name.lower():
            score = 1
        if score > (best[0] if best else 0):
            best = (score, i, d)
    return best

score, DEV, info = pick_mic()
print(f"Picked device [{DEV}] {info['name']}")

model = Model(r"vosk-model-small-vn-0.4")
rec = KaldiRecognizer(model, 16000)
fs = 16000
queue = []

# Measure noise floor first (2s silence)
noise = []
def cb_noise(indata, frames, t, status):
    noise.append(indata[:, 0].copy())
print("CALIBRATE noise floor 3s (keep quiet)...")
with sd.InputStream(samplerate=fs, blocksize=4000, dtype="float32", channels=1, device=DEV, callback=cb_noise):
    time.sleep(3)
noise_data = np.concatenate(noise) if noise else np.zeros(0, dtype=np.float32)
noise_rms = float(np.sqrt((noise_data.astype(np.float64) ** 2).mean())) if len(noise_data) else 0
print(f"noise floor rms={noise_rms:.5f}")

queue = []
def cb(indata, frames, t, status):
    mono = indata[:, 0].copy()
    rms = float(np.sqrt((mono.astype(np.float64) ** 2).mean()))
    if rms < noise_rms * 2.0:
        return
    gain = 6000.0 / max(rms, 1.0)
    amp = np.clip(mono * gain, -1.0, 1.0)
    queue.append((amp.astype(np.float32).tobytes(), rms))

print("COUNTDOWN 5s...")
for i in range(5, 0, -1):
    print(f"  {i}...")
    time.sleep(1)
print(">>> NÓI NGAY 15s: 'tiến lên' THẬT TO, sát mic, lặp liên tục")
with sd.InputStream(samplerate=fs, blocksize=4000, dtype="float32", channels=1, device=DEV, callback=cb):
    t0 = time.time()
    last_print = 0
    while time.time() - t0 < 15:
        while queue:
            pcm, rms = queue.pop(0)
            rec.AcceptWaveform(pcm)
        p = json.loads(rec.PartialResult())
        if p.get("partial") and time.time() - last_print > 1:
            print("  partial:", p["partial"], f"(rms={rms:.0f})")
            last_print = time.time()
        time.sleep(0.05)
while queue:
    pcm, rms = queue.pop(0)
    rec.AcceptWaveform(pcm)
r = json.loads(rec.Result())
print("FINAL:", repr(r.get("text", "")))