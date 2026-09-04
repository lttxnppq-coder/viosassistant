# H02 Jetson Nano ↔ ESP32-S3 Protocol Design

**Date:** 2026-09-02  
**Mode:** READ-ONLY DESIGN — NO production code/config change, NO build, NO flash, NO GPIO change  
**Baseline:** `H01_5_JETSON_ESP32_PROTOCOL_AUDIT.md:1` + `PinConfig.h:1` `config/ProtocolConfig.h:1` `config/SystemConfig.h:1` `model/Command.h:1` `drivers/UartDriver.{h,cpp}:1` `application/{CommandManager,SystemManager,ResponseManager}.{h,cpp}:1` `rtos/CommunicationTask.{h,cpp}:1` `services/{FanController,ClimateController,AirModeController}.*:1`  
**Jetson semantics source:** Demo `Serial` ASCII proposal (11 codes: `1,2,4,5,6,7,8,9,10,101-105,324-332`) — semantic only, not binary IDs.

---

## 1. Executive Summary

Architecture is now **CONFIRMED** to keep ViosAssistant binary baseline: `Serial1/UART1` `GPIO17/18` `115200 8N1`, binary frame `0xAA | ID | PAYLOAD | CRC16 | 0x55` per `ProtocolConfig.h:15` and `ResponseManager.cpp:16`. Jetson demo ASCII (`9600`, `code\r`, 6x `println`) is **proposal only** and will not be copied to production.

The 14 Jetson semantic commands (AC 2, Temp relative 2 + absolute 9, Fan ON/OFF 2, Air 2, Fan Level 5) can be mapped to existing ESP32 `CommandType` (`SET_AC 0x05`, `SET_TEMPERATURE 0x01`, `SET_FAN_SPEED 0x02`, `SET_AIR_MODE 0x03`) with **existing payload APIs**, but exact binary payload encoding for fan levels, temperature delta, and mode enum is **TBD / REQUIRES CONFIRMATION**. Fan ON/OFF save/restore behavior requires a small `FanController` state extension (currently not implemented). Status feedback for 6 Jetson fields (`engine/temperature/AC/wind_value/last_mode/door`) has **no confirmed binary contract** — 2 fields have **SOURCE NOT AVAILABLE** and must not be faked.

RX parsing (`CommunicationTask.cpp:62` discard) and TX periodic wiring (`CommunicationTask.cpp:74` empty) are **P0 BLOCKERs** — no end-to-end flow exists today.

**Result:** `PROTOCOL DESIGN READY` with explicit `TBD` tags — ready for implementation once §21 open questions are confirmed. No code changed in H02.

---

## 2. Confirmed Decisions

| Decision | Value | Source |
|---|---|---|
| Jetson ↔ ESP32 UART | `UART1 / Serial1` | `PinConfig.h:13` `UartDriver.h:12` + user H02 §1 |
| ESP32 TX | GPIO17 → Jetson RX | `PinConfig.h:13` `PIN_PI_UART_TX 17` |
| ESP32 RX | GPIO18 ← Jetson TX | `PinConfig.h:13` `PIN_PI_UART_RX 18` |
| Baud | 115200 | `UartDriver.cpp:10` `Serial1.begin(115200)` `SystemManager.cpp:106` `ProtocolConfig.h:11` `UART_BAUD_RATE` `SystemConfig.h:7` |
| Format | 8N1 `SERIAL_8N1` | `UartDriver.cpp:10` |
| Debug UART | `Serial / UART0` CH343P 43/44 | `UartDriver.cpp:9` comment `PinConfig.h:86` + H02 §1 |
| Frame | Binary `0xAA | ID | PAYLOAD | CRC16 | 0x55` | `ProtocolConfig.h:15` `CMD_HEADER 0xAA`/`FOOTER 0x55` `ResponseManager.cpp:16` |
| Payload max | 64 bytes | `ProtocolConfig.h:17` `CMD_MAX_PAYLOAD` |
| CRC | `CRC16` poly `0x8005` init `0xFFFF` LE, over `header..payload` per `ResponseManager.cpp:13` `crc16(frame, len)` | `ProtocolConfig.h:24` `utils/Crc16.*` |
| Timeout | 1000 ms | `ProtocolConfig.h:18` `CMD_TIMEOUT_MS` |
| Fan semantics | Jetson works in `LEVEL 1-5` only, ESP maps to PWM | H02 §4 + `FanController.h:1` |
| Air mode | Only `FACE` / `FOOT` (no `FACE+FOOT`) | H02 §7 |
| ASCII demo | `101\r` / `9600` / 6x `println` **not** for production | User H02 §2 |

