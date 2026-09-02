# UPDATE REPORT -- Encoder GA25 + Fan Hysteresis + nSLEEP

**Project:** `ViosAssistant` -- `C:\Users\hi\Documents\NCKH\main\ViosAssistant`
**Date:** 2026-09-01
**Environment:** `arduino-cli 1.5.1` `ESP32 core 3.3.11` `FQBN ESP32:esp32:esp32s3:CDCOnBoot=default,FlashSize=16M,PartitionScheme=default_8MB` `--jobs 1` `--libraries lib` `compiler.path=C:/Users/hi/AppData/Local/Temp/opencode/gw/` + extra_flags 8 `-I` (root,config,model,drivers,services,application,rtos,utils)
**Mode:** build/compile/static only -- **NO upload / NO flash / NO hardware test** -- `BUILD PASS != HARDWARE PASS`

---

## 1. Files changed

| File | Change |
|------|--------|
| `PinConfig.h:80` | Encoder block: `PIN_ENC_A=19` `PIN_ENC_B=20` (GA25 quadrature). `PIN_ENC_BTN` not used. Added USB-OTG conflict note. |
| `config/SystemConfig.h:22` | Added `FAN_ON_THRESHOLD_C=25.5f` `FAN_OFF_THRESHOLD_C=24.5f` |
| `services/ClimateController.h:52` | Added `bool fan_on_=false` + `bool getFanOn() const` (single decision location) |
| `services/ClimateController.cpp:1` | Include `SystemConfig.h`; hysteresis in `update()` on `inside_temp_c` (`inside_valid` guard) |
| `application/SystemManager.cpp:51` | Log `ENCODER GA25 A=19 B=20` (was TBD); `:93` `motor_pos_ctrl_.begin(&motor_,&encoder_,PIN_ENC_A,PIN_ENC_B,0)`; `:134` `fan_ctrl_.setSpeed(getFanOn()?FAN_SPEED_MAX:FAN_SPEED_MIN)` auto overrides each cycle |
| `drivers/EncoderDriver.h:5` | Comment update: pins now in PinConfig, driver remains param-driven |
| `test/test08_encoder/test08_encoder.ino:8` | Defaults `ENCODER_PIN_A=PIN_ENC_A(19)` `ENCODER_PIN_B=PIN_ENC_B(20)` `BTN=-1`; keep `PULSES_PER_REV=11300`; header updated |
| `test/test18_sensor_service_app/test18_sensor_service_app.ino:1` | Extended: `#include "../../services/ClimateController.cpp"` + `driveCC()` + full hysteresis truth-table (10 edge + toggle + invalid + boundary exact) |
| `test/test20_hw_abstraction/test20_hw_abstraction.ino:42` | `static_assert PIN_ENC_A==19 && PIN_ENC_B==20` (was TBD check) |
| `test/test22_pin_config_audit/test22_pin_config_audit.ino:28` | `static_assert PIN_ENC_A==19 && PIN_ENC_B==20` + USB-OTG note |

No other production files touched. Fan hysteresis lives in one layer only.

---

## 2. Encoder GA25

```text
ENCODER_A = GPIO19 (PIN_ENC_A)  PinConfig.h:89
ENCODER_B = GPIO20 (PIN_ENC_B)  PinConfig.h:90
Type      = quadrature (GA25)    test08:8
PULSES_PER_REV = 11300 (unchanged, test "Dieukhiendongcoencoder")  test08:33
```

**Conflict:** `PinConfig.h:80` previously documented `GPIO19/20 = USB-OTG D-/D+ RESERVED`. On this N16R8 CH343P board USB is via CH343P `GPIO43/44` (`PinConfig.h:72`), so 19/20 are free. **PRODUCTION/HARDWARE CONFLICT:** if USB-OTG is cabled, 19/20 overlap -- **user confirmed assignment anyway** (`Confirm 19/20 anyway`). No pin re-assigned by toolchain; PinConfig is single source, `EncoderDriver::begin(19,20,0)` (`drivers/EncoderDriver.cpp:8`) receives via `SystemManager.cpp:95`, no hardcode 19/20 inside driver. `PULSES_PER_REV` untouched.

---

## 3. nSLEEP

```text
Status: NOT USED -- hardware-controlled / externally handled
```

`PinConfig.h:49` `MotorDriver.h:25` `SystemManager.cpp:37,81,94` all state `nSLEEP = hardware-controlled / pending electrical verification`. No GPIO added, no constant created, no wiring changed, `MotorDriver::Config` has no nSLEEP field -- code does not assume GPIO control. Preserved per user decision.

---

## 4. Fan hysteresis

**Location:** `services/ClimateController` -- the only module receiving `TemperatureData` (`ClimateController.cpp:12`). Single source of truth.

```cpp
// SystemConfig.h:24-25
constexpr float FAN_ON_THRESHOLD_C  = 25.5f;
constexpr float FAN_OFF_THRESHOLD_C = 24.5f;
```

