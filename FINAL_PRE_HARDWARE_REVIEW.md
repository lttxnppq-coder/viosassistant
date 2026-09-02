# FINAL PRE-HARDWARE REVIEW -- ViosAssistant (READ-ONLY)

**Date:** 2026-08-31  
**Scope:** Toàn bộ T01--T24, production source, PinConfig, T21 config, unresolved HW decisions  
**Mode:** READ / AUDIT / DIFF / REPORT -- **KHÔNG build lại, KHÔNG upload/flash, KHÔNG cấp nguồn, KHÔNG sửa production**

---

## A. TEST01--TEST24 Verification (vs Production API thực tế)

| ID | File | Production API | Kiểm tra đúng mục tiêu? | Thông số thực nghiệm | Compile/Static? | BUILD vs HARDWARE |
|---|---|---|---|---|---|---|
| T01 | `test/oled_test/oled_test.ino:1` | `drivers/OledDriver.h:1` `OledDriver.cpp:6` `Wire.begin(SDA8,SCL9)` | YES -- SSD1306 128x64 0x3C | SDA8/SCL9, 0x3C, lib 2.5.7/1.11.9/1.17.4 | Compile+link, no OLED hardware | BUILD PASS `C:\Users\hi\AppData\Local\Temp\opencode\OUT\oled_test.log:1` != HARDWARE PASS |
| T02 | `test/ntc_test/ntc_test.ino:1` | `drivers/NtcDriver.h:22` `NtcDriver.cpp:1` | YES -- NTC ADC | 10k B3950 10k series 3.3V 12-bit, PIN1/2 `PinConfig.h:57` | Compile | BUILD PASS 352026 B != HARDWARE |
| T03 | `test/test03_gpio_output/test03_gpio_output.ino:1` | `drivers/RelayDriver.h:1` + `PinConfig.h:34` | YES -- Relay/FET output | Relays 4/5/6, Fan FET 7, AC 4 | Compile | PASS 302730 B |
| T04 | `test/test04_gpio_input/test04_gpio_input.ino:1` | `PinConfig.h:41` `INPUT_PULLUP` | YES -- GPIO10 input | PIN10, mode TBD preserved as `INPUT_PULLUP` placeholder, **không sửa PinConfig** | Compile | PASS 302322 B |
| T05 | `test/test05_pi_uart/test05_pi_uart.ino:1` | `drivers/UartDriver.h:12` `PIN_PI_UART_TX 17/RX18` | YES -- Pi UART | 115200 `SystemConfig.h:7` | Compile | PASS 274265 B |
| T06 | `test/test06_pwm_fan/test06_pwm_fan.ino:1` | `drivers/PwmDriver.h:11` `FanController.cpp:11` | YES -- Fan FET PWM | **1kHz/8-bit GPIO7** distinct vs Motor | Compile | PASS 281792 B |
| T07 | `test/test07_motor/test07_motor.ino:1` | `drivers/MotorDriver.h:11` `MotorDriver.cpp:6` | YES -- DRV8833 parallel IN1/IN2 | **20kHz/10-bit GPIO13/14**, delay đổi chiều 75ms, Config `pwm_freq 20000 res10` | Compile | PASS 312009 B |
| T08 | `test/test08_encoder/test08_encoder.ino:1` | `drivers/EncoderDriver.h:11` | YES -- Encoder | PULSES_PER_REV 11300, pins **TBD** không gán | Compile | PASS 273925 B |
| T09 | `test/test09_can_uart/test09_can_uart.ino:1` | `drivers/CanDriver.h:11` `PIN_CAN_UART_TX 11/RX12` | YES -- UART-to-CAN module | UART 500k, **KHÔNG phải native TWAI** | Compile | PASS 274453 B |
| T10 | `test/test10_filter/test10_filter.ino:1` | `utils/Filter.h:1` `Filter.cpp:1` | YES -- Moving/Exp/Median unit | window10 α0.2, Median `sorted[15]` note | Compile+static assert | PASS 275165 B |
| T11 | `test/test11_crc16/test11_crc16.ino:1` | `utils/Crc16.h:9` `POLY 0xA001 INIT FFFF` | YES -- CRC16 | Poly A001 | Compile | PASS 274305 B |
| T12 | `test/test12_vehicle_data/test12_vehicle_data.ino:1` | `services/VehicleDataService.h:1` `VehicleData.h:1` | YES -- VehicleDataService | parse stubs, need `-I<root>` | Compile | PASS 274673 B |
| T13 | `test/test13_motor_position/test13_motor_position.ino:1` | `services/MotorPositionController.h:1` | YES -- MotorPosition | PID kp1 ki0 kd0, error<10 | Compile | PASS 289884 B |
| T14 | `test/test14_fan_controller/test14_fan_controller.ino:1` | `services/FanController.h:11` `FanController.cpp:6` + `PwmDriver.cpp` | YES -- FanController software | ramp 10 default, `begin(nullptr)` | Compile (include cpp) | PASS 276336 B |
| T15 | `test/test15_command_manager/test15_command_manager.ino:1` | `application/CommandManager.h:1` `Command.h:1` | YES -- queue 16 | overflow 17th fail, `processCommand` stub | Compile | PASS 274433 B |
| T16 | `test/test16_response_manager/test16_response_manager.ino:1` | `application/ResponseManager.h:1` `Crc16.h` | YES -- ResponseManager | `begin(nullptr)` null-safe, **đã fix testcase `SystemMode::AUTO->NORMAL` `SystemError->ErrorCode` (`model/SystemState.h:7` vs test:22)** | Compile | PASS 274401 B |
| T17 | `test/test17_system_manager/test17_system_manager.ino:1` | `application/SystemManager.h:1` | YES -- SystemManager integration | **header-only** (tránh link `FanController::begin` undefined) | Compile/static | PASS 304285 B |
| T18 | `test/test18_sensor_service_app/test18_sensor_service_app.ino:1` | `drivers/NtcDriver.h` -> `services/ClimateController.h` -> `SystemState.h` | YES -- Sensor->Service->App chain | NTC->Climate->SystemState | Compile/static | PASS 304329 B |
| T19 | `test/test19_cmd_ctrl_driver/test19_cmd_ctrl_driver.ino:1` | `CommandManager->MotorPosition->MotorDriver` | YES -- Command->Controller->Driver | **header-only** (tránh `delay/millis/app_main` link) | Compile/static | PASS 274057 B |
| T20 | `test/test20_hw_abstraction/test20_hw_abstraction.ino:1` | `PinConfig.h:1` + all drivers | YES -- HAL abstraction | PWM distinct check, reserved pins | Compile/static | PASS 304357 B |
| T21 | `test/test21_production_build/test21_production_build.ino:1` + `C:\Users\hi\AppData\Local\Temp\opencode\va_f\va_f.ino:1` | **All 22 .cpp** (app3+drivers9+services5+rtos3+utils3) | YES -- **FULL PRODUCTION LINK** | FQBN 16M/PSRAM8M/qio | BUILD02 merged sketch | **PASS 366513 B (10%)** |
| T22 | `test/test22_pin_config_audit/test22_pin_config_audit.ino:1` | `PinConfig.h:1` audit | YES -- Pin single source | All pins vs `PinConfig.h:10` | Compile/audit | PASS 274089 B |
| T23 | `test/test23_dependency_audit/test23_dependency_audit.ino:1` | All drivers/services/app | YES -- Include graph | 8 `-I` paths, pragma once | Compile/audit | PASS 304421 B |
| T24 | `test/test24_architecture_audit/test24_architecture_audit.ino:1` | `rtos/*` -> `application/*` | YES -- Layering | drivers->services->app->rtos | Compile/audit | PASS 304705 B |

