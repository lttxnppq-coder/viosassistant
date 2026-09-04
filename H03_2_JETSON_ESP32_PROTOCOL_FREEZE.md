# H03.2 Jetson ↔ ESP32 Protocol Freeze — Decision Document

**Date:** 2026-09-02  
**Mode:** READ-ONLY — NO production code/config change, NO build, NO flash, NO GPIO change  
**Sources:** `H02_JETSON_ESP32_PROTOCOL_DESIGN.md:1` `H03_1_IMPLEMENTATION_REVIEW.md:1` `H03_JETSON_ESP32_PROTOCOL_IMPLEMENTATION.md:1` `config/ProtocolConfig.h:1` `model/Command.h:1` `application/{ResponseManager,SystemManager}.*:1` `services/{FanController,ClimateController}.*:1` `rtos/CommunicationTask.*:1`  
**Purpose:** Freeze UART/frame decisions that are CONFIRMED and isolate TBD items requiring Jetson team confirmation before H03 implementation is considered complete.

---

## 1. UART

**CONFIRMED — No change from H02 §2 / H01_5:**

| Item | Decision | Source |
|---|---|---|
| ESP32 TX | GPIO17 → Jetson RX | `PinConfig.h:13` `PIN_PI_UART_TX 17` |
| ESP32 RX | GPIO18 ← Jetson TX | `PinConfig.h:13` `PIN_PI_UART_RX 18` |
| GND | Common | H01.2 |
| Baud | 115200 | `drivers/UartDriver.cpp:10` `Serial1.begin(115200)` `SystemManager.cpp:106` `ProtocolConfig.h:11` |
| Format | 8N1 `SERIAL_8N1` | `UartDriver.cpp:10` |
| ESP32 UART | `Serial1 / UART1` | `UartDriver.h:12` |
| Debug | `Serial / UART0` CH343P 43/44 | `UartDriver.cpp:9` `PinConfig.h:86` + H02 §1 |

**Status:** `CONFIRMED` — Jetson must use `Serial1` (`/dev/ttyTHS1` or equivalent) at 115200, `Serial` remains debug. No `9600` ASCII.

---

## 2. Frame Format

**Current H03 implementation (as built):**

```
0xAA | ID | LEN | PAYLOAD[LEN] | CRC16_L | CRC16_H | 0x55
 0      1    2     3..3+LEN-1      3+LEN     4+LEN      5+LEN
LEN: 0-64, validated >64 → reset
Payload max: 64
```

**H02 / ProtocolConfig baseline:**

- `ProtocolConfig.h:15` `CMD_HEADER 0xAA` / `CMD_FOOTER 0x55` / `ProtocolConfig.h:17` `CMD_MAX_PAYLOAD 64` / `ProtocolConfig.h:18` `CMD_TIMEOUT_MS 1000` — **CONFIRMED**
- `ProtocolConfig.h:1` **does not define a `LEN` byte** — H02 §4 describes frame as `0xAA | ID | PAYLOAD | CRC | 0x55` without `LEN`
- `ResponseManager.cpp:13` **actually has** `LEN` at byte 4 (`resp_len`) for response frames: `0xAA | cmd_type | success | error_code | resp_len | resp_data | CRC | 0x55` (5+len header)
- `ResponseManager.cpp:31` `0x80`, `57` `0x81`, `83` `0x82` also have implicit LEN via payload structure

**CRC:**

- Polynomial `0x8005`, Init `0xFFFF`, `utils/Crc16.h:9`, `Crc16.cpp:1` — **CONFIRMED**, no change (`H02 §4`)
- **Byte order LE** — `ResponseManager.cpp:25` `crc &0xFF` then `>>8`, `CommunicationTask.cpp:318` `frame[3+LEN]=CRC_L`, `4+LEN=CRC_H` — **CONFIRMED LE**
- **Coverage discrepancy (needs team confirmation):**
  - `ResponseManager` response: CRC over `header+ID+success+error+len+payload` (5+len bytes, `crc16(frame,5+len)`)
  - `ResponseManager` status `0x80`: `crc16(frame,16)` over `header+ID+14 payload` (16 bytes)
  - `CommunicationTask` command (H03): `crc16(frame,3+LEN)` over `header+ID+LEN+payload` (3+LEN bytes)
  - `CommunicationTask` Jetson status `0x90`: `crc16(frame,9)` over `header+ID+LEN+6 payload` (9 bytes)
  - **All are LE and poly/init same, but coverage differs by frame type** — this is **EXISTING** behavior for response/status vs **H03 design choice** for command. Must be documented as **LEN = TBD / REQUIRES TEAM CONFIRMATION** and **CRC coverage = TBD** for Jetson command.