**Not confirmed / must not assume:** Fan Level→PWM table, temp delta vs absolute encoding, air mode enum values, status field encodings.

---

## 3. UART Architecture

```
Jetson Nano ( /dev/ttyTHS1 or USB-UART bridge )
  TX  ──►  ESP32 GPIO18 (RX)  ─┐
  RX  ◄──  ESP32 GPIO17 (TX)  ─┤
  GND ◄─►  ESP32 GND           │
                                │
  ESP32-S3 N16R8 CH343P         │
   - UART1 / Serial1 115200 8N1  │  ← CONFIRMED
   - UART0 / Serial  43/44      │  ← debug only
   - CAN UART 11/12 500k        │  ← separate, STUB
```

- `drivers/UartDriver.h:12` `begin(115200,17,18)` → `UartDriver.cpp:10` `Serial1.begin(baud, SERIAL_8N1, rx, tx)` — correct order `rx,tx`.
- `SystemManager.cpp:106` `uart_.begin(115200,17,18)` initializes at boot; `SystemManager.cpp:111` `can_.begin(500000,11,12)` separate.
- `ViosAssistant.ino:12` `Serial.begin(115200)` for debug — must stay separate from `Serial1`.
- Wiring checklist H01.2.F/G applies; common GND mandatory; no 5V→GPIO.

**Status:** `EXISTING` and correct. No GPIO change.

---

## 4. Binary Frame Format

**Baseline from `ProtocolConfig.h:1` and `ResponseManager.cpp:1` (CONFIRMED):**

```
Byte 0      : 0xAA HEADER (`ProtocolConfig.h:15`)
Byte 1      : FRAME/COMMAND ID
Byte 2..n   : PAYLOAD (0..64, `CMD_MAX_PAYLOAD`)
Byte n+1    : CRC16 low  (LE)
Byte n+2    : CRC16 high (LE)
Byte n+3    : 0x55 FOOTER
CRC coverage: header + ID + payload ( `ResponseManager.cpp:24` `crc16(frame, 5+len)` )
Poly/init   : `0x8005` / `0xFFFF` (`ProtocolConfig.h:24` `Crc16.h:1`)
Timeout     : `1000ms` (`CMD_TIMEOUT_MS`)
```

**Current `ResponseManager` frames (EXISTING):**

| Frame | ID | Payload | Size | Code |
|---|---|---|---|---|
| `sendResponse` | `cmd_type` | `success,error_code,resp_len,resp_data[0..7]` | `8+len` | `ResponseManager.cpp:13` |
| `sendStatus` | `0x80` | `mode,error,uptime32,heap32,cpu16,watchdog,retry` | 19B | `ResponseManager.cpp:31` |
| `sendTemperatureData` | `0x81` | 5 temps `*10` int16 + 4 valid bytes | 19B | `ResponseManager.cpp:57` |
| `sendVehicleData` | `0x82` | `speed*10, rpm/10, coolant*10, batt*100, ac, blower, gear, valid` | 17B | `ResponseManager.cpp:83` |
| `sendAck` | `0xFF` | `cmd_id,success` | 8B | `ResponseManager.cpp:107` |
| `sendError` | `0xFE` | `cmd_id,error_code` | 8B | `ResponseManager.cpp:122` |

**IDs `0x100/0x200/0x300` in `ProtocolConfig.h:20` are defined but not used in `ResponseManager` (uses `0x80..0x82`) — no new ID invented in H02.

**Jetson demo:** `code\r` + `println` — **no header/footer/CRC/ID** — must be replaced by binary.

