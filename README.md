# ViosAssistant — Virtual Assistant Air Conditioner Controller

ESP32-S3 firmware platform for a virtual-assistant-controlled automotive air
conditioner. Integrates NTC temperature sensing, an OLED display, motorized
damper with quadrature encoder feedback, PWM fan control, relay outputs, a
Raspberry Pi UART link, and a UART-to-CAN module.

- **Board:** ESP32-S3 N16R8 CH343P
- **Toolchain:** Arduino CLI + makeEspArduino (`makeEspArduino/`)
- **Local libs:** `lib/Adafruit_GFX`, `lib/Adafruit_SSD1306`, `lib/Adafruit_BusIO`

---

## Project Status (2026-09)

| Item | Status |
|---|---|
| Hardware architecture | DONE |
| Pin configuration (`PinConfig.h`) | DONE — single source of truth |
| Production architecture | DONE |
| Software tests T01–T24 | DONE / VERIFIED (compile + static + integration) |
| Production build | DONE — PASS (full link 22 `.cpp`) |
| Hardware validation | **NOT EXECUTED** |
| Current milestone | Software verification completed / preparation for hardware validation |

> `BUILD PASS` is **not** `HARDWARE PASS`. All T01–T24 are verified as
> compile/static/integration PASS against the real production API. No upload,
> no flash, no hardware run has been performed. Hardware tests H01–H09 are
> pending (see reports).

### Milestone detail

- **Software verification completed:** T01–T24 all PASS as build/static
  checks against `PinConfig.h` and the production source.
- **Preparation for hardware validation:** `FINAL_PRE_HARDWARE_REVIEW.md` and
  `FINALIZE_PRODUCTION_BEFORE_HARDWARE.md` describe the H01–H09 matrix and the
  blockers to resolve before power-on (GPIO10 input mode, Fan FET polarity,
  CAN module spec, motor nSLEEP, encoder USB-OTG note).

---

## Layout

| Path | Contents |
|---|---|
| `application/` | SystemManager, CommandManager, ResponseManager |
| `config/` | `PinConfig`-independent system & protocol config |
| `drivers/` | Hardware driver layer (NTC, OLED, Relay, PWM, Motor, Encoder, CAN, UART) |
| `model/` | Data models / system state |
| `services/` | Domain controllers (Climate, Fan, MotorPosition, VehicleData, AirMode) |
| `rtos/` | FreeRTOS tasks (Communication, Control, Oled) |
| `utils/` | Logger, Filter, CRC16 |
| `lib/` | Vendored Arduino libraries |
| `makeEspArduino/` | Build wrapper toolchain |
| `test/` | Test T01–T24 + helper sketches + Pi voice-assistant scripts |
| `PinConfig.h` | Single source of truth for all GPIO assignments |

---

## Reports

- `FINAL_REPORT_T01-T24.md` — full T01–T24 verification summary and evidence.
- `FINAL_PRE_HARDWARE_REVIEW.md` — read-only audit before hardware H01–H09.
- `FINALIZE_PRODUCTION_BEFORE_HARDWARE.md` — production freeze & blockers.
- `UPDATE_REPORT_ENCODER_FAN_HYST.md` — encoder GA25 + fan hysteresis + nSLEEP.

Each report explicitly records **HARDWARE: NOT EXECUTED** for every testcase.

---

## Build (not flashed)

```sh
# makeEspArduino (Windows PowerShell)
make ESP_ROOT="C:/path/to/esp32-arduino/hardware/esp32/3.3.11" deps-check
make ESP_ROOT="C:/path/to/esp32-arduino/hardware/esp32/3.3.11"
```

Build verification for T01–T24 used `arduino-cli` with 8 include dirs and
`--libraries lib` (see `FINAL_REPORT_T01-T24.md`).

**Do not flash/upload until H01–H09 blockers are resolved.**

---

## Notes on ignored content

Large voice/speech/model binaries and build outputs (`.wav`, `.mp3`, `.onnx`,
`.fst`, `.zip`, `.venv/`, `.pio/`, `*.bin/*.elf/*.o/*.a/*.map/*.pyc`) are kept
on disk but excluded from this repo via `.gitignore`. Source, tests, and
documentation are tracked normally.

> GPIO19/20 are assigned to the GA25 encoder by confirmed user decision. They
> overlap the generic-S3 USB-OTG D-/D+ pins; on this board USB uses the CH343P
> (GPIO43/44). Ensure no USB-OTG cable conflicts during encoder testing.