**Status:**

| Field | Status |
|---|---|
| Header `0xAA` | **CONFIRMED** `ProtocolConfig.h:15` |
| Footer `0x55` | **CONFIRMED** |
| CRC poly/init/LE | **CONFIRMED** |
| Payload max 64 | **CONFIRMED** |
| Timeout 1000ms | **CONFIRMED** |
| **LEN byte at position 2** | **TBD / REQUIRES TEAM CONFIRMATION** — H03 adds `LEN`; `ProtocolConfig`/`H02` text does not define it, but `ResponseManager` implies need for length. Team must confirm Jetson will send `LEN`. |
| **CRC coverage for command** | **TBD** — H03 uses `header+ID+LEN+PAYLOAD`; Jetson must match. |
| **New ID `0x90`** | **PROPOSED** — not in `ProtocolConfig` |

**No new ID/payload invented as CONFIRMED.**

---

## 3. Command Semantics

| Jetson demo | Semantic | ESP32 Command (existing) | Payload (current ESP) | Status |
|---|---|---|---|---|
| 1 | AC ON | `SET_AC 0x05` (`Command.h:14`) | `[1]` (`SystemManager.cpp:191` `setAC(1)`) | **Semantics CONFIRMED**, encoding **TBD** (Jetson 1 as binary `1` needs confirm) |
| 2 | AC OFF | `SET_AC` | `[0]` | **TBD** |
| 4 | Temp +2 (relative) | `SET_TEMPERATURE 0x01` (`Command.h:9`) expects `payload[0]+payload[1]*0.1` absolute | **No delta API** — `ClimateController.cpp:33` `setTemperature` absolute | **TBD** — delta vs absolute, payload `0x04` marker in H03 is **PROPOSED** |
| 5 | Temp -2 | `SET_TEMPERATURE` | same | **TBD** |
| 6 | Fan ON restore | `SET_FAN_SPEED 0x02` (`Command.h:10`) | `len 0` → `fanOn()` (`FanController.cpp:88`) | **Semantics CONFIRMED**, encoding **TBD** |
| 7 | Fan OFF save | `SET_FAN_SPEED` | `[0]` → `fanOff()` | **TBD** |
| 8 | FACE | `SET_AIR_MODE 0x03` (`Command.h:11`) `AirMode` enum `VENT0` | `[0]` (H03 maps `8→0`) | **TBD** — `8→VENT` is **PROPOSED** |
| 9 | FOOT | `SET_AIR_MODE` | `[2] FLOOR` (`9→2`) | **TBD** |
| 10 | FACE+FOOT | — | — | **MUST REJECT** `INVALID` — H02 §7 CONFIRMED no such mode |
| 101-105 | Fan Level 1-5 | `SET_FAN_SPEED` | `[1]-[5]` level (H03) → PWM via `FanController::setLevel` | **Semantics CONFIRMED** (Level 1-5), **encoding TBD** (level vs PWM), mapping `51*n` **PROPOSED** |
| 324-332 | Set temp 24-32 | `SET_TEMPERATURE` | `[24]-[32]` single byte (H03) | **Semantics CONFIRMED** 24-32, **encoding TBD** (1-byte vs 2-byte `*10`), **range BLOCKED** for 31-32 |

**All rows:** Semantic **CONFIRMED** per H02 §3; binary payload **TBD** except AC.

---

## 4. Fan Level

**CONFIRMED per H02 §4 & H03 §4:**

- Jetson only sends **Level 1..5**, never raw PWM `0-255`
- ESP32 owns `Level → PWM` via `FanController`/`PwmDriver` (`FanFET 7` `1000Hz 8-bit`)

**Current code:**

- `FanController.h:23` `setLevel(1-5)` `getLevel()` `FanController.cpp:54` `level*51` (51,102,153,204,255) — **PROPOSED**, not from spec
- Existing `setSpeed(0-255)` still exists for `SystemManager.cpp:139` hysteresis (`255/0`)

**Status:**

| Level | Current H03 PWM | Status |
|---|---|---|
| 0 | 0 | **CONFIRMED** (OFF) |
| 1 | 51 | **PROPOSED — REQUIRES CONFIRMATION / H06 CALIBRATION** — H02 §6 lists `51*n` as *possibility, not approved* |
| 2 | 102 | **PROPOSED** |
| 3 | 153 | **PROPOSED** |
| 4 | 204 | **PROPOSED** |
| 5 | 255 | **PROPOSED** |

**No `51*n` as CONFIRMED.** Team may choose calibrated table `60,110,160,210,255` or other.

---

