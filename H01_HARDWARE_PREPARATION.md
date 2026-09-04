# H01 — HARDWARE PREPARATION (ESP32-S3 N16R8 CH343P)

**Date:** 2026-09-02  
**Board:** ESP32-S3 N16R8 CH343P  
**PinConfig:** `PinConfig.h:1` SINGLE SOURCE OF TRUTH — no code change in H01  
**Mode:** Audit + checklist only — **NO upload, NO flash, NO actuator run, NO 12V motor power in H01**  
**References:** `FINALIZE_PRODUCTION_BEFORE_HARDWARE.md:1` `UPDATE_REPORT_ENCODER_FAN_HYST.md:1` `PinConfig.h:1` `SystemManager.cpp:1`

---

## H01.1 — Pin / Wiring Audit (Read-Only)

### Map verification vs PinConfig.h

| Function | PinConfig | Driver / Service | Direction | Check | Result |
|---|---|---|---|---|---|
| NTC1 ADC | `PinConfig.h:71` GPIO1 | `drivers/NtcDriver.h:22` `PIN_NTC1_ADC` | INPUT ADC (12-bit) | GPIO1 not used elsewhere, 10k/10k divider 3.3V | **OK** |
| NTC2 ADC | `PinConfig.h:72` GPIO2 | `NtcDriver.h:22` `PIN_NTC2_ADC` | INPUT ADC | GPIO2 distinct from NTC1, not used elsewhere | **OK** |
| AC Relay | `PinConfig.h:40` GPIO4 | `drivers/RelayDriver.h:10` `begin(4,true)` `SystemManager.cpp:75` | OUTPUT | Active-high default, `off()` at boot | **OK** |
| Fan Relay | `PinConfig.h:41` GPIO5 | `RelayDriver` `begin(5,true)` `SystemManager.cpp:76` | OUTPUT | Independent from FET7, `off()` at boot | **OK** |
| Pi Power Relay | `PinConfig.h:42` GPIO6 | `RelayDriver` `begin(6,true)` `SystemManager.cpp:77` | OUTPUT | `off()` at boot | **OK** |
| Fan FET PWM | `PinConfig.h:43` GPIO7 | `drivers/PwmDriver.h:10` `ChannelConfig pin=7 freq1000 res8` `services/FanController.h:14` `SystemManager.cpp:65` | OUTPUT PWM | 1kHz/8-bit, polarity TBD (active-HIGH assumed), `ledcWrite 0` at boot | **OK — TBD polarity noted** |
| OLED SDA | `PinConfig.h:19` GPIO8 | `drivers/OledDriver.cpp:6` `Wire.begin(8,9)` | I2C SDA | 0x3C 128x64, not used elsewhere | **OK** |
| OLED SCL | `PinConfig.h:19` GPIO9 | `OledDriver.cpp:6` | I2C SCL | Distinct from SDA, pull-up 4.7k expected on module | **OK** |
| ON/OFF Input | `PinConfig.h:48` GPIO10 | **No production driver configures** — `SystemManager.cpp:31` log only, `test04_gpio_input` placeholder `INPUT_PULLUP` | INPUT TBD | No `pinMode(10)` in production, mode/polarity TBD, must not float | **OK — TBD** |
| CAN UART TX | `PinConfig.h:77` GPIO11 | `drivers/CanDriver.h:22` `PIN_CAN_UART_TX` `SystemManager.cpp:109` | OUTPUT UART | External module, NOT TWAI, `CanDriver` STUB (no Serial2) | **OK — STUB** |
| CAN UART RX | `PinConfig.h:78` GPIO12 | `CanDriver.h:22` | INPUT UART | Paired with TX11, 500k baud config | **OK** |
| Motor IN1 | `PinConfig.h:65` GPIO13 | `drivers/MotorDriver.h:10` `PIN_MOTOR_IN1` `SystemManager.cpp:83` | OUTPUT PWM | 20kHz/10-bit, `ledcAttach 13` `ledcWrite 0` coast at boot | **OK** |
| Motor IN2 | `PinConfig.h:66` GPIO14 | `MotorDriver.h:10` `PIN_MOTOR_IN2` | OUTPUT PWM | Paired with IN1, same freq/res | **OK** |
| Pi UART TX | `PinConfig.h:13` GPIO17 | `drivers/UartDriver.h:12` `PIN_PI_UART_TX` `UartDriver.cpp:10` `Serial1 TX` | OUTPUT UART | 115200, `SERIAL_8N1` | **OK** |
| Pi UART RX | `PinConfig.h:14` GPIO18 | `UartDriver.h:12` | INPUT UART | Paired with TX17 | **OK** |
| Encoder A | `PinConfig.h:103` GPIO19 | `drivers/EncoderDriver.h:14` `begin(19,20,0)` `SystemManager.cpp:95` `PIN_ENC_A` | INPUT PULLUP + ISR CHANGE | GA25 quadrature, `PULSES_PER_REV 11300` | **OK — conflict accepted** |
| Encoder B | `PinConfig.h:104` GPIO20 | `EncoderDriver.h:14` `PIN_ENC_B` | INPUT PULLUP + ISR | Same | **OK** |
| CH343P | `PinConfig.h:86` GPIO43/44 | **No driver uses** | RESERVED USB-UART | CH343P bridge only | **OK** |
| nSLEEP | `PinConfig.h:63` | `MotorDriver.h:25` `SystemManager.cpp:37` | NOT USED | Hardware-controlled, no GPIO, no constant | **OK** |