**Discrepancy to verify in implementation:** `ResponseManager` CRC is over `header..payload` inclusive of `0xAA` at byte 0 — Jetson must implement identical. Byte order is **LE** (`crc & 0xFF` then `>>8`). Keep as-is; do not change algorithm.

---

## 5. Command Protocol Design

Jetson semantics (14 distinct intents, derived from demo `switch`):

```
1        AC ON
2        AC OFF
4        Temp +2 (relative)
5        Temp -2 (relative)
6        Fan ON (recover last level)
7        Fan OFF (save last, ->0)
8        FACE   (last_mode=2)
9        FOOT   (last_mode=1)
101-105  Fan Level 1..5
324-332  Set temp 24..32 (absolute)
```

**Design principle (H02 §3):** Do not copy raw ints `1,101,324` as binary IDs if they clash with `Command.h:7` enum. Map **semantics** to existing `CommandType`:

- `AC ON/OFF` → `SET_AC 0x05` (already `SystemManager.cpp:191`)
- `TEMP +/-2` and `SET TEMP 24-32` → `SET_TEMPERATURE 0x01` (existing `SystemManager.cpp:170` expects `payload[0]+payload[1]*0.1`)
- `FAN ON/OFF` + `LEVEL 1-5` → `SET_FAN_SPEED 0x02` (existing `SystemManager.cpp:176`)
- `FACE/FOOT` → `SET_AIR_MODE 0x03` (existing `SystemManager.cpp:181`)

All 14 map to **EXISTING** `CommandType` values; no new `CommandType` needed unless team confirms new semantics.

Payload design is **TBD** — see §6-9 for exact encoding proposals marked `REQUIRES CONFIRMATION`.

---

## 6. Fan Level Design

**CONFIRMED:** Jetson works only in `LEVEL 1-5`; Jetson never sees PWM `0-255` (`H02 §4`). ESP32 owns `Level -> PWM` mapping via `FanController`/`PwmDriver`.

**Current ESP code:**

- `FanController.h:14` `setSpeed(uint8_t 0-255)` `SystemConfig.h:18` `FAN_SPEED 0-255`
- `FanController.cpp` ramps `current -> target` by `ramp_rate 10`
- `SystemManager.cpp:139` auto overrides `fan_ctrl_.setSpeed(getFanOn()?255:0)` from hysteresis
- No `Level 1-5` concept in production yet.

**Mapping `Level 1-5` → `PWM 0-255` — NOT in code/spec → `REQUIRES CONFIRMATION`.**

**Proposed options (do not implement until confirmed):**

- **A. Linear 1-5 → 51,102,153,204,255** ( `level*51` ) — simple, but `51` steps coarse.
- **B. Calibrated table** e.g., `1:60 2:110 3:160 4:210 5:255` based on FET airflow curve — needs H06 measurement.
- **C. Direct 1-5 as `SET_FAN_SPEED` payload 1-5 and let ESP map internally — Jetson sends `level` byte, ESP expands.

**H02 decision:** Document as **TBD**; do not claim `101→51` as official. `H01_5:95` now correctly states *POSSIBILITY, NOT APPROVED*.

**Related 6/7 behavior:** See §7.

---

## 7. Temperature Design

Two semantics must both be representable (H02 §6):

**A. Relative:** `4 = +2°C`, `5 = -2°C`

- Current `SET_TEMPERATURE` (`Command.h:9` `0x01`) in `SystemManager.cpp:172` does **absolute** `payload[0]+payload[1]*0.1`, not delta.
- **Gap:** No `TEMP_DELTA` command exists. Options:
  - **PROPOSED:** Map `4/5` to `SET_TEMPERATURE` with delta payload (e.g., `int8 delta = +20` for +2.0) and let `ClimateController` add to `target_temp_`, **or**
  - Map to new `CommandType` (e.g., `SET_TEMP_RELATIVE`) — would be new ID, needs team approval → `TBD`.

**B. Absolute:** `324-332 = 24-32°C`

- Current `SystemConfig.h:22` allows `16-30°C` (`ClimateController.cpp:34` `clamp 16-30`), Jetson wants `24-32` → upper 31-32 exceeds ESP clamp. **Requires confirmation** whether to widen ESP to `32` or clamp Jetson to `30`.
- Payload: Jetson `code-300` single byte `24-32` vs ESP `2-byte *10` — **TBD**.

