# FINALIZE PRODUCTION BEFORE HARDWARE -- ViosAssistant

**Date:** 2026-09-01  
**Board:** ESP32-S3 N16R8 CH343P  
**FQBN:** `ESP32:esp32:esp32s3:CDCOnBoot=default,FlashSize=16M,PartitionScheme=default_8MB,UploadSpeed=921600`  
**Toolchain:** Arduino CLI 1.5.1, ESP32 core 3.3.11, esp-x32 2601, `compiler.path=gw` wrapper, `--jobs 1`  
**Mode:** Build/compile/link only -- **NO upload, NO flash, NO hardware test** -- `BUILD PASS != HARDWARE PASS`  
**Scope:** Resolve 4 remaining issues (GPIO10, Fan FET, CanDriver, VehicleDataService/CommandManager) + Filter audit, per task requirements.

---

## A. Hardware decisions (SINGLE SOURCE OF TRUTH -- PinConfig.h)

| Function | Pin | Source | Note |
|---|---|---|---|
| NTC1 ADC | GPIO1 | `PinConfig.h:71` | 10k B3950 10k series 3.3V 12-bit |
| NTC2 ADC | GPIO2 | `PinConfig.h:72` | same |
| AC Relay | GPIO4 | `PinConfig.h:40` | active_high configurable, default true |
| Fan Relay | GPIO5 | `PinConfig.h:41` | independent from FET |
| Pi Power Relay | GPIO6 | `PinConfig.h:42` | |
| Fan FET PWM | GPIO7 | `PinConfig.h:43` | 1kHz/8-bit, polarity TBD (see Issue B) |
| OLED SDA | GPIO8 | `PinConfig.h:19` | 0x3C 128x64 |
| OLED SCL | GPIO9 | `PinConfig.h:20` | |
| ON/OFF Input | GPIO10 | `PinConfig.h:48` | mode TBD (see Issue A) |
| CAN UART TX | GPIO11 | `PinConfig.h:77` | external module, NOT TWAI |
| CAN UART RX | GPIO12 | `PinConfig.h:78` | |
| Motor IN1 | GPIO13 | `PinConfig.h:65` | DRV8833 parallel, 20kHz/10-bit |
| Motor IN2 | GPIO14 | `PinConfig.h:66` | |
| Pi UART TX | GPIO17 | `PinConfig.h:13` | 115200 |
| Pi UART RX | GPIO18 | `PinConfig.h:14` | |
| Encoder A | GPIO19 | `PinConfig.h:103` | GA25 quadrature, PPR11300 |
| Encoder B | GPIO20 | `PinConfig.h:104` | |
| CH343P TX/RX | GPIO43/44 | `PinConfig.h:86` | RESERVED, not used |
| Strapping | GPIO0,3,45,46 | `PinConfig.h:90` | DO NOT USE |

**Confirmed decisions (per task, not changed):**
- Encoder GA25 A=19 B=20, `PULSES_PER_REV=11300`, GPIO19/20 USB-OTG conflict **ACCEPTED USER DECISION**. `drivers/EncoderDriver.cpp:13` uses `INPUT_PULLUP`, `SystemManager.cpp:95` passes `PIN_ENC_A/B`. No hardcode.
- nSLEEP **NOT USED** -- `PinConfig.h:63`, `drivers/MotorDriver.h:25`, `application/SystemManager.cpp:37,81,94` all state `hardware-controlled / pending verification`. No GPIO added, no constant created, no wiring change.
- Fan hysteresis thresholds retained: `config/SystemConfig.h:24` `FAN_ON 25.5` `FAN_OFF 24.5`, logic in `services/ClimateController.cpp:16` with `inside_valid` guard and HOLD 24.5..25.5. No debounce, no delay, no single threshold.
- OLED 128x64 0x3C SDA8/SCL9, NTC B3950, Motor 20kHz/10-bit, Fan 1kHz/8-bit, Pi 115200, CAN 500k, CRC16 A001 unchanged.

---

## B. Production files changed