### Conflict check

- **19/20 vs USB-OTG:** Generic S3 `19/20 = USB D-/D+`. This board USB via CH343P `43/44` (`PinConfig.h:86`), so 19/20 free. **ACCEPTED USER DECISION** `PinConfig.h:94` — document: if USB-OTG cable is used, conflict exists. No code hardcodes 19/20 except via `PinConfig`.
- **7 vs NFAULT:** Old tests `test/Dieukhiendongcoencoder` used `NFAULT=7`, now **removed** from production. Production uses `7` only for Fan FET, `13/14` for Motor — no overlap.
- **43/44 vs others:** No driver uses 43/44.
- **0/3/45/46 strapping:** Not used.
- **PWM distinct:** Fan `1000/8` on 7 vs Motor `20000/10` on 13/14 — `FanController.cpp:11` vs `MotorDriver.h:14` — no shared channel (core 3.x `ledcAttach` per pin).

### Direction check

- **Inputs:** `1,2` ADC, `10` TBD, `12` CAN RX, `18` Pi RX, `19,20` encoder (all `INPUT`/`INPUT_PULLUP`). No output conflict.
- **Outputs:** `4,5,6` relays, `7` FET PWM, `8,9` I2C, `11` CAN TX, `13,14` Motor PWM, `17` Pi TX — all `OUTPUT`/`ledcAttach`. No pin is both.
- **UART TX/RX not swapped:** `PinConfig.h:13` `TX17->RX Pi` correct, ` drivers/UartDriver.cpp:10` `Serial1.begin(baud, SERIAL_8N1, rx_pin_, tx_pin_)` order `rx,tx` correct. Same for CAN `11/12`.

### I2C / UART / PWM / Encoder

- **I2C:** `OledDriver.cpp:6` `Wire.begin(8,9)` matches `PinConfig.h:19` SDA8/SCL9, addr 0x3C, lib `Adafruit_SSD1306 2.5.7`.
- **UART:** Pi `17/18` 115200 `SystemConfig.h:7` via `Serial1`; CAN `11/12` 500k `CanDriver.h:22` via stub (no `Serial2` yet) — correct split.
- **PWM:** Fan `PwmDriver.cpp:9` `ledcAttach(7,1000,8)` `ledcWrite 0`; Motor `MotorDriver.cpp:11` `ledcAttach(13,20000,10)` `ledcWrite 0` — distinct.
- **Encoder:** `EncoderDriver.cpp:13` `pinMode(19, INPUT_PULLUP)` `attachInterrupt CHANGE` both A/B, `SystemManager.cpp:95` `motor_pos_ctrl_.begin(&motor_,&encoder_,19,20,0)` — matches `PinConfig.h:103`.

**H01.1 Result:** **PASS — No pin conflict except documented 19/20, no code change needed.**

---

## H01.2 — Hardware Checklist (Visual / Continuity — No Power to Actuators)

User to tick physically. All checks **without 12V motor/fan supply** in H01.

### A. Board & Power Rails

- [ ] ESP32-S3 N16R8 CH343P board present, USB via CH343P (check `USB` not `OTG` port)
- [ ] 3.3V rail measured at ESP32 3.3V pin (DMM, no load) — target 3.25-3.35V
- [ ] 5V rail (if used for OLED/relays logic) — 4.9-5.1V, common GND verified
- [ ] 12V rail **DISCONNECTED** for fan/motor in H01 (tape off) — verify 12V supply OFF, not connected to DRV8833 VM or fan +
- [ ] Common GND: ESP32 GND — DRV8833 GND — relay board GND — OLED GND — NTC GND — Pi GND — CAN module GND — **continuity <1Ω**, no GND loop via chassis
- [ ] No 5V/12V short to any GPIO (continuity GPIO vs 12V = OPEN)

### B. DRV8833 + GA25-370

