# JETSON ↔ ESP32 Protocol Spec V1 — DRAFT FOR TEAM CONFIRMATION

**Date:** 2026-09-02  
**Status:** `DRAFT FOR JETSON TEAM CONFIRMATION` — **NOT** `READY FOR IMPLEMENTATION`  
**Mode:** Documentation only — NO production code/config change, NO build, NO flash in this step  
**Sources:** `H03_2_JETSON_ESP32_PROTOCOL_FREEZE.md:1` `H03_1_IMPLEMENTATION_REVIEW.md:1` `H02_JETSON_ESP32_PROTOCOL_DESIGN.md:1` `H01_5_JETSON_ESP32_PROTOCOL_AUDIT.md:1` `config/ProtocolConfig.h:1` `model/Command.h:1` `application/ResponseManager.*:1` `services/{FanController,ClimateController}.*:1` `rtos/CommunicationTask.*:1` + **Latest teammate input (2026-09-02)** `1,2,3,4,5,6,7,8,9,10,323-330`  
**UART Baseline:** `PinConfig.h:13` `drivers/UartDriver.*:1`  
**Previous demo:** `Serial` ASCII `9600` `code\r` `6x println` is **NOT** production.

---

## 1. Purpose

- **Jetson Nano:** AI / user command processing, sends **COMMAND** to ESP32, receives **STATUS/ACK/ERROR** from ESP32. Does **not** drive GPIO/PWM directly.
- **ESP32-S3:** Receives command via `Serial1`, validates binary frame, dispatches via `CommandManager → SystemManager → Controller/Service → Driver → Hardware`, sends `ACK/ERROR` and periodic `STATUS` via `ResponseManager → UartDriver → Serial1` to Jetson.
- Both sides implement **the same binary frame** `0xAA | ID | LEN | PAYLOAD | CRC16 | 0x55` per `ProtocolConfig.h:15`.

---

## 2. Physical UART

**CONFIRMED — No change**

| Item | Value |
|---|---|
| ESP32 TX | `GPIO17` → Jetson RX |
| ESP32 RX | `GPIO18` ← Jetson TX |
| GND | Common `ESP32 GND ↔ Jetson GND` — mandatory |
| UART | `UART1 / Serial1` |
| Baud | `115200` |
| Format | `8N1` (`8 data, No parity, 1 stop`) `SERIAL_8N1` `UartDriver.cpp:10` |
| ESP32 API | `Serial1.begin(115200, SERIAL_8N1, 18, 17)` `SystemManager.cpp:106` |
| Jetson device | `/dev/ttyTHS1` or USB-UART bridge at 115200 (Jetson side to confirm) |
| Debug UART | `Serial / UART0` `CH343P 43/44` **reserved for USB/debug/upload** — **not** Jetson channel |

**Status:** `CONFIRMED` — do not move Jetson to `Serial`/`USB`.

---

## 3. Binary Frame

**PROTOCOL DECISION / REQUIRES FINAL TEAM CONFIRMATION** for `LEN` position (see below). All other fields **CONFIRMED** per `ProtocolConfig.h:15` and `ResponseManager.cpp:13`.

```
Byte 0      : HEADER 0xAA (`ProtocolConfig.h:15` `CMD_HEADER`)
Byte 1      : ID (Command/Frame identifier)
Byte 2      : LEN (payload length, 0..64) — PROPOSED at pos 2
Byte 3..3+LEN-1 : PAYLOAD (LEN bytes, 0..64)
Byte 3+LEN  : CRC16 low  (LE)
Byte 4+LEN  : CRC16 high (LE)
Byte 5+LEN  : FOOTER 0x55 (`CMD_FOOTER`)
Total: 6+LEN bytes
```

**HEADER:** `0xAA` — **CONFIRMED**

**ID:** Command/frame identifier (`model/Command.h:7` or `ResponseManager` `0x80/0x81/0x82/0xFE/0xFF/0x90`) — see §4.

**LEN:** Payload length **not counting** `HEADER/ID/LEN/CRC/FOOTER` — `0..64` (`CMD_MAX_PAYLOAD 64`). **Note:** `ProtocolConfig.h:15` defines `HEADER/FOOTER/MAX` but **does not define a `LEN` byte**. `ResponseManager.cpp:13` **does** use `LEN` at byte 4 (`resp_len`). H03 implementation adds `LEN` at byte 2 (`CommunicationTask.cpp:282`). This is **PROTOCOL DECISION / REQUIRES FINAL TEAM CONFIRMATION** — team must confirm Jetson will send `LEN` at byte 2. Do not pretend LEN is already confirmed.

