# H03.1 Implementation Review — Jetson ↔ ESP32 Protocol

**Date:** 2026-09-02  
**Mode:** READ-ONLY — NO code change, NO build, NO flash  
**Baseline:** `H02_JETSON_ESP32_PROTOCOL_DESIGN.md:1` `H01_5_JETSON_ESP32_PROTOCOL_AUDIT.md:1` `H03_JETSON_ESP32_PROTOCOL_IMPLEMENTATION.md:1` + production code `PinConfig.h:1` `ProtocolConfig.h:1` `services/FanController.*:1` `rtos/CommunicationTask.*:1` `application/SystemManager.*:1` `application/ResponseManager.*:1`

---

## 1. PASS

| Item | Location | Evidence |
|---|---|---|
| UART baseline | `PinConfig.h:13` `17/18` `drivers/UartDriver.cpp:10` `Serial1 115200 8N1` `SystemManager.cpp:106` | Correct, `Serial` debug 43/44 separate, no conflict 11/12, no GPIO change |
| Binary frame header/footer | `CommunicationTask.cpp:274` `0xAA` `312` `0x55`, `ProtocolConfig.h:15` `0xAA/0x55` `ResponseManager.cpp:16` `0xAA/0x55` | Matches |
| Payload max 64, buffer[64] | `CommunicationTask.cpp:284` `if byte>64 reset` `parser_payload_[64]`, `ProtocolConfig.h:17` `64` | Correct, no overflow |
| CRC algorithm | `CommunicationTask.cpp:79` `Crc16::calculate` `ProtocolConfig.h:24` `0x8005/FFFF` LE | Matches `ResponseManager.cpp:24` |
| AC ON/OFF | `CommunicationTask.cpp:101` `SET_AC 0x05` `[1]/[0]` → `SystemManager.cpp:191` `setAC` | Direct, no new ID |
| FAN ON/OFF save/restore | `FanController.h:38` `last_level_` `FanController.cpp:79` `fanOff` save, `fanOn` restore, `SystemManager.cpp:176` dispatch | Implements H02 §5 semantics |
| FACE/FOOT reject `10` | `CommunicationTask.cpp:187` `if mode==10 mapped=false` → `sendError 1` | Correct per H02 §7 |
| RX state machine | `CommunicationTask.cpp:263` `WAIT_HEADER→FOOTER` 7 states, byte-wise, no `String` | Correct |
| Malformed handling | `WAIT_LEN >64 reset`, `WAIT_FOOTER` reset regardless, `available()` loop | Correct, no block |
| CRC failure | `CommunicationTask.cpp:324` `sendError 3` on mismatch | Correct, no ACK on CRC fail |
| Command dispatch | `CommunicationTask.cpp:251` `handleFrame` → `sys_mgr_->handleCommand` → `resp_mgr_->sendResponse/sendError` | Correct via `CommandManager→SystemManager` |
| ACK/ERROR | `CommunicationTask.cpp:256` `sendResponse` on success, `sendError` on fail | Matches `ResponseManager` |
| Periodic TX wiring | `CommunicationTask.cpp:334` `sendPeriodicUpdates` now calls `sendStatus 0x80` `sendTemperature 0x81` `sendVehicle 0x82` + `0x90` | Wired (previously empty) |
| RTOS | `CommunicationTask.cpp:60` 50ms `vTaskDelayUntil`, `processUartMessages` one byte, `UartDriver` non-blocking | No block of Control/Oled |
| Baud/format | `115200 8N1` `UartDriver.cpp:10` `Serial1` | Correct |

---

## 2. FAIL

| Item | Location | Issue |
|---|---|---|
| STATUS 0x90 placeholder | `CommunicationTask.cpp:354` `jet_payload[0]=mode!=OFF?1:0` `jet_payload[2]=error==NONE?1:0` `jet_payload[3]=0` `jet_payload[4]=0` `jet_payload[5]=0` | **FAIL** — `engine`/`AC`/`wind_value`/`last_mode`/`door` sent as `0` placeholders. Jetson could interpret `0` as real `engine OFF`/`door closed`/`wind 0` — violates `DO NOT FAKE IT` H02 §9-10. `engine`/`door` have `SOURCE NOT AVAILABLE` and must not be sent as 0, or must be documented as invalid. `AC` uses `SystemState.error` not `ClimateController.getAC()`. `wind/ last_mode` hardcoded 0. |

---

## 3. BLOCKER