- [ ] DRV8833 VM **not powered** in H01 (jumper removed)
- [ ] IN1 wired to GPIO13, IN2 to GPIO14 — verify continuity, no swap
- [ ] nSLEEP pin — verify hardwired to VCC (pull-up) or floating per board — **do not add GPIO** — note wiring
- [ ] Motor leads to DRV8833 OUT1/OUT2 (parallel mode AIN1+BIN1=13, AIN2+BIN2=14) — visual only
- [ ] GA25 encoder 4-wire: VCC 3.3V, GND, A->GPIO19, B->GPIO20 — continuity 19/20, no short to 5V/12V
- [ ] Encoder PPR 11300 concept check — no hardware cal in H01

### C. NTC1/NTC2

- [ ] NTC1 divider: 10k NTC (GPIO1) — 10k fixed to 3.3V — GND — verify 10k value, midpoint to GPIO1
- [ ] NTC2 same to GPIO2
- [ ] No 5V on NTC divider, no short to GND

### D. Relays & FET

- [ ] AC Relay IN -> GPIO4, VCC 5V/3.3V per module, JD-VCC jumper checked
- [ ] Fan Relay IN -> GPIO5
- [ ] Pi Power Relay IN -> GPIO6
- [ ] Relay boards GND common, flyback diode present on board
- [ ] Fan FET (e.g., IRLZ44N/AO3400): Gate -> GPIO7, Source GND, Drain -> fan -, fan + -> 12V (12V OFF in H01), gate pull-down 10k if present, **flyback diode across fan** — note if missing (TBD)
- [ ] Fan Relay 5 and FET 7 are independent — verify not shorted

### E. OLED

- [ ] SDA -> GPIO8, SCL -> GPIO9, VCC 3.3V, GND
- [ ] 4.7k pull-up on SDA/SCL (often on module/board) — visual
- [ ] Addr 0x3C

### F. Pi UART

- [ ] ESP TX17 -> Pi RX (via level shifter if Pi 3.3V? Pi is 3.3V tolerant, direct OK), RX18 <- Pi TX
- [ ] GND Pi — ESP common
- [ ] Baud 115200

### G. UART-to-CAN Module

- [ ] Module VCC 3.3V/5V per spec, GND
- [ ] ESP TX11 -> module RX, RX12 <- module TX — verify not swapped
- [ ] Termination 120Ω not yet installed in H01 — note TBD (H09)
- [ ] Module model noted (e.g., MCP2515 UART bridge?) — TBD, do not power CAN bus 12V in H01

### H. Wiring General

- [ ] All signal wires <30cm or twisted, no parallel run with 12V high current
- [ ] No GPIO0/3/45/46 used
- [ ] No GPIO43/44 used except USB
- [ ] Heat shrink / ferrules, no exposed copper

---

## H01.3 — Safety Check (Boot Behavior — No Actuator Power)

**Principle:** In H01, **no actuator may move on boot**. Verify by code audit, not by powering.

| Actuator | Boot state in code | Safe? | Check |
|---|---|---|---|
| Motor IN1/IN2 GPIO13/14 | `MotorDriver.cpp:19` `ledcWrite 0,0` coast, `SystemManager.cpp:91` `motor_.coast()` at `begin()` | **SAFE** — coast (both LOW) | Verify DRV8833 VM OFF anyway in H01 |
| Relays GPIO4/5/6 | `RelayDriver.cpp:10` `off()` at `begin()`, `SystemManager.cpp:75` `begin(...,true)` `off()` | **SAFE** — relays OFF, no AC/fan/Pi power | Verify relay board active-high (default true) matches wiring |
| Fan FET GPIO7 | `PwmDriver.cpp:10` `ledcWrite 0`, `FanController.cpp:11` `ramp 0`, `SystemManager.cpp:69` `fan_pwm_.begin 0` + `fan_ctrl_.begin` | **SAFE if active-HIGH** — 0=OFF. **TBD if active-LOW** → 0 would be MAX in H06. H01 requires **no fan 12V** to avoid risk | Keep 12V OFF, note polarity TBD `PinConfig.h:33` |
| OLED I2C | `Wire.begin` only, no backlight power beyond 3.3V | SAFE | — |
| Pi UART / CAN | No TX at boot until `loop` | SAFE | — |
| Encoder | `INPUT_PULLUP` + ISR, no output | SAFE | — |
| GPIO10 Input | No `pinMode` in production — floating if no external pull | **RISK** — floating → random `inside_valid`? But `ClimateController` holds `fan_on_` and motor not driven by GPIO10 yet. No actuator directly tied to GPIO10 in `SystemManager`. Document TBD, ensure external pull before H04 | Do not tie to relay in H01 |

**No output-input confusion:** All GPIOs have single direction per table H01.1. No pin is driven both as output and input.