**PAYLOAD:** Command-specific, see §4-10. `LEN=0` allowed (e.g., `FAN ON` restore).

**CRC16:**

- Polynomial `0x8005`, Init `0xFFFF`, `utils/Crc16.h:9` — **CONFIRMED**
- **Little Endian** — `ResponseManager.cpp:25` `crc &0xFF` then `>>8`, `CommunicationTask.cpp:318` `CRC_L | CRC_H<<8` — **CONFIRMED**
- **Coverage — MUST BE EXACT:**
  - **Command frame (Jetson → ESP32):** `CRC over HEADER + ID + LEN + PAYLOAD` (3+LEN bytes: `frame[0]=0xAA, frame[1]=ID, frame[2]=LEN, frame[3..]=PAYLOAD`) — **H03 design** `CommunicationTask.cpp:318` `crc16(frame,3+LEN)`
  - **Response frame (ESP32 → Jetson):** `CRC over HEADER + ID + success + error_code + LEN + PAYLOAD` (5+LEN bytes: `frame[0]=0xAA,1=cmd_type,2=success,3=error,4=len,5..=payload`) — `ResponseManager.cpp:24` `crc16(frame,5+len)` for `sendResponse`; `sendStatus` `16` bytes, `sendTemperature` `16` bytes, `sendVehicle` `14` bytes
  - **If Jetson implements CRC over different coverage, frames will be rejected.** Do not say "CRC over whole frame" — specify above.
- `MAX PAYLOAD 64` — **CONFIRMED** `ProtocolConfig.h:17`

**FOOTER:** `0x55` — **CONFIRMED**

**TIMEOUT:** `1000ms` `CMD_TIMEOUT_MS` `ProtocolConfig.h:18` — for Jetson to retry if no `ACK` within 1000ms.

**No new field invented as CONFIRMED.**

---

## 4. Command ID

**Latest teammate semantics (2026-09-02) — CONFIRMED as semantics:**

| Demo Code | Semantic |
|---|---|
| 1 | AC ON (Mở điều hòa) |
| 2 | AC OFF (Tắt điều hòa) |
| 3 | SET TEMPERATURE (Nhiệt độ chỉ định) — **new vs H02** |
| 4 | Temperature +2°C |
| 5 | Temperature -2°C |
| 6 | Fan ON (Mở quạt) |
| 7 | Fan OFF (Tắt quạt) |
| 8 | FACE (Hướng gió lên mặt) |
| 9 | FOOT (Hướng gió xuống chân) |
| 10 | DEFROST / WINDSHIELD (Hướng gió sưởi kính) — **new vs H02 `FACE+FOOT`** |
| 323 | 23°C |
| 324 | 24°C |
| 325 | 25°C |
| 326 | 26°C |
| 327 | 27°C |
| 328 | 28°C |
| 329 | 29°C |
| 330 | 30°C |

**Binary encoding — NOT CONFIRMED as ID:**

- Teammate codes `1,2,3,4,5,6,7,8,9,10` fit in **1-byte ID** `0x01-0x0A`.
- Codes `323-330` do **NOT fit in 1-byte ID** (`323=0x143 >255`). H02 correctly noted this. **Must be payload, not ID.**

**H03 mapping (PROPOSED, not CONFIRMED):**

| ID (binary) | COMMAND (existing `Command.h:7`) | Semantic | PAYLOAD (PROPOSED) | STATUS |
|---|---|---|---|---|
| `0x05` | `SET_AC` | AC ON | `[1]` | **TBD** — confirm `1→1` |
| `0x05` | `SET_AC` | AC OFF | `[0]` | **TBD** |
| `0x01` | `SET_TEMPERATURE` | SET TEMP absolute | `[temp]` single byte `23-30` (see §5) | **TBD** — single vs 2-byte |
| `0x01` | `SET_TEMPERATURE` | +2 | `[0x04]` marker or absolute `cur+2` | **TBD** — delta encoding |
| `0x01` | `SET_TEMPERATURE` | -2 | `[0x05]` marker | **TBD** |
| `0x02` | `SET_FAN_SPEED` | FAN ON restore | `LEN 0` (no payload) | **TBD** — Jetson must send `LEN 0` |
| `0x02` | `SET_FAN_SPEED` | FAN OFF save | `[0]` | **TBD** |
| `0x03` | `SET_AIR_MODE` | FACE | `[0] VENT` (8→0) | **TBD** |
| `0x03` | `SET_AIR_MODE` | FOOT | `[2] FLOOR` (9→2) | **TBD** |
| `0x03` | `SET_AIR_MODE` | DEFROST | `[4] DEFROST` (10→4) | **TBD** — H02 had `10` as `FACE+FOOT` to reject, now `DEFROST` |
| `0x02` | `SET_FAN_SPEED` | LEVEL 1-5 | `[1]-[5]` (101-105 → 1-5) | **TBD** — Level vs PWM |
| `0x01` | `SET_TEMPERATURE` | 23-30 | `[23]-[30]` (323-330 → 23-30) | **TBD** |