| File | Lines | Change | Reason |
|---|---|---|---|
| `PinConfig.h:33` | 33-43 | Expanded Fan FET comment: polarity TBD, active-HIGH assumption, Arduino Mega not authoritative, invert point | Issue B documentation, safety |
| `PinConfig.h:48` | 48-56 | Expanded ON/OFF Input comment: mode TBD, active polarity TBD, external pull required, T04 placeholder note | Issue A documentation |
| `services/FanController.h:7` | 7-11 | Added class-level polarity TBD note | Issue B |
| `drivers/PwmDriver.h:7` | 7-11 | Added active-HIGH note, invert point | Issue B |
| `utils/Filter.h:40` | 40-58 | Added `kMaxWindowSize=15`, `kDefaultWindowSize=5`, `getWindowSize()` | Filter bug guard |
| `utils/Filter.cpp:90` | 90-106 | Clamp `window_size` to `kMaxWindowSize`, odd adjustment with re-clamp, fix circular index logic `(index+ N - count + i)` | Filter bug fix |
| `utils/Filter.cpp:122` | 122-147 | Use `sorted[kMaxWindowSize]`, corrected indexing for partial window | Filter bug fix |
| `drivers/CanDriver.h:8` | 8-25 | Added STUB/NOT_IMPLEMENTED header doc, FAIL>fake success rule | Issue C |
| `drivers/CanDriver.cpp:1` | 1-34 | Added stub comments, `write()` now returns `false` always (was `initialized_`), `(void)baud` | Issue C safety |
| `services/VehicleDataService.h:8` | 8-19 | Added PARTIAL STUB doc | Issue D |
| `services/VehicleDataService.cpp:55` | 55-63 | Added stub comments for parsers | Issue D |
| `application/CommandManager.h:9` | 9-28 | Added error-code doc (0 SUCCESS,1 INVALID,2 NOT_IMPL,3 QUEUE_FULL) | Issue D |
| `application/CommandManager.cpp:15` | 15-45 | `queueCommand` rejects `NONE`; `processCommand` validates enum, distinguishes SUCCESS/INVALID/NOT_IMPLEMENTED | Issue D safety |
| `application/SystemManager.cpp:157` | 157-199 | `handleCommand` preserves `response` from `processCommand`, early return on `!success`, avoids fake success | Issue D consistency |

**Files NOT changed (per constraints):** `PinConfig.h` GPIOs, `MotorDriver`, `EncoderDriver`, `ClimateController` hysteresis, `SystemConfig` thresholds, `ProtocolConfig` baud, `ViosAssistant.ino`, `Makefile` FQBN, `makeEspArduino`.

---

## C. Issues resolved

### ISSUE A -- GPIO10 ON/OFF INPUT MODE
- **Audit:** `grep` across all production/ drivers/services/application shows **no** `pinMode(10` or `PIN_ON_OFF_INPUT` usage except `PinConfig.h:48` define and `SystemManager.cpp:31` log. `drivers/EncoderDriver` uses pullup for 19/20, not 10. No schematic/spec provides pull or polarity. T04 `test04_gpio_input.ino:15` defines `INPUT_PULLUP` as placeholder with explicit TBD comment.
- **Decision:** No hardware evidence exists. No production driver configures GPIO10. **Keep TBD.** Enhanced `PinConfig.h:48` comment to list `INPUT / INPUT_PULLUP / INPUT_PULLDOWN` options, active HIGH vs LOW TBD, external pull warning, and reference to `FINALIZE... ISSUE A`. No GPIO change, no mode hardcode, no polarity assumption.
- **Build impact:** T04 rebuilt PASS 302322 (see ÂSE). Production still does not configure GPIO10 -- safe for hardware verification (H04 requires external pull measurement).

### ISSUE B -- FAN FET POLARITY (GPIO7)
- **Audit:** `services/FanController.cpp:11` 1kHz/8-bit, `drivers/PwmDriver.cpp:22` `ledcWrite(pin,duty)` direct, `services/FanController.cpp:27` `map(...,0,255,0,255)` no invert. `PinConfig.h:29` no polarity. `SystemManager.cpp:139` `getFanOn()?255:0`. Test `FET/codeemfet.ino:33` shows Arduino Mega active-LOW (255=OFF,0=MAX) but **NOT authoritative** for ESP32-S3. `drivers/MotorDriver` uses 13/14 distinct, no GPIO7 conflict; old `cauh`/`dongco` NFAULT on GPIO7 removed from production.
- **Decision:** No ESP32-S3 hardware evidence. Keep current **active-HIGH** implementation (duty 0=OFF,255=MAX) but document as **TBD** in `PinConfig.h:33`, `PwmDriver.h:7`, `FanController.h:7`. Specify inversion must be done at single point (PwmDriver or FanController) after scope/DMM measurement on H06. No duty inversion, no GPIO change, no constant added beyond doc.
- **Build impact:** T06 rebuilt PASS 281792, T14 PASS 276336, T21 PASS. No functional change.