**H02:** Both semantics documented as **TBD payload**, no temperature range change (keep `24-32` Jetson, `16-30` ESP as-is until confirmed).

---

## 8. Air Mode Design

**CONFIRMED per H02 §7:** Only `FACE` and `FOOT` (no `FACE+FOOT`).

**Current ESP enum** `services/ClimateController.h:10` `AirMode { VENT0, BI_LEVEL1, FLOOR2, MIX3, DEFROST4, FLOOR_DEFROST5 }` plus `AirModeController`.

- Jetson `8: last_mode=2` (Face), `9: =1` (Foot) — values `2/1` do **not** match ESP `VENT0` vs `FLOOR2` — **no confirmed mapping**.
- `10: =3` (`FACE+FOOT`) is **explicitly excluded** per H02 — must not be implemented; Jetson should not send `10` in production.

**PROPOSED mapping (TBD):**

- `FACE` → `VENT 0` (or `BI_LEVEL 1` if face+windshield)
- `FOOT` → `FLOOR 2`

Exact enum value **REQUIRES CONFIRMATION** — use enum names, not raw ints. No mode `3` will be implemented.

---

## 9. AC Design

**CONFIRMED:** `AC ON` / `AC OFF` only (`H02 §8`).

- Jetson `1/2` → ESP `SET_AC 0x05` `payload[0] 1/0` `SystemManager.cpp:191` `climate_ctrl_.setAC(payload!=0)` — direct 1:1.
- `RelayDriver` `PIN_AC_RELAY 4` `SystemManager.cpp:75` `active_high true` — note relay trigger level TBD from H01.3.

**Status:** `EXISTING` mapping `1→ON, 2→OFF` is **PROPOSED** (needs Jetson confirmation that `1` means ON), no code gap.

---

## 10. Status Protocol

**Desired** `Jetson ← ESP` per H02 §9: `engine, temperature, AC, wind_value, last_mode, door` — but **DO NOT FAKE**.

| Jetson field | ESP32 source (audited) | Representation (per H02 §4,9) | Binary payload (proposal) | Status |
|---|---|---|---|---|
| `engine` | `model/VehicleData.h:1` `ignition_on/engine_running` or `SystemState.mode` — **no direct `engine` int** | `0/1` or `mode` enum | Not in `ResponseManager` `0x80/0x81/0x82` — **no field** | **SOURCE NOT AVAILABLE / TBD** |
| `temperature` | `TemperatureData.inside_temp_c` NTC1 `SystemManager.cpp:129` `ClimateController` `setpoint` | `int16 *10` `sendTemperatureData 0x81` has 5 temps | `0x81` bytes 2-3 = `inside_temp*10` (EXISTING) | **TBD** — which temp? `inside` vs `setpoint` needs Jetson confirm |
| `AC` | `ClimateController.getAC()` `SystemManager.cpp:144` | `0/1` | Could reuse `sendVehicleData 0x82` `ac_compressor_active` or `sendStatus` — no dedicated AC status byte | **TBD** |
| `wind_value` | `FanController.getSpeed()` 0-255 + `ClimateController` hysteresis | **Level 1-5** per H02 §4 (not PWM) | Not in `ResponseManager` — `sendVehicleData` has `blower_active` bool only | **TBD** — needs Level mapping inverse |
| `last_mode` | `ClimateController.getAirMode()` / `AirModeController` | `FACE/FOOT` enum | Not in `ResponseManager` — `sendStatus` has `SystemMode` not `AirMode` | **TBD** |
| `door` | `VehicleData.h:1` has no `door` field (only speed/rpm/coolant/battery/ac/blower/gear) | `0/1` | Not defined | **SOURCE NOT AVAILABLE** — must not fake |

**Additional TBD:** Delimiter (`\n` vs binary), frequency (Jetson 100ms vs ESP 50/500/1000ms timers `CommunicationTask:74`), ACK/Error handling.

**No fake data will be sent** for `engine`/`door`.

---

## 11. ACK / Error Protocol