**ID size:** **REQUIRES CONFIRMATION** — is `ID` 1 byte (`CommandType` `0x01-0xF1`) or 2-byte for `323`? **Proposed is 1-byte `CommandType` with temp in payload**, not `323` as ID.

**Do not invent new `CommandType` ID** until team confirms.

---

## 5. Temperature

**Teammate latest (CONFIRMED range 23-30):**

- `323 → 23°C` ... `330 → 30°C` — **CONFIRMED per teammate 2026-09-02** (replaces H02 `324-332 →24-32`)
- No `31/32` — **do not extend to 31/32**
- Relative `4 → +2°C`, `5 → -2°C` — **CONFIRMED semantics**

**Current ESP code:**

- `ClimateController.h:31` `Config 16.0-30.0` `ClimateController.cpp:34` `clamp 16-30` — **EXISTING** matches `23-30`, so **no widen needed** (previous H02 `24-32` would have required `32`, now `30` fits)
- `SystemManager.cpp:169` handles `len>=2` `payload[0]+payload[1]*0.1` and `len==1` single byte (H03) — **EXISTING after H03**

**Payload encoding — TBD:**

- Absolute `23-30` as **single byte** `23-30` (**PROPOSED**) vs **2-byte `*10`** (`24.0→240` LE) as in `ResponseManager` temps — **REQUIRES CONFIRMATION**
- Relative `+2/-2` as **delta marker** `0x04/0x05` (**PROPOSED** in `CommunicationTask.cpp:121`) vs new `CommandType` — **TBD**
- `3 → SET TEMPERATURE` (new code 3) — **no payload defined yet** — is it same as `323-330` or separate? **TBD**

**No new temperature state invented.**

---

## 6. Fan

**CONFIRMED per H02 §4 & teammate:**

- Jetson only knows **Level 1..5**, **never raw PWM `0-255`**
- ESP32 owns `Level → PWM` via `FanController`/`PwmDriver` (`FET 7` `1000Hz 8-bit`)

**Current H03 code:**

- `FanController.h:23` `setLevel(1-5)` `FanController.cpp:54` `0→0,1→51,2→102,3→153,4→204,5→255` — **PROPOSED / REQUIRES CONFIRMATION** (linear `*51`, not from spec)
- Alternative calibrated `60,110,160,210,255` needs H06

**Fan ON/OFF (CONFIRMED semantics):**

- `7 FAN OFF` → `FanController.cpp:79` `fanOff()` saves `last_level_ = target/current` then `setSpeed(0)`
- `6 FAN ON` → `fanOn()` restores `setSpeed(last_level_)`
- `SystemManager.cpp:176` `len0→fanOn`, `0→fanOff`, `1-5→setLevel`

**Status:** Level semantics **CONFIRMED**, `51*n` table **PROPOSED — REQUIRES CONFIRMATION / H06 CALIBRATION** — do not treat as CONFIRMED.

---

## 7. Air Mode

**Teammate latest (CONFIRMED):**

- `8 → FACE` (Hướng gió lên mặt)
- `9 → FOOT` (Hướng gió xuống chân)
- `10 → DEFROST / WINDSHIELD` (Hướng gió sưởi kính)

**Conflict with H02:** H02 §7 previously `CONFIRMED` only `FACE/FOOT` and said `10 = FACE+FOOT → MUST REJECT`. Now teammate says `10 = DEFROST`. **This is new information — 10 is valid as `DEFROST`, not to be rejected.**

**Current ESP enum** `ClimateController.h:11` `AirMode { VENT0, BI_LEVEL1, FLOOR2, MIX3, DEFROST4, FLOOR_DEFROST5 }`

**H03 mapping (PROPOSED, now updated for 10):**

- `FACE (8)` → `VENT 0` (**PROPOSED**, could be `BI_LEVEL 1` if face+windshield)
- `FOOT (9)` → `FLOOR 2`
- `DEFROST (10)` → `DEFROST 4`

Exact numeric **REQUIRES CONFIRMATION** — use enum names, not raw ints. No `FACE+FOOT` mode will be implemented.

---

## 8. AC

**CONFIRMED:**

- `1 → AC ON`
- `2 → AC OFF`