**Thông số thực nghiệm không thay đổi:** Motor 20kHz/10-bit `drivers/MotorDriver.h:14`, Fan 1kHz/8-bit `services/FanController.cpp:11`, delay 75ms hard-coded in T07, NTC 10k/B3950, UART 115200 `SystemConfig.h:7`, CAN 500k `drivers/CanDriver.h:11`, PULSES_PER_REV 11300 T08, Encoder TBD `PinConfig.h:85`, CRC16 A001 `utils/Crc16.h:9`. **T01/02 giữ nguyên tên** `test/oled_test`, `test/ntc_test`.

**Compile/static test:** T10-T24 đều là compile/static/integration -- chưa có `HARDWARE PASS`; `BUILD PASS` hiện tại chỉ chứng minh toolchain + include + link, **không gọi là TEST PASS**.

---

## B. Production Source Integrity (READ-ONLY DIFF)

**Kết quả:** **PASS -- Không có thay đổi production trong quá trình T01--T24.**

Dựa trên `LastWriteTime` (dir `C:\Users\hi\Documents\NCKH\main\ViosAssistant\`):

- `model/SystemState.h:1` -- `2026-08-22 20:52` -- **không sửa** `SystemMode` (`OFF/INITIALIZING/NORMAL/CLIMATE_AUTO…`) và `ErrorCode` (`NONE…`) như review lo ngại. Log T16 `AUTO->NORMAL` và `SystemError->ErrorCode` là **fix trong testcase** `test/test16_response_manager/test16_response_manager.ino:22` (`model::SystemState st; st.mode=model::SystemMode::NORMAL; st.error=model::ErrorCode::NONE;`), **không thay đổi** `model/SystemState.h:7`. Nếu production từng chứa `AUTO`/`SystemError` thì hiện tại đã ở trạng thái đúng và không bị ghi đè thêm.
- Toàn bộ production: `PinConfig.h:1` 2026-08-30 13:24, `application/SystemManager.h:1` 2026-08-30 13:24, `application/SystemManager.cpp:1` 2026-08-30 13:25, `config/SystemConfig.h:1` 2026-08-22, `drivers/*`, `services/*`, `utils/*` đều 2026-08-22--08-30, **không có timestamp 2026-08-31** (ngày tạo T14--T24). Các file test mới đều 2026-08-31.
- `NEED REVIEW` = **Không** -- không file production nào cần revert.

Nếu sau này phát hiện production bị sửa (ví dụ `model/SystemState.h` từng bị đổi) -> ghi `NEED REVIEW` nhưng hiện tại DIFF trống.

---

## C. Pin/Config Verification (PinConfig.h:1 vs drivers/services/application)

| Chức năng | PinConfig | Driver/Service | Conflict? |
|---|---|---|---|
| NTC1/2 | `PinConfig.h:57` 1/2 | `drivers/NtcDriver.h:22` `PIN_NTC1_ADC` | OK |
| OLED | `PinConfig.h:19` SDA8 SCL9 0x3C 128x64 | `drivers/OledDriver.cpp:6` `Wire.begin(SDA,SCL)` | OK |
| INPUT ON/OFF | `PinConfig.h:41` GPIO10 TBD | `drivers` không hardcode, test T04 dùng `PIN_ON_OFF_INPUT` | OK, TBD giữ |
| CAN UART | `PinConfig.h:63` TX11/RX12 | `drivers/CanDriver.h:11` `PIN_CAN_UART_TX/RX` | OK |
| Motor | `PinConfig.h:51` IN1 13 IN2 14 | `drivers/MotorDriver.h:11` `PIN_MOTOR_IN1/IN2` | OK |
| Pi UART | `PinConfig.h:13` TX17/RX18 | `drivers/UartDriver.h:12` `PIN_PI_UART_TX/RX` | OK |
| Fan FET | `PinConfig.h:36` GPIO7 | `services/FanController.h:11` `PIN_FAN_FET_PWM` `FanController.cpp:11` 1kHz/8-bit | OK |
| AC/Fan/Pi Relay | `PinConfig.h:34` 4/5/6 | `drivers/RelayDriver.h:1` | OK |
| CH343P | `PinConfig.h:72` 43/44 RESERVED | Không driver nào dùng 43/44 | OK |
| Encoder | `PinConfig.h:85` TBD commented | `drivers/EncoderDriver.h:11` `TODO pending, no defaults` | OK -- **không gán** |
| Strapping 0/3/45/46, USB 19/20 | `PinConfig.h:77` DO NOT USE | Không driver dùng | OK |

**Kết luận Pin:** **No conflict.** Tất cả drivers dùng `PIN_*` macro, không hardcode số. PWM configs tách biệt `FanController.cpp:11` vs `MotorDriver.h:14`.

---

## D. T21 Configuration Verification (vs BUILD02 / Makefile / ESP32-S3)

| Mục | BUILD02 (`C:\Users\hi\AppData\Local\Temp\opencode\gwbuild.bat:7` / `va_f\va_f.ino:1`) | Makefile (`Makefile:21`) | T21 `test/test21_production_build/test21_production_build.ino:1` | Match? |
|---|---|---|---|---|
| Chip/Board | `ESP32:esp32:esp32s3` | `CHIP=esp32 BOARD=esp32s3` `CHIP esp32` | `ESP32:esp32:esp32s3` | **YES** |
| Core | 3.3.11 `hardware/esp32/3.3.11` | `ESP_ROOT …/3.3.11` | 3.3.11 | YES |
| Flash | 16M `FlashSize=16M` | `FLASH_SIZE 16MB QIO 80m` | 16M | YES |
| PSRAM | 8MB (via build) | `PSRAM_TYPE opi`, `default_16MB`, `BOARD_HAS_PSRAM` | 8MB implicit | YES |
| Partition | `default_8MB` (BUILD02) vs `default_16MB` (Makefile) | `PARTITIONS default_16MB` | `default_8MB` (BUILD02) | **DIFF nhỏ** -- BUILD02 dùng `default_8MB` (8M flash), Makefile dùng `default_16MB` cho 16M. Cả hai đều QIO, không ảnh hưởng link; **T21 đã REUSE BUILD02** nên giữ `default_8MB` để khớp BUILD02 đã PASS. |
| CDC | `CDCOnBoot=default` | `CH343P primary, NOT CDC` (`Makefile:39`) | `CDCOnBoot=default` | YES (BUILD02 default) |
| Upload | 921600 (BUILD02) | 921600 (`Makefile:102`) | 921600 | YES |
| Compiler | `compiler.path=gw` (gwrap2) | makeEspArduino | `compiler.path=gw` | YES -- T21 reuse |
| Include | 7 dirs (BUILD02 thiếu root) | `INCLUDE_DIRS config model drivers services application rtos utils` (`Makefile:45`) | 8 dirs **+ root** (`runone.bat:12` root added) | **DIFF**: T21 thêm `-I<root>` để `PinConfig.h:1` tìm thấy (vì `PinConfig.h` ở root). BUILD02 gốc thiếu root nhưng va_f dùng absolute `#include "C:/.../PinConfig.h"` nên vẫn PASS; T21 thêm root là **cải tiến tương thích**, không sai. |

**T21 khác BUILD02 ở đâu?** Chỉ 2 điểm không ảnh hưởng link: (1) partition `default_8MB` vs `default_16MB` -- giữ BUILD02; (2) thêm `-I<root>` -- cần thiết. **Không tự sửa**, ghi nhận.

**T21 chứng minh full link:** merged sketch `#include` 22 production `.cpp` absolute -> 366513 B (10%) `C:\Users\hi\AppData\Local\Temp\opencode\OUT\test21_production_build.log:1` -- **không phải compile root đơn giản**.

---

## E. Unresolved Hardware Decisions (cần xác nhận trước H01--H09)

1. **Encoder pins TBD** -- `PinConfig.h:85` `#define PIN_ENC_A TBD` commented, `drivers/EncoderDriver.h:11` yêu cầu explicit pins. **BLOCKED** cho H08 và H07 PID ghép. Cần quyết định GPIO (tránh 43/44, 19/20, strapping).
2. **GPIO10 input mode TBD** -- `PinConfig.h:42` `INPUT/INPUT_PULLUP/INPUT_PULLDOWN TBD`. Test T04 dùng `INPUT_PULLUP` tạm. Cần xác nhận hardware kéo lên/xuống.
3. **Motor nSLEEP** -- `drivers/MotorDriver.h:25` `nSLEEP hardware-controlled / pending electrical verification`. Chưa rõ chân sleep (pull-up hay GPIO điều khiển).
4. **Fan FET polarity** -- `PinConfig.h:29` Fan FET GPIO7 PWM, chưa xác định active HIGH/LOW và flyback diode.
5. **CAN physical interface** -- `drivers/CanDriver.h:10` UART-to-CAN module (500k), không phải TWAI native. Chưa rõ module cụ thể (MCP2515 UART bridge?) và termination 120Ohm.
6. **Relay active level** -- `drivers/RelayDriver.h` chưa rõ HIGH/LOW trigger cho AC/Fan/Pi relays 4/5/6.
7. **NTC calibration** -- B3950 10k divider, chưa có bảng tra ADC->°C thực nghiệm.
8. **OLED I2C pull-up** -- SDA8/SCL9 cần 4.7k? CH343P 43/44 không dùng cho OLED.
9. **Watchdog & PSRAM** -- `SystemConfig.h:12` WATCHDOG 5000ms, PSRAM OPI 8MB `Makefile:30` cần test stability.
10. **Power** -- Fan relay 5 + FET 7 độc lập, chưa rõ sequencing.

---

## F. Production Issues (xác nhận lại, KHÔNG sửa)

- `utils/Filter.h:1` Median `float sorted[15]` -- overflow nếu `window>15`.
- `drivers/CanDriver.cpp:1` stub, `services/VehicleDataService.cpp:55` `parseCanFrame/parseUartFrame` return false stub, `application/CommandManager.cpp:23` `processCommand` luôn success stub, `rtos/CommunicationTask.cpp:1` stub.
- `PinConfig.h:85` Encoder TBD, `PinConfig.h:42` input mode TBD.
- PWM distinction `services/FanController.cpp:11` 1kHz/8-bit vs `drivers/MotorDriver.h:14` 20kHz/10-bit.
- Không sửa -- ghi vào `FINAL_REPORT_T01-T24.md: G`.

---

## G. Hardware Readiness

**Hiện trạng:** Tất cả T01--T21 BUILD PASS, nhưng **chưa có H01--H09 nào HARDWARE PASS**. `SHR2` core 59 objects, `rearc` ổn định, nhưng toolchain vẫn cần 4-10 retries do `cc1plus CreateProcess` -> cần test hardware với nguồn ổn định và không build đồng thời.

**Sẵn sàng:** Power, wiring checklist, CH343P driver, serial monitor, multimeter, PSRAM test, relay board, NTC divider, OLED, UART Pi (level shifter?), CAN module, motor DRV8833 + encoder (khi có pins).

**Chưa sẵn sàng:** Encoder pins, input mode, nSLEEP, fan polarity, CAN termination -- BLOCKED.

---

## H. H01--H09 Hardware Test Matrix (chưa chạy)

| H | Module | Testcase | Pin | I/O | Thiết bị phụ trợ | Expected | PASS | FAIL | Dependency | Safety |
|---|---|---|---|---|---|---|---|---|---|---|
| H01 | OLED | T01 | SDA8/SCL9 0x3C | I2C out | OLED 128x64, 3.3V | Display init, no I2C NACK | No display / NACK | Wire, 4.7k pull-up | 3.3V only |
| H02 | NTC | T02 | ADC1/2 (GPIO1/2) | ADC in | NTC 10k +10k divider, 3.3V, DMM, temp chamber | ADC 0-4095 -> °C via B3950, valid flag | ADC stuck / 0 or 4095 | 12-bit, no 5V | Không cấp 5V vào ADC |
| H03 | GPIO OUTPUT | T03 | Relay 4/5/6, FET7 | Digital out | Relay board, DMM, 12V fan supply | Relay clicks, FET PWM duty | No click / short | PSRAM off | Cách ly 12V |
| H04 | GPIO INPUT | T04 | GPIO10 | Digital in | Button + 10k pull-up/down, DMM | `digitalRead` reflects level, mode TBD | Floating / always HIGH | Input mode quyết | Không để floating |
| H05 | Pi UART | T05 | TX17/RX18 115200 | UART | Pi4, USB-TTL, level shifter, scope | TX->RX loop, `uartAvailable` | Framing error / no RX | CH343P 43/44 reserve | 3.3V level |
| H06 | Fan PWM | T06 | FET7 1kHz/8-bit | PWM out | FET + fan 12V, scope/DMM, relay5 | Duty 0-255 -> speed ramp 10, enable | No PWM / freq sai | Separate from Motor | Flyback diode, relay5 ON trước PWM |
| H07 | Motor | T07 | IN1 13/IN2 14 20kHz/10-bit | PWM out | DRV8833 parallel, motor, scope, bench supply | Forward/reverse 75ms delay, brake/coast | Shoot-through / no move | nSLEEP verify | Current limit, không stall lâu |
| H08 | Encoder | T08 | TBD | PCNT in | Encoder hardware (11300 PPR), scope | `getPosition` counts, PPR correct | No count / noise | **Phải test riêng trước PID** | Không gán khi TBD |
| H09 | CAN | T09 | TX11/RX12 500k | UART | UART-to-CAN module, CAN analyzer, 120Ohm term | Module TX->RX, `CanDriver.write` | No frame / baud mismatch | Module spec, termination | 12V CAN supply isolate |

**Nguyên tắc:** Motor và Encoder **test riêng** trước ghép `MotorPositionController` PID (`services/MotorPositionController.cpp:31` kp1). CAN UART **không gọi là CAN BUS PASS** -- chỉ module interface. Không gọi bất kỳ compile `BUILD PASS` nào là `HARDWARE PASS`.

---

## Kết luận

**BLOCKED -- NEED DECISION/FIX**

Lý do BLOCKED (cần bạn xác nhận trước khi cấp nguồn/chạy H01--H09):

1. **Encoder GPIO assignment** -- `PinConfig.h:85` TBD -> chọn GPIO trống (tránh 43/44, 19/20, 0/3/45/46) và cập nhật `PinConfig.h`.
2. **GPIO10 input mode** -- `PinConfig.h:42` TBD -> xác định `INPUT` vs `PULLUP` vs `PULLDOWN` theo mạch nút.
3. **Motor nSLEEP** -- `drivers/MotorDriver.h:25` pending -> xác định chân / wiring (pull-up cứng hay GPIO).
4. **Fan FET polarity & termination** -- xác nhận HIGH active và diode.
5. **CAN module model + termination** -- xác định module UART-to-CAN cụ thể.
6. **Partition** `default_8MB` (BUILD02) vs `default_16MB` (Makefile) -- xác nhận dùng `default_16MB` cho 16M flash khi flash production (T21 hiện dùng 8MB).

Khi 6 quyết định trên được xác nhận (và cập nhật `PinConfig.h` nếu cần, **không sửa logic production khác**), trạng thái chuyển **READY FOR HARDWARE TEST** theo thứ tự H01->H09, với H07 và H08 riêng biệt.

**DỪNG TẠI REVIEW -- không build lại, không upload, không test hardware.**