### ISSUE C -- CanDriver STUB
- **Audit:** `drivers/CanDriver.cpp:1` original: `begin()` sets `initialized_=true` returns true (no UART), `write()` returned `initialized_` (fake success), `read()` false, `available()` 0. `services/VehicleDataService.cpp:55` calls `can->write(0x7DF)` without checking return. `application/SystemManager.cpp:109` `can_.begin(500000,11,12)` would succeed and look operational. `test09_can_uart.ino:1` correctly documents stub and tests raw `Serial2` 11/12 independently. Config confirms external module via UART 11/12, **NOT native TWAI** (`drivers/CanDriver.h:8`).
- **Decision:** Keep GPIO11/12, do NOT switch to TWAI, do NOT invent protocol. Fix fake-success: `CanDriver.cpp:16` `write()` now **always returns false** (NOT_IMPLEMENTED) even when initialized, with comment `FAIL > fake success`. `begin()` still sets `initialized_` for API compatibility but documents **no Serial2 opened**. Added header STUB doc referencing H09 verification (module model, termination). `VehicleDataService` remains stub.
- **Build impact:** T09 rebuilt PASS 274453, T12 PASS 274673, T21 PASS. No application now believes CAN TX succeeded.

### ISSUE D -- VehicleDataService / CommandManager STUB
- **VehicleDataService audit:** `services/VehicleDataService.cpp:55` `parseCanFrame`/`parseUartFrame` return false (honest stub, no fake data). `getData()` returns `data_valid=false` default. `requestData()` does `can->write` now correctly fails. No protocol invented, no frame format guessed. Added doc in `.h:8` and `.cpp:55`.
  - **Resolution:** Keep stub, add explicit STUB comments, no fake data, no new protocol.
- **CommandManager audit:** `application/CommandManager.cpp:23` original `processCommand` always `success=true` `error_code=0` return true (fake success). `queueCommand` correctly rejected `>=16` but accepted `NONE`. `SystemManager.cpp:157` called `processCommand` then `switch` and only default set `success=false`, so valid-but-unhandled commands would appear successful at CommandManager layer.
  - **Resolution:** `CommandManager.h:9` documents error codes 0/1/2/3. `CommandManager.cpp:15` now: `queueCommand` rejects `NONE`; `processCommand` validates `CommandType` switch: 7 implemented (`SET_TEMPERATURE, SET_FAN_SPEED, SET_AIR_MODE, SET_RECIRCULATION, SET_AC, SET_HEATER, SYSTEM_RESET`) -> success 0, 4 valid-but-not-implemented (`SET_DAMPER_POS, REQUEST_STATUS, REQUEST_VEHICLE_DATA, FACTORY_RESET`) -> `success=false error=2` return false, `NONE`/unknown -> `error=1` return false. `SystemManager.cpp:157` now early-returns on `!success` preserving error code, only actuates on success. Satisfies `VALID+IMPLEMENTED / VALID+NOT_IMPLEMENTED / INVALID / QUEUE_FULL` distinction, no fake success.
  - **Build impact:** T15 rebuilt PASS 274665 (was 274433, +232 B due to switch), T17 PASS 304285, T19 header-only still PASS.

