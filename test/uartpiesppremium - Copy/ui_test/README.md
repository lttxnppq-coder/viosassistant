# Smart AC Control — UI Test

Isolated desktop UI prototype for testing the Smart AC dashboard.

This UI is an isolated prototype.
It does not connect to ESP32, Vosk, UART, microphone or audio.

The real project pipeline is untouched:

```
MIC → Vosk → command_ai → CODE → ESP32 → WAV
```

This UI simulates it on the PC only:

```
Keyboard
   ↓
Fake UI State
   ↓
Tkinter Dashboard
```

## Run

```bash
python ui_test/main.py
```

Automated self-check (sends all test keys, verifies state, closes itself):

```bash
python ui_test/main.py --smoke
```

Requires only Python 3 stdlib (Tkinter). No extra packages.

## Controls

Keyboard simulation — no touchscreen, no on-screen buttons:

| Key | Action | Last command |
|-----|--------|--------------|
| `1` | AC ON | Đã bật điều hòa |
| `2` | AC OFF | Đã tắt điều hòa |
| `↑` | TEMP +2°C | Đã tăng nhiệt độ lên hai độ |
| `↓` | TEMP -2°C | Đã giảm nhiệt độ xuống hai độ |
| `F` | FAN ON | Đã bật quạt |
| `G` | FAN OFF | Đã tắt quạt |
| `M` | WIND FACE | Đã chuyển hướng gió lên mặt |
| `K` | WIND FOOT | Đã chuyển hướng gió xuống chân |
| `D` | DEFROST toggle | Đã bật/tắt chế độ sưởi kính chắn gió |
| `Z` | TEMP 18°C | Đã đặt nhiệt độ 18 độ |
| `X` | TEMP 19°C | Đã đặt nhiệt độ 19 độ |
| `C` | TEMP 20°C | Đã đặt nhiệt độ 20 độ |
| `V` | TEMP 21°C | Đã đặt nhiệt độ 21 độ |
| `B` | TEMP 22°C | Đã đặt nhiệt độ 22 độ |
| `3` | TEMP 23°C | Đã đặt nhiệt độ 23 độ |
| `4` | TEMP 24°C | Đã đặt nhiệt độ 24 độ |
| `5` | TEMP 25°C | Đã đặt nhiệt độ 25 độ |
| `6` | TEMP 26°C | Đã đặt nhiệt độ 26 độ |
| `7` | TEMP 27°C | Đã đặt nhiệt độ 27 độ |
| `8` | TEMP 28°C | Đã đặt nhiệt độ 28 độ |
| `9` | TEMP 29°C | Đã đặt nhiệt độ 29 độ |
| `0` | TEMP 30°C | Đã đặt nhiệt độ 30 độ |
| `U` | UNKNOWN | Tôi chưa hiểu yêu cầu của bạn |
| `R` | RESET | Ready |

Number keys `3–9` / `0` are reserved for temperature — they do NOT conflict
with other commands (the other commands use letters / arrow keys). Temperatures
18–22 °C use the letter keys `Z`, `X`, `C`, `V`, `B`.

## Temperature

Range is fixed at **18–30 °C**:

- At 18 °C, pressing `↓` stays at 18 °C.
- At 30 °C, pressing `↑` stays at 30 °C.

The UI never crashes on boundary input.

## System states

`READY` → `LISTENING` → `PROCESSING` → `EXECUTING` → `READY`
after every simulated command (shown in the header and footer).

## Architecture

```
ui_test/
├── main.py        — Tkinter dashboard + keyboard simulation + --smoke self-check
├── ui_state.py    — UIState (pure stdlib, no project imports)
└── README.md
```

`ui_state.py` imports nothing from the main project (`command_ai`,
`vosk_test`, `esp32_sender`, ...), does not read `config.json`, opens no COM
port, plays no WAV and touches no microphone. The project pipeline stays
unchanged.