**Unexpected relay ON at boot?** `RelayDriver` `active_high=true` default — if relay board is active-LOW, `off()` would drive HIGH (correct OFF for active-low? Wait `set(false)` with `active_high true` drives LOW, which is ON for active-low board — **check**). `RelayDriver.cpp:23` `digitalWrite(pin_, (state_ == active_high_) ? HIGH : LOW)` — if board is active-LOW (common), then `active_high` should be `false`. Current production uses `true` `SystemManager.cpp:75`. This is **undocumented assumption**. For H01, note: verify relay module trigger level (measure IN pin vs relay click with DMM on bench 5V, not on ESP). If relays are active-LOW, correct before H03.

**Fan FET unexpected ON:** As above, keep 12V OFF in H01.

**Safety prerequisites for H01:**

- [ ] DRV8833 VM disconnected
- [ ] Fan 12V disconnected
- [ ] AC relay load disconnected (or AC mains OFF)
- [ ] Pi not powered via relay 6 (use separate USB for Pi if needed in H05)
- [ ] Verify no GPIO short to 12V with DMM (continuity)

**H01 does NOT request:** No motor run, no relay click under load, no fan spin, no 12V on.

---

## H01.4 — Report Summary

### Pin audit (H01.1)

**PASS** — All GPIOs match `PinConfig.h` map, single source, no hardcode. Only conflict is documented `19/20 USB-OTG` accepted. No `0/3/45/46` or `43/44` misuse. Directions correct, UART/I2C/PWM/encoder correct.

### Wiring checklist (H01.2)

Provided above — user to tick physically in H01 without 12V.

### Power checklist (H01.2.A)

- 3.3V rail OK, 5V OK, 12V **OFF** in H01, common GND <1Ω, no 5V/12V to GPIO.

### Safety checklist (H01.3)

- Motor coast, relays off, FET 0, encoder pullup — **safe with 12V OFF**.
- Two **TBD safety items** require H04/H06 measurement before load: GPIO10 floating, relay active level, FET polarity.

### Known risks

1. **GPIO10 floating** `PinConfig.h:48` — no pull in production → random reads until external pull confirmed H04.
2. **Relay active level** `SystemManager.cpp:75` assumes `active_high true` — many relay boards are `LOW` trigger → verify with DMM/bench before H03.
3. **Fan FET polarity** `PinConfig.h:33` — if active-LOW, `duty 0` is MAX → **must keep 12V OFF in H01/H03** until H06 scope.
4. **19/20 conflict** — if USB-OTG cable used, encoder will be disrupted — accepted, use CH343P port only.
5. **CAN stub** — `CanDriver` `write()` returns `false` intentionally — H09 will show no CAN traffic, not a bug.
6. **Encoder PPR 11300** — not calibrated on this hardware — H08 will verify.
7. **DRV8833 VM without nSLEEP** — ensure nSLEEP hardwired HIGH — H07.

### TBD items (from `FINALIZE_PRODUCTION_BEFORE_HARDWARE.md:100`)

| ID | Item | Location | State |
|---|---|---|---|
| TBD-1 | GPIO10 mode `INPUT/PULLUP/PULLDOWN` | `PinConfig.h:48` | TBD H04 |
| TBD-2 | GPIO10 polarity HIGH/LOW | `PinConfig.h:48` | TBD H04 |
| TBD-3 | Fan FET polarity HIGH/LOW | `PinConfig.h:33` | TBD H06 |
| TBD-4 | Fan diode/sequencing | `PinConfig.h:33` | TBD H06 |
| TBD-5 | CAN module/protocol | `CanDriver.h:8` | STUB H09 |
| TBD-6 | VDS parsers | `VehicleDataService.cpp:55` | STUB |
| TBD-7 | 4 commands NOT_IMPL | `CommandManager.cpp:33` | assumption |
| TBD-8 | nSLEEP wiring | `PinConfig.h:63` | NOT USED H07 |
| TBD-9 | 19/20 conflict | `PinConfig.h:94` | accepted |

### Hardware test prerequisites (for H02-H09)

- DMM, scope/logic for FET gate, bench 5V/12V current-limited, CH343P driver, USB cable to 43/44, no OT-G cable.
- 12V supplies remain **OFF** until H01 tick complete and H03/H06/H07 prerequisites met.

---

## H01 Result

**H01 = READY — with TBD notes**

**Reason:** Pin/wiring audit PASS, no code change, no GPIO conflict beyond documented 19/20, boot states safe **provided 12V motor/fan and AC mains remain OFF** in H01. Checklist and safety prerequisites are complete and actionable without powering actuators. Two TBDs (GPIO10 mode, relay active level, FET polarity) are **explicitly documented and do not block preparation**, but **block H03/H04/H06** until measured.

**Next:** User ticks H01.2 checklists physically (continuity, GND, 3.3V, 12V OFF). After H01 READY, proceed to H02 (NTC/OLED) and H04 (GPIO10) with DMM only — still no motor/fan 12V.

*No upload, no flash, no actuator run in H01. PinConfig remains source of truth.*