```cpp
// ClimateController.cpp:16-28
if (!temps.inside_valid) return; // HOLD on invalid
float t = temps.inside_temp_c;   // NTC1 (PinConfig.h:57, user decision: inside)
if (!fan_on_) { if (t > 25.5f) fan_on_ = true; }
else          { if (t < 24.5f) fan_on_ = false; }
// 24.5..25.5 HOLD -- no toggle
```

Truth table exactly as spec:

```text
OFF: T=24.0->OFF  24.5->OFF  25.0->OFF  25.5->OFF  25.51->ON
ON : T=26.0->ON   25.5->ON   25.0->ON   24.5->ON   24.49->OFF
24.99↔25.01 : no toggle (both OFF and ON held)
```

Wired in `SystemManager.cpp:134` -- `fan_ctrl_.setSpeed(getFanOn()?FAN_SPEED_MAX:FAN_SPEED_MIN)` each `update()` cycle; manual `SET_FAN_SPEED` (`SystemManager.cpp:162`) is transient (auto re-applies next cycle, per user decision). No duplicate logic in `FanController`/`VehicleDataService`/`SystemManager`.

Architecture:
```text
NTC (1/2) -> TemperatureData (inside_valid) -> ClimateController::update (hysteresis) -> SystemManager::update -> FanController::setSpeed -> PwmDriver (FET7 1kHz/8-bit)
```

---

## 5. Tests

Re-built after SHR2 cold-cache recovery (4--7s sleep between retries, intermittent `cc1/cc1plus CreateProcess` toolchain spawns classified TOOLCHAIN/ENVIRONMENT).

| TEST | BUILD | RESULT | NOTE |
|------|-------|--------|------|
| TEST08 encoder `test08_encoder.ino` | **PASS 281197 B (8%)** | PASS | A=19 B=20, PPR 11300, `HARDWARE: NOT EXECUTED` |
| TEST13 motor_position `test13_motor_position` | **PASS 289884 B (8%)** | PASS | `MotorPositionController` 0,0,0 path still valid, outside hysteresis |
| TEST14 fan_controller `test14_fan_controller` | **PASS 276336 B (8%)** | PASS | Ramp 10->50, `begin(nullptr)` unchanged -- actuator only |
| TEST17 system_manager `test17_system_manager` | **PASS 304285 B (9%)** | PASS | header-only, now sees `PIN_ENC_A/B` + `getFanOn` |
| TEST18 sensor_service_app `test18_sensor_service_app` | **PASS 306185 B (9%)** | **PASS** | **Extended** -- 10 edge + 24.99↔25.01 no-toggle + invalid HOLD + 25.5/24.5 exact |
| TEST19 cmd_ctrl_driver `test19_cmd_ctrl_driver` | **PASS 274057 B (8%)** | PASS | header-only chain, `SET_FAN_SPEED` still transient |
| TEST20 hw_abstraction `test20_hw_abstraction` | **PASS 304397 B (9%)** | PASS | Now asserts `PIN_ENC_A==19 && PIN_ENC_B==20` |
| TEST21 production_build `test21_production_build` | **PASS 366813 B (10%)** | PASS | Full link, 22 .cpp via merged sketch + gw |
| TEST22 pin_config_audit `test22_pin_config_audit` | **PASS 274073 B (8%)** | PASS | Now asserts `PIN_ENC_A==19 && PIN_ENC_B==20` |

Other T01--T24 (not re-built this batch, still PASS in `OUT/*.log`):
`T01 339030 T02 352026 T03 302730 T04 302322 T05 274265 T06 281792 T07 312009 T09 274453 T10 275165 T11 274305 T12 274673 T15 274433 T16 274401 T23 304421 T24 304705` -- unchanged, `HARDWARE: NOT EXECUTED`.

---

## 6. Production build

```text
BUILD: PASS 366813 B (10% flash, 7% RAM)  test21_production_build  SHR2/gw + merged sketch
FLASH: NOT EXECUTED
HARDWARE TEST: NOT EXECUTED
```

Toolchain cold-cache flaky spawns (`cc1/cc1plus CreateProcess`) require 1--4 retries per sketch (7s sleep, process kill) -- classified `TOOLCHAIN / ENVIRONMENT ERROR`, not source.

---

## 7. Remaining issues

* **GPIO10 input mode TBD** -- `PinConfig.h:42` `INPUT / INPUT_PULLUP / INPUT_PULLDOWN` needs hardware confirmation; T04 uses `INPUT_PULLUP` placeholder.
* **Fan FET GPIO7 polarity/termination** -- `PinConfig.h:36` `FanController.cpp:11` 1kHz/8-bit, relay 5+FET 7 independent, flyback diode not confirmed.
* **CanDriver stub** -- `drivers/CanDriver.cpp:1` UART-to-CAN not fully implemented; `services/VehicleDataService.cpp:55` parse stub; `application/CommandManager.cpp:23` always-success stub.
* **`utils/Filter.h:15` Median `sorted[15]`** -- overflow risk if window >15.
* **GPIO19/20 conflict noted above** -- accepted per user, document as hardware caveat if USB-OTG cabled.

Dừng -- không flash, không hardware test, chờ user xác nhận nếu cần chỉnh thêm trước H01--H09.