**Current `ResponseManager` (EXISTING, binary):**

- Success: `sendResponse` `0xAA | cmd_type | 1 | error_code 0 | resp_len | data | CRC | 0x55` (`ResponseManager.cpp:13`)
- `CommandManager.cpp:38` `success true/false` `error_code 1=INVALID 2=NOT_IMPL`
- `sendAck 0xFF` / `sendError 0xFE` 8B (`ResponseManager.cpp:107`)
- `ProtocolConfig.h:15` timeout `1000ms`

**Jetson demo:** No ACK/error handling — `Serial.println(code)` fire-and-forget, no `available()` parse for response.

**Questions (TBD):**
- Should Jetson wait for `sendResponse`/`sendAck` before next command?
- On `CRC fail` → ESP discards frame (no `sendError` yet) — should it `sendError`?
- On `INVALID`/`NOT_IMPL` → ESP already sets `error_code 1/2` in `CommandManager.cpp:38` and `SystemManager.cpp:161` early-return — should it also `sendError 0xFE`?
- Jetson timeout handling (1000ms per `ProtocolConfig`) — Jetson needs retry logic.

**Proposal (not implemented):** Every Jetson command gets `sendResponse` with `success/error_code`; on CRC fail no response (or `sendError` after team confirms). Mark **PROPOSED**.

---

## 12. RX Data Flow

```
Jetson Nano
  ↓ 115200 8N1
UART1 TX17/RX18 (CONFIRMED PinConfig.h:13)
  ↓
drivers/UartDriver.cpp:10 Serial1 (EXISTING)
  ↓
rtos/CommunicationTask.cpp:62 processUartMessages()
  - reads buffer[64] via UartDriver:28
  - // Process UART frame (DISCARD) — NO parser → P0 BLOCKER
  ↓
[Parser] — MISSING (needs binary 0xAA parser per ProtocolConfig.h:15 + CRC)
  ↓
application/CommandManager.h:9 queue 16 (EXISTING)
  - queueCommand validates, processCommand 7 vs 4 assumption (EXISTING)
  ↓
application/SystemManager.cpp:157 handleCommand (EXISTING, 7 cmds)
  ↓
services/ClimateController / FanController / AirModeController (EXISTING, but Fan save/restore missing)
  ↓
drivers/RelayDriver / PwmDriver / MotorDriver (EXISTING)
  ↓
Hardware
```

**Existing nodes:** `UartDriver`, `CommandManager`, `SystemManager`, Services/Drivers.  
**Missing:** **Parser** — `P0 BLOCKER`.

---

## 13. TX Data Flow

```
Hardware (NTC 1/2, VehicleData, SystemState)
  ↓
drivers/NtcDriver, VehicleDataService (EXISTING, but VehicleDataService parsers STUB)
  ↓
SystemManager.cpp:121 update() — polls NTC, VehicleData, Climate (EXISTING)
  ↓
application/ResponseManager.cpp:31/57/83/107 (EXISTING binary frames)
  ↓ (NOT WIRED)
rtos/CommunicationTask.cpp:74 sendPeriodicUpdates()
  - has timers last_status 1000, last_temps 500, last_vehicle 1000
  - **NO calls to ResponseManager** — P0 BLOCKER
  ↓
drivers/UartDriver.cpp:20 write() Serial1 (EXISTING)
  ↓
UART1 17/18 115200
  ↓
Jetson Nano
```

**Existing at driver level:** `ResponseManager` frames `0x80/0x81/0x82` with CRC, `UartDriver` write.  
**Missing wiring:** Periodic send never called; no 6-field frame for Jetson; `engine`/`door` no source.

---

## 14. Command Mapping Table