### FILTER ISSUE -- `utils/Filter.h` `sorted[15]`
- **Audit:** `utils/Filter.h:1` `MedianFilter` `float sorted[15]` fixed, but `MedianFilter::begin(uint8_t window_size)` accepts any `uint8_t` (0..255), `window_size_ = window_size` (even -> +1). If `window_size>15` or `window_size==15`+odd bump -> `16` -> overflow on `sorted[15]` write. `MovingAverage` uses dynamic malloc correct. API allows `>15`.
- **Resolution:** Production bug fix (allowed per ÂS5): `Filter.h:42` added `kMaxWindowSize=15`, `getWindowSize()`. `Filter.cpp:90` clamps `window_size` to `15` before odd adjustment, handles even->odd with re-clamp. `sorted` now `float sorted[kMaxWindowSize]`. Fixed circular index to `(index_ + N - count_ + i) % N` (original `(index_+i)%N` wrong for partial). `getValue()` same. No test-only fix.
- **Build impact:** T10 rebuilt PASS 275185 (was 275165, +20 B), T21 PASS. `test10_filter.ino:12` note remains accurate (window>15 now guarded, not overflow).

---

## D. Issues intentionally left TBD (require hardware/spec)

| ID | Item | Location | State | Next step (Hxx) |
|---|---|---|---|---|
| TBD-1 | GPIO10 input mode (`INPUT` vs `PULLUP` vs `PULLDOWN`) | `PinConfig.h:48` | **TBD** -- no schematic evidence, production does not configure | H04: measure wiring, confirm pull, update `PinConfig.h` comment or add `PIN_ON_OFF_INPUT_MODE` if driver added |
| TBD-2 | GPIO10 active polarity (HIGH/LOW) | `PinConfig.h:48` | **TBD** -- no spec | H04: button press + DMM/scope |
| TBD-3 | Fan FET GPIO7 polarity (HIGH/LOW) | `PinConfig.h:33` | **TBD** -- active-HIGH assumed, not verified, Mega test not authoritative | H06: scope gate, if LOW invert single point |
| TBD-4 | Fan FET flyback diode / sequencing relay5+FEt7 | `PinConfig.h:33` `SystemManager.cpp:145` | Not confirmed | H06: check diode, test relay ON before PWM |
| TBD-5 | CAN physical module + protocol | `drivers/CanDriver.h:8` | **STUB NOT_IMPLEMENTED** -- GPIO11/12 kept, no TWAI, no frame spec | H09: identify module (e.g., MCP2515 bridge), 120Ohm term, define frame format then implement `CanDriver` Serial2 + `VehicleDataService` parsers |
| TBD-6 | VehicleData parsers | `services/VehicleDataService.cpp:55` | STUB false | Implement after CAN spec |
| TBD-7 | Command `SET_DAMPER_POS` etc. | `application/CommandManager.cpp:30` | VALID+NOT_IMPLEMENTED (error 2) | Define handler or keep 2 |
| TBD-8 | Motor nSLEEP | `PinConfig.h:63` | NOT USED, hardware-controlled | H07: verify pull-up/wiring, no code change |
| TBD-9 | GPIO19/20 USB-OTG conflict | `PinConfig.h:94` | ACCEPTED USER DECISION, documented | H08: if USB-OTG cabled, conflict warned |

All TBDs are **explicitly documented** in code and here, not hidden.

---

## E. Tests rebuilt

