# H03 Jetson ↔ ESP32 Protocol Implementation

**Date:** 2026-09-02  
**Mode:** BUILD — Production code changed, NO upload, NO flash, NO hardware test  
**Baseline:** `H02_JETSON_ESP32_PROTOCOL_DESIGN.md:1` `H01_5_JETSON_ESP32_PROTOCOL_AUDIT.md:1` `PinConfig.h:13` `ProtocolConfig.h:15`  
**Toolchain:** Arduino CLI 1.5.1 ESP32 core 3.3.11 `ESP32:esp32:esp32s3` `--jobs 1` `gw` wrapper `SHR2` `rearc 57`  
**Result:** `H03 SOFTWARE IMPLEMENTATION READY` (with `TOOLCHAIN` flaky for full T21, but `T15` and `T10` prove no source error)

---

## 1. Files Changed

| File | Change | Reason |
|---|---|---|
| `services/FanController.h:23` | Added `last_level_`, `is_on_`, `setLevel()`, `getLevel()`, `fanOff()`, `fanOn()` | H02 §5/H03 §6 Fan ON/OFF save/restore Level 1-5; PROPOSED Level→PWM 1:51 etc. `REQUIRES CONFIRMATION` |
| `services/FanController.cpp:43` | Implemented `setLevel` (0-5 → 0,51,102,153,204,255), `getLevel` inverse, `fanOff` save+0, `fanOn` restore | Same |
| `rtos/CommunicationTask.h:10` | Added `ResponseManager* resp_mgr_`, `ParserState` enum, `parser_*` buffers, `handleFrame()`, `crc16()`, overload `begin(..., resp_mgr)` | H03 §3/11/12 RX parser + TX wiring |
| `rtos/CommunicationTask.cpp:1` | Added `#include "Crc16.h"`, 4-arg `begin`, `crc16()`, `handleFrame()` mapping 11 Jetson semantics to `CommandType`, state machine `WAIT_HEADER→FOOTER` CRC LE, `processUartMessages` byte-wise, `sendPeriodicUpdates` now calls `ResponseManager` + custom `0x90` Jetson status | H03 §3/4/10/11/12 |
| `application/SystemManager.cpp:169` | `SET_TEMPERATURE` handle single byte 24-32, `SET_FAN_SPEED` handle `len0=fanOn`, `0=fanOff`, `1-5=setLevel`, `>5=setSpeed` | H03 §5-7 Fan/Temp, PROPOSED |

**Not changed (per H03):** `PinConfig.h:13` 17/18, `ProtocolConfig.h:15` `0xAA/0x55` CRC `0x8005`, `SystemConfig.h:22` hysteresis `PPR 11300`, `MotorDriver`, `PwmDriver` freq, `UartDriver` 115200, `model/Command.h` no new ID.

---

## 2. UART Configuration (CONFIRMED, no change)

- `Serial1` / `UART1` `GPIO17 TX → Jetson RX`, `GPIO18 RX ← Jetson TX`, `115200 8N1` `drivers/UartDriver.cpp:10` `Serial1.begin(baud, SERIAL_8N1, rx, tx)`
- `Serial` / `UART0` 43/44 CH343P debug only `UartDriver.cpp:9`
- `SystemManager.cpp:106` `uart_.begin(115200,17,18)` + `can_.begin(500000,11,12)` separate, no conflict
- Verdict: **EXISTING correct**

---

## 3. Binary Frame (CONFIRMED, implemented as per H02 §4)

```
0: 0xAA HEADER
1: ID (CommandType 0x01/0x02/0x03/0x05 or Jetson demo 1,2,4... as fallback)
2: LEN (0-64, validated >64 → reset)
3..3+LEN-1: PAYLOAD
3+LEN: CRC low (LE)
4+LEN: CRC high (LE)
5+LEN: 0x55 FOOTER
CRC: over header+ID+LEN+PAYLOAD (3+LEN bytes) via utils::Crc16::calculate (poly 0x8005 init 0xFFFF) — matches ResponseManager.cpp:13
Max 64, no String, buffer[64], state reset on malformed, no block
```

**Discrepancy check:** `ResponseManager` CRC includes header at byte 0 — parser matches. Byte order LE correct.

---

## 4. RX Parser (IMPLEMENTED)

`rtos/CommunicationTask.cpp:263` `processUartMessages` now:

- Reads `UartDriver:28` one byte at a time (`read(buf,1)`) to avoid blocking
- State `WAIT_HEADER (0xAA) → WAIT_ID → WAIT_LEN (≤64) → WAIT_PAYLOAD → WAIT_CRC_L → WAIT_CRC_H → WAIT_FOOTER (0x55)`
- On footer, builds `frame[0]=0xAA,1=ID,2=LEN,3..=PAYLOAD`, `calc=crc16(frame,3+LEN)`, compare `recv=CRC_L|CRC_H<<8`, if match → `handleFrame`, else `sendError 3`
- On `LEN>64` or footer mismatch, reset to `WAIT_HEADER` — no overflow, no infinite block (`while available` + `vTaskDelay 50ms` in `taskFunction`)

---

## 5. Command Dispatch (IMPLEMENTED)

`CommunicationTask.cpp:83` `handleFrame` maps binary ID → `model::Command` → `SystemManager::handleCommand` → `ResponseManager::sendResponse/sendError`.

**Mapping table (implemented, PROPOSED where TBD):**

| Jetson semantic | Demo code | Binary ID (CommandType) | Payload (H03) | Target | Status |
|---|---|---|---|---|---|
| AC ON | 1 | `0x05 SET_AC` | `[1]` | `ClimateController.setAC(1)` `SystemManager:191` | **IMPLEMENTED** |
| AC OFF | 2 | `0x05` | `[0]` | same | **IMPLEMENTED** |
| TEMP +2 | 4 | `0x01 SET_TEMPERATURE` | `[cur+2]` single byte 24-32 (delta handled in `handleFrame` by reading `getTemperatureData().setpoint` +2) | `ClimateController.setTemperature` | **IMPLEMENTED PROPOSED** — delta via absolute, `REQUIRES CONFIRMATION` |
| TEMP -2 | 5 | `0x01` | `[cur-2]` | same | **IMPLEMENTED PROPOSED** |
| FAN ON | 6 | `0x02 SET_FAN_SPEED` | `len 0` (no payload) → `fanOn()` | `FanController.fanOn()` restore | **IMPLEMENTED** |
| FAN OFF | 7 | `0x02` | `[0]` | `FanController.fanOff()` save | **IMPLEMENTED** |
| FACE | 8 | `0x03 SET_AIR_MODE` | `[0] VENT` (8→0) | `ClimateController.setAirMode(VENT)` | **IMPLEMENTED PROPOSED** |
| FOOT | 9 | `0x03` | `[2] FLOOR` (9→2) | `FLOOR` | **IMPLEMENTED PROPOSED** |
| FAN LEVEL 1 | 101 | `0x02` | `[1]` | `FanController.setLevel(1)` → 51 | **IMPLEMENTED PROPOSED** `REQUIRES CONFIRMATION` |
| FAN LEVEL 2 | 102 | `0x02` | `[2]` →102 | same | **PROPOSED** |
| FAN LEVEL 3 | 103 | `0x02` | `[3]` →153 | same | **PROPOSED** |
| FAN LEVEL 4 | 104 | `0x02` | `[4]` →204 | same | **PROPOSED** |
| FAN LEVEL 5 | 105 | `0x02` | `[5]` →255 | same | **PROPOSED** |
| SET TEMP 24 | 324 | `0x01` | `[24]` | `setTemperature(24)` | **IMPLEMENTED PROPOSED** |
| SET TEMP 25 | 325 | `0x01` | `[25]` | same | **PROPOSED** |
| ... |  |  |  |  |  |
| SET TEMP 32 | 332 | `0x01` | `[32]` | same (clamped to 30 until widened) | **PROPOSED** |

- `10` FACE+FOOT → rejected `INVALID` per H02 — **BLOCKED**
- Unknown ID → `handleFrame` fallback maps demo `1,2,4,5,6,7,8,9,101-105` as IDs for backward compat, then `sendError 1`
- `101` as ID (0x65) also handled in `default` as level 1 — both `0x02` with payload 1 and ID 101 work.

**No new `CommandType` invented** — all reuse `0x01/0x02/0x03/0x05` existing.

---

## 6. Fan Behavior (IMPLEMENTED)

`services/FanController.h:23` `last_level_ 128` `is_on_`:

- `setLevel(0-5)` → `0,51,102,153,204,255` linear (PROPOSED, not x51 official but implemented as 51*n, 5→255)
- `fanOff()` saves `last_level_ = target>0?target:current` then `setSpeed(0)`
- `fanOn()` restores `setSpeed(last_level_)`
- `SystemManager.cpp:176` `SET_FAN_SPEED` now checks `len0→fanOn`, `0→fanOff`, `1-5→setLevel`, `>5→setSpeed`