| ID | Issue | Location | H02 Status | Why BLOCKER |
|---|---|---|---|---|
| **B1 FAN Level→PWM** | `FanController.cpp:63` `level*51` `1→51,5→255` | `H02 §6` `REQUIRES CONFIRMATION` — H02 explicitly says `x51` is *POSSIBILITY, NOT APPROVED*, must not implement until confirmed. H03 implements as linear. | **BLOCKER** — implementation is **PROPOSED**, not `CONFIRMED`. Must be flagged, Jetson cannot rely on 51 steps until H06 calibration. |
| **B2 TEMP 24-32 clamp** | `ClimateController.h:31` `temp_min 16 max 30` + `FanController` etc., `H03` report `31-32 → clamp to 30` | `H02 §6` `24-32` vs `16-30` exceeds, `REQUIRES CONFIRMATION` whether to widen to 32. H03 keeps `30` clamp. | **BLOCKER** — requirement `24-32` not fully met; 31-32 will be rejected/clamped, Jetson expectation fails. |
| **B3 AIR MODE enum** | `CommunicationTask.cpp:185` `8→0 VENT`, `9→2 FLOOR` | `H02 §8` `PROPOSED` `REQUIRES CONFIRMATION` — mapping `8/9` to `0/2` is guess, not from `AirMode` spec. | **BLOCKER** — `FACE` could be `BI_LEVEL 1` not `VENT 0`; `FOOT` could be `FLOOR 2` correct but not confirmed. |
| **B4 TEMP delta 4/5** | `CommunicationTask.cpp:121` `payload 0x04→+2` via absolute conversion reading `getTemperatureData().setpoint` | `H02 §7` `4/5` delta vs `SET_TEMPERATURE` absolute `TBD` — H03 converts delta to absolute by reading setpoint, but setpoint may be stale (H01 hysteresis) and delta handling via `0x04/0x05` marker is **invented payload** not in `Command.h`. | **BLOCKER** — payload `0x04/0x05` not defined in `CommandType` spec; should be `TBD`. |
| **B5 STATUS 0x90 contract** | `CommunicationTask.cpp:372` `0x90` `6` fields `engine,temperature,AC,wind,last_mode,door` | `H02 §10` all 6 fields `TBD`/`SOURCE NOT AVAILABLE` for `engine`/`door`. H03 sends `0x90` with placeholders. | **BLOCKER** — no binary layout confirmed; Jetson expects 6 `println` vs binary `0x90` not agreed. |
| **B6 STATUS frequency** | `CommunicationTask.cpp:344` `1000ms` status + `500ms` temp | `H02 §10` `100ms` vs `50/500/1000ms` `TBD` | **BLOCKER** — period not confirmed. |

---

## 4. DISCREPANCY

| ID | H02 / ProtocolConfig | H03 Implementation | Location | Severity |
|---|---|---|---|---|
| **D1 LEN field** | `ProtocolConfig.h:15` defines `HEADER 0xAA`, `FOOTER 0x55`, `MAX 64`, `CRC`, `TIMEOUT` — **no explicit `LEN` byte** in config. `H02 §4` describes frame as `0xAA|ID|PAYLOAD|CRC|0x55` without `LEN` (payload length implied). `ResponseManager.cpp:13` actually has `LEN` at byte 4 (`resp_len`). | H03 parser `CommunicationTask.cpp:273` implements **`0xAA|ID|LEN|PAYLOAD|CRC|0x55`** with `LEN` at byte 2, validated `>64`. | **DISCREPANCY** — H03 **adds `LEN` field** not in `ProtocolConfig`/`H02` text, but **matches `ResponseManager`'s need for variable payload** and `ProtocolConfig` `MAX 64`. Not wrong, but must be documented as H03 design choice and confirmed with Jetson (Jetson must send `LEN`). |
| **D2 CRC coverage** | `ResponseManager.cpp:24` `crc16(frame, 5+len)` covers `header+ID+success+error+len+payload` (5+len). `H02 §4` says `header→payload` but not precise. `ProtocolConfig` doesn't define coverage. | H03 `CommunicationTask.cpp:318` `crc16(frame, 3+len)` over `header+ID+LEN+payload` (3+len). For command frame this is correct, but differs from `ResponseManager` response frame which includes `success/error`. | **DISCREPANCY** — coverage is consistent **within** command frames, but Jetson must implement **different** CRC for command vs response. Must document. |
| **D3 CRC byte order** | `ResponseManager.cpp:25` `crc &0xFF` then `>>8` = **LE** | H03 `CommunicationTask.cpp:378` same LE, `parser_crc_l_ | crc_h<<8` | **PASS** — matches, correctly documented. |
| **D4 New ID 0x90** | `ProtocolConfig.h:20` defines `0x100/0x200/0x300` but `ResponseManager` uses `0x80/0x81/0x82/0xFE/0xFF` — no `0x90`. `H02` says no new ID invented. | H03 `CommunicationTask.cpp:375` `frame[1]=0x90` Jetson status | **DISCREPANCY** — H03 **invents `0x90`** not in `ProtocolConfig`/`Command.h`, not `H02` confirmed. Must be `PROPOSED`. |
| **D5 Payload 64** | `ProtocolConfig.h:17` `64` | H03 validates `>64` reset, buffer `64` | **PASS** — matches. |

---

## 5. Files Affected

- **Changed in H03:** `services/FanController.h:23` `services/FanController.cpp:54` `rtos/CommunicationTask.h:10` `rtos/CommunicationTask.cpp:1` `application/SystemManager.cpp:169`
- **Not changed (correct):** `PinConfig.h:13` `17/18` `ProtocolConfig.h:15` `SystemConfig.h:22` `model/Command.h:7` `drivers/UartDriver.*:10` `application/CommandManager.*` `application/ResponseManager.*` (except used)
- **New status ID:** `0x90` only in `CommunicationTask.cpp:375` — not in `ProtocolConfig`/`ResponseManager`.