Via `SET_AC 0x05` `[1]/[0]` `SystemManager.cpp:191` `climate_ctrl_.setAC(payload!=0)` → `RelayDriver` `PIN_AC_RELAY 4` (`SystemManager.cpp:75` `active_high true` — trigger level TBD per H01.3).

**No new status source invented.**

---

## 9. ACK / Error

**Current `ResponseManager` / `CommunicationTask` (EXISTING, no change):**

| Case | Frame (ESP → Jetson) | Code |
|---|---|---|
| **Accepted** | `0xAA | cmd_type | 1 | 0 | resp_len | resp_data | CRC | 0x55` | `ResponseManager.cpp:13` `sendResponse` `success 1` `error 0` |
| **Invalid command** | `0xAA | cmd_id | 0 | error_code | 0 | CRC | 0x55` via `sendResponse` or `sendError` | `CommandManager.cpp:38` `error 1=INVALID` `CommunicationTask.cpp:247` `sendError 1` |
| **NOT_IMPL** | same `error 2` | `CommandManager` `2=NOT_IMPL` |
| **Invalid payload** | `sendResponse` with `error 1` or `sendError` | `SystemManager.cpp:161` early-return |
| **CRC error** | **No ACK**, frame discarded, optionally `sendError 0xFE` with `error 3` (`CommunicationTask.cpp:324` `sendError 3`) | **EXISTING** |
| **Malformed (LEN>64, bad footer)** | Discard, reset to `WAIT_HEADER` | `CommunicationTask.cpp:284` `312` |

**IDs:** `sendAck 0xFF` 8B `cmd_id,success` `sendError 0xFE` 8B `cmd_id,error_code` (`ResponseManager.cpp:107`).

**Jetson must:** Validate `HEADER` `LEN` `CRC` `FOOTER`, timeout `1000ms` (`ProtocolConfig.h:18`) if no `ACK` then retry.

**No new error code invented.**

---

## 10. ESP32 → Jetson Status

**Jetson demo wants (from H01.5/H02):** `engine, temperature, AC, wind_value, last_mode, door` every ~100ms via 6 `println`.

**H02/H03 rule:** **DO NOT FAKE** if no source.

| Field | Source audited | Current H03 `0x90` (PROPOSED) | Status |
|---|---|---|---|
| `engine` | `VehicleData.h:1` `ignition_on/engine_running` or `SystemState.mode` — **no `engine` int** | `jet_payload[0]=mode!=OFF?1:0` `CommunicationTask.cpp:356` | **SOURCE NOT AVAILABLE / PROPOSED** — placeholder `0` must not be treated as real |
| `temperature` | `TemperatureData.inside_temp_c` NTC1 `SystemManager.cpp:129` or `setpoint_temp_c` | `jet_payload[1]=(uint8)(inside_valid?inside:setpoint)` `CommunicationTask.cpp:358` | **TBD** — which temp? `inside` vs `setpoint` |
| `AC` | `ClimateController.getAC()` — not exposed via `SystemManager` (H03 uses `SystemState.error` placeholder) | `jet_payload[2]=error==NONE?1:0` **wrong source** | **BLOCKED** — fix to `ClimateController` |
| `wind_value` | `FanController.getLevel() 1-5` / `getSpeed() 0-255` | `jet_payload[3]=0` hardcode | **TBD** — should be Level 1-5, currently 0 |
| `last_mode` | `ClimateController.getAirMode()` | `jet_payload[4]=0` hardcode | **TBD** |
| `door` | `VehicleData` has no `door` | `jet_payload[5]=0` | **SOURCE NOT AVAILABLE** — must not fake |

**Existing `ResponseManager` frames (EXISTING, real sources):**

- `0x80` `sendStatus` 19B `mode,error,uptime,heap,cpu,watchdog` — real
- `0x81` `sendTemperatureData` 19B 5 temps `*10` + valid — real
- `0x82` `sendVehicleData` 17B `speed, rpm, coolant, batt, ac, blower, gear, valid` — real but no `engine/door/wind/last_mode`

**`0x90` 6-field frame (`0xAA 0x90 0x06 [6] CRC 0x55` 12B) is PROPOSED and currently sends placeholders — `BLOCKED` until team confirms binary layout vs reusing `0x80/0x81/0x82` vs 6-line ASCII.**

**Rule:** Do not send `0` placeholder as if it were `engine OFF`/`door closed`.

---

## 11. Status Frequency

- H03 `CommunicationTask.cpp:344` `1000ms` status (`0x80`+`0x90`), `500ms` temp (`0x81`), `1000ms` vehicle (`0x82`)
- Demo: `delay(100)` → 6 lines every 100ms