## 5. Temperature

**Semantics CONFIRMED per H02 §6:**

- Relative `+2` (4) / `-2` (5)
- Absolute `24..32` (324..332)

**Current code:**

- `ClimateController.h:31` `Config 16.0-30.0` `ClimateController.cpp:34` `clamp 16-30`
- `SystemManager.cpp:169` now handles `len 1` single byte `24-32` and `len>=2` `payload[0]+payload[1]*0.1`
- `CommunicationTask.cpp:121` for `4/5` converts delta to absolute by reading `getTemperatureData().setpoint` `+2/-2` then sends absolute (PROPOSED)

**Status:**

| Range | Current Capability | Status |
|---|---|---|
| 24-30 | Can execute via `setTemperature` single byte or 2-byte | **EXISTING** |
| 31-32 | Will be **clamped to 30** by `ClimateController` | **BLOCKED / REQUIRES DECISION** — H02 says keep `16-30` until team confirms widen to `32`; Jetson must limit to `24-30` or ESP widen to `32` |

**No code change in H03.2** — document as `24-30` executable, `31-32` `BLOCKED`.

---

## 6. Air Mode

**CONFIRMED per H02 §7:** Only `FACE` and `FOOT`, no `FACE+FOOT`.

**Current enum** `ClimateController.h:11` `AirMode { VENT0, BI_LEVEL1, FLOOR2, MIX3, DEFROST4, FLOOR_DEFROST5 }`

**H03 mapping (PROPOSED):**

- `FACE (8)` → `VENT 0` (`CommunicationTask.cpp:185` `8→0`)
- `FOOT (9)` → `FLOOR 2` (`9→2`)
- `10` → reject `INVALID`

**Status:** `PROPOSED — REQUIRES CONFIRMATION` — H02 §8 says mapping needs confirmation, `VENT` vs `BI_LEVEL` for `FACE` not decided.

**No new enum value invented as CONFIRMED.**

---

## 7. Fan ON/OFF

**Semantic CONFIRMED per demo + H02 §5:**

```
Level X → FAN OFF (7) → save X → PWM 0 → FAN ON (6) → restore X
```

**Current code:**

- `FanController.h:38` `last_level_ 128`, `FanController.cpp:79` `fanOff()` saves `target/current` then `setSpeed(0)`, `fanOn()` restores `setSpeed(last_level_)`
- `SystemManager.cpp:176` `len0→fanOn`, `0→fanOff`
- `CommunicationTask.cpp:152` `len0` for `6`, `len1 0` for `7`

**Status:** `CONFIRMED` semantics, `EXISTING` implementation after H03.

---

## 8. AC

**CONFIRMED:** `AC ON` / `AC OFF` only (`H02 §8`).

- Jetson `1/2` → `SET_AC 0x05` `[1]/[0]` → `ClimateController.setAC` `SystemManager.cpp:191` → `RelayDriver` `PIN_AC_RELAY 4` `SystemManager.cpp:75` `active_high true` (TBD from H01.3)

**Current source in `CommunicationTask.cpp:354` `sendPeriodicUpdates` for `0x90` AC placeholder is **BLOCKED** — uses `SystemState.error==NONE?1:0` instead of `ClimateController.getAC()` — marked below.

**Status:** Semantics `CONFIRMED`, AC source for status **BLOCKED** (see §9).

---

## 9. Status ESP32 → Jetson

**Jetson wants:** `engine, temperature, AC, wind_value, last_mode, door` every ~100ms via 6 `println` (demo). H02 says **DO NOT FAKE**.

**Existing `ResponseManager` frames (EXISTING):**

- `0x80` `sendStatus` 19B `mode,error,uptime,heap,cpu,watchdog` (`ResponseManager.cpp:31`)
- `0x81` `sendTemperatureData` 19B 5 temps `*10` + valid (`ResponseManager.cpp:57`)
- `0x82` `sendVehicleData` 17B `speed*10, rpm/10, coolant*10, batt*100, ac, blower, gear, valid` (`ResponseManager.cpp:83`)

**H03 status `0x90` (PROPOSED, 6-field):**

`CommunicationTask.cpp:372` `0xAA 0x90 0x06 [engine, temp, AC, wind, last_mode, door] CRC 0x55` 12B, sent every 1000ms in `sendPeriodicUpdates:344`.