---

## 6. Exact Code Locations

- **FAN 51*n:** `FanController.cpp:63` `level*51` `FanController.h:23` comment `PROPOSED` — `H02 §6` says `REQUIRES CONFIRMATION`.
- **TEMP clamp:** `ClimateController.h:31` `30.0f` `FanController.cpp:63` `level>5 clamp` `CommunicationTask.cpp:132` `32 clamp` `SystemManager.cpp:169` single-byte `setTemperature(payload[0])` → `ClimateController.cpp:34` `clamp 16-30` → `31/32` blocked.
- **AIR MODE:** `CommunicationTask.cpp:185` `8→0,9→2`, `CommunicationTask.cpp:233` `10→false`, `ClimateController.h:11` enum `VENT0..FLOOR_DEFROST5`.
- **FRAME LEN:** `CommunicationTask.cpp:283` `WAIT_LEN >64`, `CommunicationTask.cpp:318` `frame[3+len]` CRC, `ProtocolConfig.h:15` no LEN.
- **STATUS 0x90:** `CommunicationTask.cpp:372` `jet_payload[6]` `375` `0x90`, `CommunicationTask.cpp:354` placeholders `0`.
- **RX:** `CommunicationTask.cpp:263` state machine 7 states, `CommunicationTask.cpp:324` CRC fail `sendError 3`.
- **TX:** `CommunicationTask.cpp:334` `sendPeriodicUpdates` `1000/500/1000` + `ResponseManager` calls + `0x90`.

---

## 7. Recommended Fixes (No Edit in H03.1)

1. **FAN:** Keep `setLevel` but change `H03` report to state `51*n` is **PROPOSED** and add `TODO: H06 calibrate` comment in `FanController.cpp:63`; offer `level*51` as default but make table configurable (e.g., `constexpr uint8_t LEVEL_PWM[6]={0,51,102,153,204,255}`) and mark `REQUIRES CONFIRMATION`.

2. **TEMP:** Either widen `ClimateController.h:31` `temp_max 30→32` (if team confirms `24-32` requirement) or document that `31-32` will be **clamped to 30** and Jetson must limit to `24-30` until confirmed — add `H03` note.

3. **AIR MODE:** Keep `8→0,9→2` but add `REQUIRES CONFIRMATION` comment at `CommunicationTask.cpp:185` and ensure `10` is explicitly rejected with `INVALID` (already).

4. **FRAME LEN:** Document in `H03` that `LEN` at byte 2 is **H03 design choice** to satisfy `ProtocolConfigh:17` `MAX 64`, and update `H02`/`ProtocolConfig` comment to include `LEN` or define frame as `0xAA ID LEN PAYLOAD CRC FOOTER`.

5. **STATUS 0x90:** Either remove `0x90` send and reuse `0x80/0x81/0x82` (less risk) or keep `0x90` but **do not send placeholders as 0** — instead send `engine`/`door` as `0xFF` invalid marker, or omit `0x90` entirely until Jetson confirms 6-field binary layout. Update `CommunicationTask.cpp:354` to use `0xFF` for unavailable or skip `0x90` and only send `0x80/0x81` which have real data. Add `TODO` for `wind_level` inverse map via `FanController::getLevel()` and `last_mode` via `ClimateController::getAirMode()` — add getters to `SystemManager` (P1).

6. **STATUS placeholders:** Change `jet_payload[0]=0` etc. to not be sent if `SOURCE NOT AVAILABLE` — either don't send `0x90` at all, or send with `valid` mask. Recommend **not sending `0x90` in H03.1** until contract signed.

7. **AC source:** `CommunicationTask.cpp:362` `error==NONE?1:0` is wrong for AC — change to `ClimateController` AC via `SystemManager` getter (needs new `SystemManager::getAC()`).

---

## Final Status

**H03.1 REVIEW BLOCKED**

**Reason:** 4 BLOCKERs (B1 fan table, B2 temp 31-32 clamp vs 24-32 req, B3 air mode enum, B4 temp delta payload, B5 0x90 contract, B6 frequency) + 1 FAIL (0x90 placeholder) + 2 DISCREPANCIES (D1 LEN, D4 0x90 ID) prevent considering H03 implementation complete. All are **protocol/behavior**, not `PinConfig`/`hysteresis`/`PPR`.

**H03.1 PASS items:** UART 17/18 115200, binary header/footer/CRC LE/payload 64, AC, FAN save/restore structure, FACE/FOOT reject 10, RX state machine, CRC/overflow/footer handling, dispatch via `CommandManager→SystemManager`, ACK/ERROR, RTOS 50ms, no GPIO conflict.

**Next:** Team confirms §21 questions 1-12 and §7 fan/temp/mode mappings, then fix `FanController.cpp:63` table, `ClimateController.h:31` max, `CommunicationTask.cpp:185` mapping, and `CommunicationTask.cpp:372` 0x90 contract (or remove placeholders) before `H03.2` implementation and `T21` rebuild.

*No code, build, flash, or hardware test in H03.1.*