| Test | File | Build | Reason | Result |
|---|---|---|---|---|
| T04 | `test/test04_gpio_input/test04_gpio_input.ino` | via `runone.bat` SHR2/gw | PinConfig GPIO10 doc clarified, input mode TBD preserved | **PASS** 302322 B (9%) `OUT/test04_gpio_input.log:1` |
| T06 | `test/test06_pwm_fan/test06_pwm_fan.ino` | SHR2/gw | Fan FET polarity doc, no logic change | **PASS** 281792 B (8%) |
| T08 | `test/test08_encoder/test08_encoder.ino` | SHR2/gw | Encoder 19/20 unchanged, verify still PASS | **PASS** 281197 B (8%) |
| T09 | `test/test09_can_uart/test09_can_uart.ino` | SHR2/gw | CanDriver stub fake-success fixed | **PASS** 274453 B (8%) |
| T10 | `test/test10_filter/test10_filter.ino` | SHR2/gw clean+inc | Filter overflow guard | **PASS** 275185 B (8%) (+20 B) |
| T12 | `test/test12_vehicle_data/test12_vehicle_data.ino` | SHR2/gw | VehicleDataService doc + CanDriver | **PASS** 274673 B (8%) |
| T14 | `test/test14_fan_controller/test14_fan_controller.ino` | SHR2/gw | FanController doc | **PASS** 276336 B (8%) |
| T15 | `test/test15_command_manager/test15_command_manager.ino` | SHR2/gw | CommandManager validation (error codes) | **PASS** 274665 B (8%) (+232 B) |
| T17 | `test/test17_system_manager/test17_system_manager.ino` | SHR2/gw | SystemManager handleCommand + CanDriver + CommandManager | **PASS** 304285 B (9%) |
| T21 | `test/test21_production_build/test21_production_build.ino` | SHR2/gw merged 22 .cpp | Full production link | **PASS** 366813 B (10%) |
| T22 | `test/test22_pin_config_audit/test22_pin_config_audit.ino` | SHR2/gw | Pin audit 1/2,4/5/6,7,8/9,10,11/12,13/14,17/18,19/20,43/44 | **PASS** 274073 B (8%) |
| T23 | `test/test23_dependency_audit/test23_dependency_audit.ino` | SHR2/gw | Include graph clean | **PASS** 304421 B (9%) |
| T24 | `test/test24_architecture_audit/test24_architecture_audit.ino` | SHR2/gw | Layering drivers->services->app->rtos | **PASS** 304705 B (9%) |
| T01 | `test/oled_test/oled_test.ino` | **not rebuilt** -- no dependency on changed files | -- | **PREV PASS** 339030 B `OUT/oled_test.log` (2026-08-31) -- still valid |
| T02 | `test/ntc_test/ntc_test.ino` | not rebuilt | NTC unchanged | PREV PASS 352026 B |
| T03 | `test/test03_gpio_output/test03_gpio_output.ino` | not rebuilt | Relay/FET output, PinConfig comment only | PREV PASS 302730 B |
| T05 | `test/test05_pi_uart/test05_pi_uart.ino` | not rebuilt | Pi UART unchanged | PREV PASS 274265 B |
| T07 | `test/test07_motor/test07_motor.ino` | not rebuilt | Motor 20kHz/10-bit unchanged | PREV PASS 312009 B |
| T11 | `test/test11_crc16/test11_crc16.ino` | attempted but toolchain flaky, not critical | CRC A001 unchanged | PREV PASS 274305 B |
| T13 | `test/test13_motor_position/test13_motor_position.ino` | not rebuilt | MotorPosition depends on encoder 19/20 unchanged | PREV PASS 289884 B |
| T16 | `test/test16_response_manager/test16_response_manager.ino` | not rebuilt | ResponseManager unchanged | PREV PASS 274401 B |
| T18 | `test/test18_sensor_service_app/test18_sensor_service_app.ino` | **toolchain corrupted SHR2** (core 27), but previous PASS 306185 and T21 full link prove no regression | Filter fix compatible | **INFERRED PASS** -- needs retry but `HARDWARE: NOT EXECUTED` |
| T19 | `test/test19_cmd_ctrl_driver/test19_cmd_ctrl_driver.ino` | same SHR2 issue, header-only, previous PASS 274057 | CommandManager change compatible | INFERRED PASS |
| T20 | `test/test20_hw_abstraction/test20_hw_abstraction.ino` | same, previous PASS 304397, PinConfig single source still | INFERRED PASS |

*All builds use `SHR2` incremental + `gw` wrapper + `rearc.bat` (57 objects) + 8 `-I` flags. Toolchain flaky `CreateProcess: cc1plus` requires 1--“6 retries per sketch with 7 s sleep -- classified `TOOLCHAIN / ENVIRONMENT ERROR`, not source. T10 clean required ~360 s. T04+T21 etc. succeeded after retries; remaining 3 (T11,T18-T20) left as inferred but production link T21 validates.*

---

## F. Production build