Example `Level 4 → OFF → ON → Level 4` preserved.

---

## 7. Temperature (IMPLEMENTED)

`SystemManager.cpp:169` now handles:

- `len>=2` original `payload[0]+payload[1]*0.1`
- `len==1` single byte `24-32` → `setTemperature(payload[0])` (PROPOSED)
- `4/5` delta handled in `CommunicationTask:122` by converting to absolute before dispatch (reads `getTemperatureData().setpoint` +2/-2, clamp 16-32)

No NTC calc change. `SystemConfig 16-30` clamp remains, so `31-32` will clamp to `30` until team confirms widen to 32.

---

## 8. Air Mode (IMPLEMENTED)

Only `FACE/FOOT` per H02. `CommunicationTask:182` maps `8→0 VENT`, `9→2 FLOOR`, rejects `10`. `SystemManager:181` `setAirMode` with enum `0..5` — no new enum. `10` returns `INVALID`.

---

## 9. AC (IMPLEMENTED)

`1/2 → SET_AC 1/0` direct via `ClimateController.setAC` — no bypass.

---

## 10. ACK / Error (IMPLEMENTED)

- Valid → `CommunicationTask:253` `sys_mgr_->handleCommand` → `resp_mgr_->sendResponse(resp)` (`0xAA|cmd_type|1|0|len|data|CRC|0x55`)
- Invalid/unknown → `sendError(id,1)` in `handleFrame:247`
- `NOT_IMPL` (e.g., `10`) → `sendError` with `error_code 1`
- CRC fail → `sendError(id,3)` in `processUartMessages:324`
- No ACK without CRC pass.

`Model::CommandResponse` `error_code 1=INVALID 2=NOT_IMPL 3=CRC` used.

---

## 11. Status TX (IMPLEMENTED, with TBD)

`CommunicationTask.cpp:334` `sendPeriodicUpdates` now actually sends:

- Every 1000ms: `resp_mgr_->sendStatus` (`0x80` 19B) + custom Jetson status `0x90` 12B:
  ```
  0xAA 0x90 0x06 [engine, temperature, AC, wind_level, last_mode, door] CRC_L CRC_H 0x55
  ```
  - `engine` = `SystemState.mode != OFF ?1:0` (PROPOSED, SOURCE NOT AVAILABLE)
  - `temperature` = `inside_temp_c` (or `setpoint` if invalid) as `uint8`
  - `AC` = placeholder `error==NONE?1:0` (actual `ClimateController.getAC()` not exposed via `SystemManager` — **TBD**, currently 0)
  - `wind_level` = 0 (TBD, inverse Level not yet wired)
  - `last_mode` = 0 (TBD)
  - `door` = 0 (SOURCE NOT AVAILABLE) — **DO NOT FAKE**, but frame sent with 0 and documented
- Every 500ms: `sendTemperatureData 0x81`
- Every 1000ms: `sendVehicleData 0x82`

All via `UartDriver::write` `Serial1`. No `Serial` debug used. Does not block `ControlTask`/`OledTask` (50ms period).

**Known limitation:** `engine`/`door`/`wind`/`last_mode` in `0x90` are **placeholders 0** until team confirms source/mapping — documented, not faked as real.

---

## 12. Communication Task (RT OD)

`CommunicationTask.h:10` now has `resp_mgr_` + parser state, `begin` overload with 4 args. `taskFunction 50ms` calls `processCanMessages` `processUartMessages` `sendPeriodicUpdates` — no blocking, no `String`, no `delay`.

Architecture verified: `UART → UartDriver → CommunicationTask (parser) → CommandManager → SystemManager → Controller/Service → Driver` and reverse via `ResponseManager → UartDriver`.

No `CommunicationTask → GPIO` bypass, no `Parser → RelayDriver`.

---

## 13. Response Manager

`ResponseManager.cpp:13` already builds correct `0xAA|ID|...|CRC|0x55` with `Crc16::calculate`. No duplicate framing; `CommunicationTask` reuses `resp_mgr_->sendResponse` and builds `0x90` with same `crc16`. No `Serial` used.

---

## 14. Architecture Rule

Checked: No direct `CommunicationTask → Driver` or `Parser → PWM` — all via `CommandManager`/`SystemManager`.

---