| Field | Source audited | Current H03 `0x90` value | Status |
|---|---|---|---|
| `engine` | `VehicleData.h:1` has `ignition_on/engine_running`, `SystemState.mode` — **no `engine` int** | `jet_payload[0] = mode!=OFF?1:0` (`CommunicationTask.cpp:356`) | **SOURCE NOT AVAILABLE / PROPOSED** — placeholder, must not be treated as real |
| `temperature` | `TemperatureData.inside_temp_c` `SystemManager.cpp:129` NTC1, `setpoint_temp_c` | `jet_payload[1] = (uint8)(inside_valid?inside:setpoint)` (`CommunicationTask.cpp:358`) | **TBD** — which temp? `inside` vs `setpoint` needs confirm, but source **EXISTING** |
| `AC` | `ClimateController.getAC()` not exposed via `SystemManager` | `jet_payload[2] = error==NONE?1:0` (**wrong source**) | **BLOCKED** — should be `ClimateController.getAC()` via new `SystemManager` getter |
| `wind_value` | `FanController.getSpeed()` 0-255 + `getLevel()` 1-5 | `jet_payload[3]=0` hardcode (**placeholder**) | **SOURCE NOT AVAILABLE in 0x90** — `TBD`, should be `getLevel()` 1-5, currently 0 |
| `last_mode` | `ClimateController.getAirMode()` | `jet_payload[4]=0` hardcode | **TBD** |
| `door` | `VehicleData` has no `door` | `jet_payload[5]=0` | **SOURCE NOT AVAILABLE** — must not fake |

**Evaluation of existing frames:**

- `0x80/0x81/0x82` have **real sources** and are **EXISTING**, but do not contain 6-field layout Jetson expects.
- `0x90` is **PROPOSED / BLOCKED — REQUIRES TEAM CONFIRMATION** — must not be considered *CONFIRMED*.

**Rule:** Do not send `0` placeholder as if it were `engine OFF` or `door closed` — Jetson would misinterpret.

---

## 10. Status Contract

| Field | Type | Range | Source | Validity | Frequency | Frame ID | Payload pos | Status |
|---|---|---|---|---|---|---|---|---|
| `engine` | `uint8` | `0/1` or `mode` | **No direct** — `SystemState`/`VehicleData` | `SOURCE NOT AVAILABLE` | 1000ms (H03) vs 100ms demo | `0x90` byte 0 (PROPOSED) | 0 | **BLOCKED** |
| `temperature` | `uint8` or `int16*10` | `24-32` or `0-255` | `TemperatureData.inside_temp_c` / `setpoint` | `inside_valid` | 500ms `0x81` vs 1000ms `0x90` | `0x81` bytes2-3 / `0x90` byte1 | 1 | **TBD** |
| `AC` | `uint8` | `0/1` | `ClimateController.getAC()` (not exposed) | always | 1000ms | `0x90` byte2 | 2 | **BLOCKED** (wrong source) |
| `wind_value` | `uint8` | `1-5` `Level` (per §4) | `FanController.getLevel()` | `fan_on` hysteresis | 1000ms | `0x90` byte3 | 3 | **TBD** |
| `last_mode` | `uint8` | `FACE 0 / FOOT 2` | `ClimateController.getAirMode()` | always | 1000ms | `0x90` byte4 | 4 | **TBD** |
| `door` | `uint8` | `0/1` | **No source** | — | 1000ms | `0x90` byte5 | 5 | **SOURCE NOT AVAILABLE** |

Frequency: H03 `1000ms` status `500ms` temp vs demo `100ms` — **REQUIRES TEAM CONFIRMATION**.

---

## 11. Status Frequency

- H03 `CommunicationTask.cpp:344` `1000ms` status (`0x80`+`0x90`), `500ms` temp (`0x81`), `1000ms` vehicle (`0x82`)
- Demo: `delay(100)` → 6 `println` every 100ms
- **Status:** `REQUIRES TEAM CONFIRMATION` — Jetson must confirm if 1000ms binary is acceptable or needs 100ms.

---

## 12. ACK / Error

**Current `ResponseManager` / `CommunicationTask` (EXISTING, no change):**

- Success: `sendResponse` `0xAA | cmd_type | 1 | 0 | resp_len | data | CRC | 0x55` (`ResponseManager.cpp:13`)
- `CommandManager.cpp:38` `error_code 1=INVALID 2=NOT_IMPL` + `SystemManager.cpp:161` early-return
- `sendError 0xFE` 8B `cmd_id,error_code` (`ResponseManager.cpp:122`), `sendAck 0xFF` (`ResponseManager.cpp:107`)
- `CommunicationTask.cpp:247` `sendError 1` on unknown ID, `324` `sendError 3` on CRC fail
- CRC fail → **no ACK**, frame discarded (per `ResponseManager` behavior)
- Unknown command → `sendError` with `error_code` (H03)
- Timeout: `ProtocolConfig.h:18` `1000ms` — Jetson should retry if no `sendResponse` within 1000ms (PROPOSED)