| Jetson semantic | Current ESP command | Payload (current ESP) | Target | Status |
|---|---|---|---|---|
| AC ON (1) | `SET_AC 0x05` | `payload[0]=1` | `ClimateController.setAC` `SystemManager:191` | **TBD** — intent matches, confirm 1→1 |
| AC OFF (2) | `SET_AC 0x05` | `payload[0]=0` | same | **TBD** |
| TEMP +2 (4) | `SET_TEMPERATURE 0x01` | `payload[0]+payload[1]*0.1` absolute | `ClimateController.setTemperature` | **TBD** — delta vs absolute |
| TEMP -2 (5) | `SET_TEMPERATURE` | same | same | **TBD** |
| FAN ON (6) | `SET_FAN_SPEED 0x02` | `uint8 0-255` | `FanController.setSpeed` / `enable` | **TBD** — save/restore not in `FanController.h:14` |
| FAN OFF (7) | `SET_FAN_SPEED` | same | same | **TBD** |
| FACE (8) | `SET_AIR_MODE 0x03` | `AirMode` enum | `ClimateController.setAirMode` | **TBD** — enum value confirm |
| FOOT (9) | `SET_AIR_MODE` | enum | same | **TBD** |
| FAN LEVEL 1 (101) | `SET_FAN_SPEED` | `1` (PROPOSED) | `FanController` Level→PWM | **TBD** — mapping Level→PWM `REQUIRES CONFIRMATION`, x51 NOT APPROVED |
| FAN LEVEL 2 (102) | `SET_FAN_SPEED` | `2` | same | **TBD** |
| FAN LEVEL 3 (103) | `SET_FAN_SPEED` | `3` | same | **TBD** |
| FAN LEVEL 4 (104) | `SET_FAN_SPEED` | `4` | same | **TBD** |
| FAN LEVEL 5 (105) | `SET_FAN_SPEED` | `5` | same | **TBD** |
| SET TEMP 24 (324) | `SET_TEMPERATURE` | `24` (PROPOSED single byte) vs 2-byte `*10` | `ClimateController` | **TBD** — payload format |
| SET TEMP 25 (325) | `SET_TEMPERATURE` | `25` | same | **TBD** |
| SET TEMP 26 (326) | `SET_TEMPERATURE` | `26` | same | **TBD** |
| SET TEMP 27 (327) | `SET_TEMPERATURE` | `27` | same | **TBD** |
| SET TEMP 28 (328) | `SET_TEMPERATURE` | `28` | same | **TBD** |
| SET TEMP 29 (329) | `SET_TEMPERATURE` | `29` | same | **TBD** |
| SET TEMP 30 (330) | `SET_TEMPERATURE` | `30` | same | **TBD** |
| SET TEMP 31 (331) | `SET_TEMPERATURE` | `31` | same | **TBD** — exceeds ESP `30` max `SystemConfig.h:22` |
| SET TEMP 32 (332) | `SET_TEMPERATURE` | `32` | same | **TBD** — exceeds `30` |

No `?` — all rows have explicit status. No new `CommandType` invented.

---

## 15. Status Mapping Table

| Jetson field | ESP32 source | Representation (H02) | Binary payload (proposal) | Status |
|---|---|---|---|---|
| `engine` | `SystemState.mode` `VehicleData.ignition_on` — no `engine` int | `0/1` | Not in `ResponseManager` | **SOURCE NOT AVAILABLE / TBD** |
| `temperature` | `TemperatureData.inside_temp_c` NTC1 `SystemManager:129` or `ClimateController target` | `int16 *10` (existing `0x81` bytes 2-3) | `0x81` inside_temp | **TBD** — which temp? |
| `AC` | `ClimateController.getAC()` | `0/1` | `0x82` `ac_compressor_active` or `0x80` status — no dedicated | **TBD** |
| `wind_value` | `FanController.getSpeed()` + `FanController` Level | **Level 1-5** (per §4) | Not in `ResponseManager` — `0x82` has `blower_active` bool only | **TBD** — Level vs PWM, inverse map needed |
| `last_mode` | `ClimateController.getAirMode()` | `FACE/FOOT` (2 values) | Not in `ResponseManager` | **TBD** |
| `door` | `VehicleData` has no `door` | `0/1` | Not defined | **SOURCE NOT AVAILABLE / TBD** |

No fake field created. Additional TBD: delimiter, frequency, ACK.

---

## 16. Current Code Gaps