**Status:** `REQUIRES TEAM CONFIRMATION` — Jetson must confirm if `1000ms` binary acceptable or needs `100ms`.

---

## 12. Example Frames

**Only semantic examples — no HEX until ID/LEN/CRC confirmed.** `LEN` and `CRC` are `REQUIRES CONFIRMATION`, so no byte sequence is **CONFIRMED**.

- Jetson → ESP32: `"AC ON"` → semantics `1` → binary `SET_AC 0x05 [1]` (PROPOSED)
- Jetson → ESP32: `"FAN ON"` → `SET_FAN_SPEED` `LEN 0` (PROPOSED)
- Jetson → ESP32: `"FACE"` → `SET_AIR_MODE [0]` (PROPOSED)
- Jetson → ESP32: `"SET TEMPERATURE 24°C"` → `SET_TEMPERATURE [24]` (PROPOSED, single byte)
- ESP32 → Jetson: status via `0x80/0x81/0x82` (EXISTING) or `0x90` 6-field (PROPOSED)

**No HEX frame is CONFIRMED.**

---

## 13. Implementation Rules for Jetson

Teammate **must:**

- Use `UART 115200 8N1` on `17/18`, not `Serial` USB, not `9600`
- Implement parser per §3: validate `HEADER 0xAA`, `LEN 0-64`, `CRC LE` over `header+ID+LEN+payload`, `FOOTER 0x55`, reject malformed, handle `LEN>64` reset, no `String` blocking
- Not send raw PWM `0-255`, only `Level 1-5`
- Not change semantics `1,2,4,5,6,7,8,9,10,323-330`
- Not change `ID` size, `LEN` position, `CRC` poly/init/LE, `HEADER`/`FOOTER`
- Not assume `51*n`, `24-32` vs `2-byte`, `FACE/FOOT` enum, `0x90` layout, frequency — if `TBD`, **ask ESP32 team**

If `TBD` → **do not guess, ask**.

---

## 14. Source of Truth

`JETSON_ESP32_PROTOCOL_SPEC_V1.md` **is** the communication contract between Jetson and ESP32 after both teams confirm.

- Do **not** use demo sketch `Serial 9600` ASCII as production.
- All `CONFIRMED` items above are frozen; `PROPOSED`/`TBD` items require sign-off before implementation is considered complete.
- No `PinConfig`/`ProtocolConfig` change in this spec.

---

## 15. Team Confirmation

**Checklist for Jetson teammate — please tick:**

- [ ] UART 115200 8N1, `TX17→RX` `RX18←TX` `GND`, `Serial1` not `Serial`
- [ ] Binary frame `0xAA | ID | LEN | PAYLOAD | CRC16 LE | 0x55`, `LEN` at byte 2, `LEN` is payload len `0-64`
- [ ] `HEADER 0xAA` `FOOTER 0x55` `MAX 64`
- [ ] ID size 1 byte (`CommandType` `0x01/0x02/0x03/0x05` etc.)
- [ ] CRC `poly 0x8005 init 0xFFFF` LE, over `header+ID+LEN+payload` (3+LEN) for command
- [ ] CRC coverage same for response (`header+ID+success+error+len+payload`)
- [ ] Command `1→AC ON`, `2→AC OFF` as `SET_AC [1]/[0]`
- [ ] Command `3→SET TEMPERATURE` absolute (new), payload `single byte 23-30`
- [ ] Command `4→+2` `5→-2` as `SET_TEMPERATURE` delta (or absolute `cur±2`) — which encoding?
- [ ] Command `6 FAN ON` `LEN 0` restore, `7 FAN OFF` `[0]` save
- [ ] Fan Level `1-5` as `SET_FAN_SPEED [1]-[5]` Level, not PWM, `51*n` **PROPOSED**
- [ ] Temperature range `23-30` single byte, `31-32` not needed
- [ ] `FACE 8→0 VENT`, `FOOT 9→2 FLOOR`, `DEFROST 10→4 DEFROST` enum
- [ ] ACK `0xAA|cmd|1|0|len|data|CRC|0x55` / Error `0xFE/0xFF` / CRC fail discard
- [ ] Status frame `0x90` 6-field vs reuse `0x80/0x81/0x82` — which for production?
- [ ] Status 6-field layout `[engine,temp,AC,wind,last_mode,door]` order and `engine/door` source (or remove)
- [ ] Status frequency `1000ms` vs `100ms`

**STATUS:**

`DRAFT FOR JETSON TEAM CONFIRMATION`

**Not** `READY FOR IMPLEMENTATION` until all **CONFIRMED** items above are ticked and **TBD/PROPOSED** are either confirmed or removed.