**No new error code invented.**

---

## 13. Final Decision Table

| ITEM | STATUS | DECISION REQUIRED |
|---|---|---|
| UART 17/18 115200 8N1 Serial1 | **CONFIRMED** | None |
| Header/Footer `0xAA/0x55` | **CONFIRMED** | None |
| CRC poly/init/LE | **CONFIRMED** | None |
| Payload max 64 | **CONFIRMED** | None |
| **LEN byte at pos 2** | **TBD / REQUIRES TEAM CONFIRMATION** | Team must confirm Jetson will send `LEN` |
| CRC coverage `header+ID+LEN+payload` | **TBD** | Confirm Jetson CRC includes `LEN` |
| New ID `0x90` | **PROPOSED / BLOCKED** | Remove or confirm 6-field binary vs `0x80/0x81/0x82` |
| AC 1/2 → `SET_AC` | **TBD** | Confirm `1=ON` |
| TEMP +2/-2 (4/5) | **TBD** | Confirm delta payload `0x04/0x05` vs absolute, `H03` uses `0x04/0x05` marker → absolute |
| FAN ON/OFF 6/7 save/restore | **CONFIRMED** semantics, **EXISTING** code | None |
| FACE/FOOT 8/9 → `VENT0/FLOOR2` | **PROPOSED / REQUIRES CONFIRMATION** | Confirm enum |
| Reject 10 | **CONFIRMED** | None |
| FAN LEVEL 1-5 (101-105) `1-5` | **Semantics CONFIRMED**, **mapping PROPOSED** | Confirm Level 1-5 as payload `1-5` |
| FAN LEVEL→PWM table `51*n` | **PROPOSED — REQUIRES CONFIRMATION / H06** | Calibrate vs linear |
| SET TEMP 24-32 single byte | **TBD** | Confirm `24-32` single byte vs 2-byte `*10` |
| Temp 31-32 vs `30` clamp | **BLOCKED** | Widen `ClimateController` to `32` or limit Jetson to `30` |
| `engine` source | **SOURCE NOT AVAILABLE** | Define or remove |
| `door` source | **SOURCE NOT AVAILABLE** | Define or remove |
| `wind_value` Level vs PWM | **TBD** | Confirm Level 1-5 |
| `last_mode` enum | **TBD** | Confirm `FACE 0 / FOOT 2` |
| `temperature` which field | **TBD** | `inside` vs `setpoint` |
| `AC` source for status | **BLOCKED** | Fix `CommunicationTask.cpp:362` to use `ClimateController` |
| Status frequency 1000/500 vs 100 | **TBD** | Confirm 1000ms binary vs 100ms |
| ACK/Error `0xFF/0xFE`/`sendResponse` | **EXISTING** | Confirm Jetson will parse |

---

## 14. Team Confirmation Checklist

For Jetson teammate — please confirm (max 12):

1. Frame has `LEN` byte at position 2? (`0xAA ID LEN PAYLOAD CRC FOOTER`)
2. LEN is `payload length` 0-64?
3. CRC covers `header+ID+LEN+PAYLOAD` (3+LEN bytes) LE?
4. Fan Level 1-5 mapping to payload `1-5` and `Level→PWM` table (`51*n` vs calibrated)?
5. Temperature 24-32 must be supported — should ESP widen `30→32` or Jetson limit to `30`?
6. `+2/-2` (4/5) payload encoding — `0x04/0x05` marker as absolute delta, or different?
7. `FACE` enum — `8→0 VENT` correct, or `BI_LEVEL 1`?
8. `FOOT` enum — `9→2 FLOOR` correct?
9. Status frame ID — `0x90` 6-field vs reuse `0x80/0x81/0x82`?
10. Status 6-field binary layout — `[engine,temp,AC,wind,last_mode,door]` order and `engine/door` source?
11. Status frequency — `1000ms` binary acceptable vs Jetson 100ms?
12. `engine`/`door`/`wind`/`last_mode` sources — define or remove from status?

---

## Final Freeze Status

**H03.2 PROTOCOL FREEZE STATUS: READY FOR TEAM CONFIRMATION**

**Not** `READY FOR IMPLEMENTATION`. Architecture (UART 17/18 115200, binary `0xAA...0x55`, CRC, `ProtocolConfig` baseline) is **CONFIRMED and frozen**. All semantic-to-payload mappings with `TBD`/`PROPOSED`/`SOURCE NOT AVAILABLE`/`BLOCKED` above require Jetson team sign-off before H03 is considered fully implemented. No production code will be changed until checklist §14 is signed.