| File | Gap | Status |
|---|---|---|
| `rtos/CommunicationTask.cpp:62` `processUartMessages` | Reads `buffer[64]` but `// Process UART frame` discards — no `0xAA` parser, no `Command` creation | **BLOCKED P0** |
| `rtos/CommunicationTask.cpp:74` `sendPeriodicUpdates` | Timers `1000/500/1000` but **no** `ResponseManager` calls | **BLOCKED P0** |
| `services/FanController.h:14` | No `last_wind_level` storage for `6` (ON) / `7` (OFF) save/restore — demo has `last_wind_level` var | **BLOCKED P1** — needs `uint8 last_level_` + `enable` logic |
| `services/ClimateController.h:10` `AirMode` | Has `VENT,BI_LEVEL,FLOOR,MIX,DEFROST,FLOOR_DEFROST` — Jetson needs only `FACE/FOOT` mapping, `10` must be rejected | **TBD** — enum mapping not confirmed |
| `model/Command.h:7` `CommandType` | No `TEMP_DELTA` — `4/5` would need delta handling or reuse `SET_TEMPERATURE` | **TBD** |
| `application/ResponseManager.*` | No 6-field `engine/door` frame; `engine`/`door` sources missing | **SOURCE NOT AVAILABLE** |
| `services/VehicleDataService.*` | `engine`/`door` not in `VehicleData.h:1` | **TBD** |
| `drivers/UartDriver.*` | Works at `115200` but Jetson demo uses `9600` — Jetson must update | **MISMATCH** |

---

## 17. P0 / P1 / P2 Blockers

### P0 — Must fix before end-to-end Jetson run

- **P0-1 RX parser** `CommunicationTask:62` — implement binary `0xAA` parser per `ProtocolConfig.h:15` with CRC `0x8005`/`0xFFFF` LE, validate `FOOTER 0x55`, handle `max 64`/`timeout 1000`.
- **P0-2 TX wiring** `CommunicationTask:74` — wire timers to `ResponseManager::sendStatus`/`sendTemperatureData`/`sendVehicleData` or new 6-field frame.
- **P0-3 Command mapping** — 14 semantics (§14) all **TBD** — need team sign-off on exact `CommandType` + payload bytes for 1,2,4,5,6,7,8,9,101-105,324-332.
- **P0-4 Status contract** — 6 fields §15 all **TBD** — define binary layout for Jetson or confirm Jetson will parse `0x80/0x81/0x82`.

### P1 — Before H05 hardware test

- **P1-1** Fan Level→PWM table (1-5 → PWM) — **REQUIRES CONFIRMATION**, do not use `x51`.
- **P1-2** Temp delta vs absolute — decide if `4/5` stays delta or becomes absolute `SET_TEMPERATURE`.
- **P1-3** Air mode enum — confirm `8→FACE`/`9→FOOT` values vs `AirMode` enum.
- **P1-4** Jetson wiring/bau d — confirm Jetson `Serial1` 17/18 115200.

### P2 — Nice to have

- ESP-only commands (`RECIRCULATION`, `HEATER` etc.) — Jetson may ignore.
- `engine`/`door` source definition or removal from Jetson expectation.
- Frequency 100ms vs 50/500/1000ms.
- `sendAck 0xFF`/`sendError 0xFE` handling on Jetson.

---

## 18. Exact Files Requiring Modification (Implementation Phase)

**No file modified in H02** — list for next phase:

- `rtos/CommunicationTask.cpp:62` + `rtos/CommunicationTask.h:23` — add parser + TX wiring
- `rtos/CommunicationTask.cpp:74` — call `ResponseManager`
- `application/CommandManager.*` — only if new `CommandType` or payload mapping confirmed (currently 7 vs 4 assumption stays)
- `application/ResponseManager.*` — only if new status frame for 6 fields needed (else reuse `0x80/0x81/0x82`)
- `services/FanController.{h,cpp}:14` — add `last_level_` for `6/7` save/restore (P1)
- `services/ClimateController.*` — only if `TEMP_DELTA` or AirMode enum mapping confirmed
- `config/ProtocolConfig.h:1` — **no change** (baseline kept)
- `PinConfig.h:13` — **no change** (17/18 confirmed)
- `drivers/UartDriver.*` — **no change** (115200 already)
- `model/Command.h:7` — only if new `CommandType` approved