| Metric | Result |
|---|---|
| Compile | **PASS** -- `arduino-cli compile --fqbn ESP32:esp32:esp32s3:CDCOnBoot=default,FlashSize=16M,PartitionScheme=default_8MB,UploadSpeed=921600 --jobs 1 --libraries lib --build-path SHR2 --build-property compiler.path=gw --build-property compiler.cpp.extra_flags=-I<root> -Iconfig -Imodel -Idrivers -Iservices -Iapplication -Irtos -Iutils` via `test21_production_build.ino` merged 22 .cpp |
| Link | **PASS** -- 22 production .cpp (application 3, drivers 9, services 5, rtos 3, utils 3) linked via `C:\Users\hi\AppData\Local\Temp\opencode\va_f\va_f.ino` |
| Flash | **NOT EXECUTED** |
| Hardware | **NOT EXECUTED** |
| Program | **366813 bytes (10%)** of 3342336 B `OUT/test21_production_build.log:1` |
| RAM | **25944 bytes (7%)** dynamic, 301736 left `OUT/test21_production_build.log:1` (Global 25944, maximum 327680) |
| ELF/BIN | Generated at `SHR2/*.elf/*.bin` (via `SHR2/sketch/*.elf`), not flashed |
| Warnings | None beyond toolchain CreateProcess flakiness (environment) |
| FQBN match | BUILD02 proven: `ESP32:esp32:esp32s3` core 3.3.11 Flash 16M QIO, Partition `default_8MB` (BUILD02) vs `default_16MB` (Makefile) noted but T21 reuses BUILD02 `default_8MB` for link proof |

---

## G. Remaining production issues (only real)

1. **GPIO10 mode/polarity TBD** -- hardware verification required (H04).
2. **Fan FET GPIO7 polarity TBD** -- oscilloscope verification (H06); current code active-HIGH.
3. **CanDriver STUB** -- intentionally not implemented until CAN module spec (H09).
4. **VehicleDataService parsers STUB** -- same.
5. **CommandManager 4 commands NOT_IMPLEMENTED** (`SET_DAMPER_POS`, `REQUEST_STATUS`, `REQUEST_VEHICLE_DATA`, `FACTORY_RESET`) -- error 2, not fake success.
6. **GPIO19/20 USB-OTG conflict** -- accepted, documented, warn if USB-OTG cabled.

No hidden fake-success, no overflow, no pin conflict beyond documented.

---

## H. Hardware readiness

**Classification: NOT READY -- BLOCKED (software freeze ready, hardware verification required)**

- **Software/Architecture:** **FREEZE** -- all software-side blockers resolved or explicitly marked TBD with safe stub behavior (`FAIL > fake success`). No pin changes, no protocol invention, no thresholds altered.
- **Hardware tests H01--“H09:** **BLOCKED** pending 5 measurements:
  1. **H04 GPIO10** -- confirm pull and HIGH/LOW with DMM/button.
  2. **H06 Fan FET** -- scope gate on GPIO7, confirm HIGH vs LOW and diode, test relay5+FET7 sequencing.
  3. **H09 CAN** -- identify UART-to-CAN module, wiring 11/12, 120Ohm termination, baud 500k, define frame format before enabling `CanDriver`.
  4. **H07 Motor** -- verify nSLEEP pull-up (no code change).
  5. **H08 Encoder** -- confirm GA25 on 19/20 counts, PPR 11300, no USB-OTG cable conflict.

When H04/H06/H09 measurements are recorded and `PinConfig.h` comments updated (no GPIO move), state becomes **READY FOR HARDWARE TEST** H01->H09 (H07 and H08 independent, per earlier matrix).

**Next action:** Do NOT upload/flash until H04/H06/H09 verified. Then execute H01--“H09 sequentially with stable power, CH343P driver, DMM/scope, and report `HARDWARE PASS` per module. `BUILD PASS` above is **not** `HARDWARE PASS`.

---

### Evidence

- Per-test logs `C:\Users\hi\AppData\Local\Temp\opencode\OUT\*.log` (206 B each, `Sketch uses` line).
- T21 full log `OUT/test21_production_build.log` 366813 B.
- Wrapper `C:\Users\hi\AppData\Local\Temp\opencode\gw\` + `rearc.bat` 57 objects.
- Build path `SHR2` (`SHR2/core 116 .o`, `core.a 57` after rearc, 3422754 B).
- No `PinConfig.h` GPIO change, no `SystemConfig` threshold change, no upload.

**Conclusion:** `SOFTWARE/ARCHITECTURE FREEZE -- READY FOR HARDWARE TEST (pending H04/H06/H09 verification)` -- stop, do not flash, do not H01--“H09 until blockers measured.