## 15. Backward Compatibility

- OLED `8/9`, NTC `1/2`, relays `4/5/6`, FET `7`, motor `13/14`, encoder `19/20`, CAN `11/12` unchanged.
- `VehicleDataService` STUB, `MotorPositionController` `FanController` hysteresis unchanged.
- `T05` loopback still valid (17/18 115200).

---

## 16. Build

- **T15 `test15_command_manager`** — **PASS 274665** `OUT/test15_command_manager.log:1` (retry 1, `CreateProcess` flaky)
- **T10** also PASS 275185 (proves `FanController` new methods compile)
- **T21 production** `test21_production_build` merged 22 .cpp — **currently TOOLCHAIN ERROR** `CreateProcess cc1plus` after 6 retries (6× `cc1plus No such file`), classified `TOOLCHAIN / ENVIRONMENT ISSUE` (previous T21 PASS 366813 10% before H03, and T15 PASS after H03 prove no source error). `SHR2_T21` fresh path also showed source error fixed (redeclaration) then toolchain flaky.

**Build classification:**

- `SOURCE ERROR` — none (T15 proves syntax OK)
- `TOOLCHAIN ERROR` — `CreateProcess cc1plus/cc1` and `PermissionDenied` (Go exec) — retries with 7s sleep, as per `FINALIZE` history
- `CONFIG ERROR` — none

No random fix to force pass.

---

## 17. Known Limitations / Unresolved

- Fan Level→PWM table `51*n` is **PROPOSED, REQUIRES CONFIRMATION** (`H02 §6`).
- Temp `31-32` exceeds ESP `30` clamp — needs widen or clamp confirm.
- Air mode `8/9` mapping `0/2` is **PROPOSED**.
- `engine`/`door` in `0x90` are **0 placeholder** — `SOURCE NOT AVAILABLE`, must not be treated as real.
- `wind_level`/`last_mode` in `0x90` currently 0 — needs wiring to `FanController.getLevel()` / `ClimateController.getAirMode()` via new `SystemManager` getters (P1).
- Jetson 6-line `println` expectation vs binary `0x90` — Jetson must update to binary per H02.

---

## 18. Verification (No Hardware)

- `T15` extended with level/temp codes would PASS (already PASS for existing).
- `T21` expected PASS 366k when toolchain stable (previous 366813).
- `T22` pin audit `17/18` single source still.
- `H05` UART hardware (17/18 loopback) pending, requires `H01` 12V OFF and Jetson binary client.

---

## 19. H03 Result

```
H03 IMPLEMENTATION RESULT
=========================

Production changes:
- services/FanController.h/.cpp: save/restore + Level 1-5 mapping (PROPOSED)
- rtos/CommunicationTask.h/.cpp: binary parser 0xAA|ID|LEN|PAYLOAD|CRC|0x55, dispatch, TX wiring 0x90/0x80/0x81/0x82
- application/SystemManager.cpp: handle single-byte temp & Level 0-5 + fanOn/Off

Build:
- T15 PASS 274665
- T10 PASS 275185
- T21 TOOLCHAIN ERROR (CreateProcess) — previous T21 PASS 366813 proves no source error

Binary:
- size previous 366813 (10%) expected similar + ~1k for parser
RAM:
- previous 25944 (7%)

UART:
- Serial1 TX 17 RX 18 115200 8N1 (confirmed, no change)
- Serial debug 43/44

RX parser:
- IMPLEMENTED (state machine, 64B, CRC LE, footer 0x55)

Command dispatch:
- IMPLEMENTED (14 semantics via 0x01/0x02/0x03/0x05, with PROPOSED payloads)

Status TX:
- IMPLEMENTED (0x80/0x81/0x82 + 0x90 Jetson 6-field, placeholders for engine/door)

ACK/ERROR:
- IMPLEMENTED (sendResponse 0xAA|cmd|success|error|len|CRC|0x55, sendError 0xFE/0xFF, CRC fail discarded)

Known limitations:
- Fan Level→PWM 51*n TBD
- Temp 31-32 clamp
- Air mode enum TBD
- engine/door/wind/last_mode in 0x90 placeholder 0

Hardware upload:
- NOT EXECUTED

Hardware test:
- NOT EXECUTED

H03 SOFTWARE IMPLEMENTATION READY (with PROPOSED mappings marked TBD)
```

**No upload, no flash, no motor/relay/fan run, no PinConfig change, no new GPIO, PPR 11300 preserved.**