**Not to modify:** `PinConfig` GPIOs, `SystemConfig` hysteresis/`PULSES_PER_REV 11300`, `MotorDriver` nSLEEP, `PwmDriver` freq, `ProtocolConfig` CRC.

---

## 19. Implementation Order

1. **Contract sign-off** (§21) — team confirms §14/15 exact bytes (fan level, temp, mode, door/engine).
2. **RX parser** `CommunicationTask:62` — binary `0xAA` per `ProtocolConfig` + CRC LE, create `Command`.
3. **Fan save/restore** `FanController` — add `last_level_` for `6/7` after contract.
4. **TX wiring** `CommunicationTask:74` — wire `ResponseManager` sends at period agreed (e.g., 500ms temp, 1000ms status).
5. **Jetson update** — switch demo to `Serial1` 115200 binary per contract, remove `9600`/`6 println`.
6. **Verification** — `T05` loopback 17/18, `T15` with new codes, `T21` full link, then `H05` UART hardware.

---

## 20. Verification / Test Plan

| Test | Purpose | Method |
|---|---|---|
| `T05` Pi UART (existing) | Loopback `17/18 115200` | `test05_pi_uart` style — must still PASS |
| `T15` CommandManager | New 14 codes with confirmed payloads | Extend `test15_command_manager` with new `CommandType` payloads, check `error_code 0` |
| `T21` Production | Full link 22 .cpp + new parser | `test21_production_build` merged sketch — must PASS 366k |
| `T22` Pin audit | `17/18` still single source | `test22_pin_config_audit` — `PIN_PI_UART 17/18` |
| `H05` Hardware UART | Jetson ↔ ESP32 17/18 | DMM/scope, send `0xAA` frame from Jetson, expect `0xAA` response / `ACK` |

No `0xAA` test will be added in H02 — only plan. `H01` safety (12V OFF) still applies before `H05`.

---

## 21. Open Questions — Only Unconfirmed from Code/Spec

1. **Fan 1-5 → PWM?** Jetson sends level `1-5` (`101-105`), ESP `0-255` — what exact PWM per level? `x51` is **not approved**.
2. **Temp `4/5` delta vs absolute?** Should `+2/-2` remain delta or be removed in favor of `324-332` absolute?
3. **Temp `324-332` payload?** Single byte `24-32` vs 2-byte `*10` vs existing `24-32` exceeds ESP `30` max — widen ESP or clamp?
4. **Air mode `8/9` mapping?** `8=2`/`9=1` vs ESP `VENT0/FLOOR2` — which enum value?
5. **What is `10` (both)?** H02 says no `FACE+FOOT` — should `10` be rejected (`INVALID`) or mapped to `BI_LEVEL`?
6. **Fan `6/7` semantics?** Confirm save/restore `last_level` behavior exact (ESP `FanController` currently has no `last_level_`).
7. **`engine` source?** `VehicleData` has no `engine`; is it `SystemState.mode` or `VehicleData.ignition` or new?
8. **`door` source?** No source — remove or define new sensor?
9. **`wind_value` representation?** Level `1-5` or PWM `0-255` in status — H02 says Level?
10. **`last_mode` representation?** `FACE/FOOT` enum value and frequency?
11. **Status framing?** `6×println \n 100ms` vs binary `0x80/0x81/0x82` + CRC — which for production?
12. **ACK needed?** Does Jetson expect `sendResponse`/`0xFF`/`0xFE` or fire-and-forget?

All above are **TBD / REQUIRES CONFIRMATION** — no guess will be implemented.

---

## 22. Final Result

**PROTOCOL DESIGN READY** — with explicit `TBD`/`REQUIRES CONFIRMATION` tags

**But implementation is `BLOCKED` until:**

- P0-1 to P0-4 resolved (parser, TX wiring, command mapping table, status contract) and
- §21 questions 1-12 confirmed by Jetson/team.

**Next step:** Team reviews §14/15 tables and §21 questions, signs binary contract, then implementation phase edits §18 files in order §19.

*No production code/config built, flashed, or committed in H02. PinConfig 17/18 and `PULSES_PER_REV 11300` etc. preserved.*

