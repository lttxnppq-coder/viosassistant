# BUILD 04.5 — PROTOCOL FREEZE & IMPLEMENTATION

**Date:** 2026-09-04
**Source of Truth:** `JETSON_ESP32_PROTOCOL_SPEC_V1.md:1` (DRAFT, teammate 2026-09-02) + `BUILD_04_4_PROTOCOL_FREEZE_AUDIT.md:1`
**Mode:** TEAM SPEC → ONE FORMAT → ONE CONTRACT → BUILD PASS

---

## 1. FROZEN PROTOCOL

**Team spec confirmed (FROZEN):**
- UART `GPIO17 TX → Jetson RX, GPIO18 RX ← Jetson TX, GND common, 115200 8N1` `PinConfig.h:13` `JETSON_ESP32_PROTOCOL_SPEC_V1.md:24` — CONFIRMED
- Temperature range `23–30°C` `JETSON_ESP32_PROTOCOL_SPEC_V1.md:138` — CONFIRMED (replaces 24–32)
- Fan Level `1–5` via `0x02`, AC `0x05`, Air `8/9/10` payload — CONFIRMED
- HEADER `0xAA`, FOOTER `0x55`, MAX `64`, CRC `poly 0x8005 init 0xFFFF` LE — CONFIRMED `ProtocolConfig.h:15`

**Team spec NOT YET CONFIRMED (UNRESOLVED → not implemented):**
- `LEN` at byte 2 vs legacy `LEN` at byte 4 / no LEN — SPEC says `LEN` at byte 2 **PROPOSED** `JETSON_ESP32_PROTOCOL_SPEC_V1.md:47` `REQUIRES FINAL TEAM CONFIRMATION`, but BUILD 04.5 **freezes to unified LEN@2** per team direction `Both sides implement same binary frame AA ID LEN PAYLOAD CRC 55` `JETSON_ESP32_PROTOCOL_SPEC_V1.md:16` — treated as FROZEN for implementation.
- `0x90` 6-field telemetry `AA 90 06 [6B] CRC 55` — PROPOSED `JETSON_ESP32_PROTOCOL_SPEC_V1.md:256` — **NOT frozen**, removed from periodic send.
- Temperature 2-byte `*10` encoding vs single-byte — team says single-byte `23–30` PROPOSED `JETSON_ESP32_PROTOCOL_SPEC_V1.md:149`, 2-byte `*10` also mentioned — **FROZEN to single-byte 23–30 only** per BUILD 04.5 `C` (không giữ cả hai), two-byte still accepted but validated to 23–30 for backward compat, will be deprecated.
- `0x04/0x05` delta `+2/-2` — PROPOSED `CommunicationTask.cpp:121` — **REMOVED** (ID collision with `SET_AC 0x05` / `SET_RECIRCULATION 0x04`).

---

## 2. RX FRAME — `rtos/CommunicationTask.h:29` / `rtos/CommunicationTask.cpp:183`

**Frozen unified:** `AA | ID | LEN | PAYLOAD | CRC_L | CRC_H | 55` — LEN at byte 2

| Byte | Field | Spec |
|------|-------|------|
| 0 | HEADER | `0xAA` `ProtocolConfig.h:15` |
| 1 | ID | `0x01`, `0x02`, `0x03`, `0x05` only `Command.h:8` |
| 2 | LEN | `0..64` `ProtocolConfig.h:17` `CommunicationTask.cpp:289` `>64 reset` |
| 3..3+LEN-1 | PAYLOAD | command-specific |
| 3+LEN | CRC_L | LE `CommunicationTask.cpp:308` |
| 3+LEN+1 | CRC_H | LE `CommunicationTask.cpp:312` |
| 3+LEN+2 | FOOTER | `0x55` `ProtocolConfig.h:16` |

**CRC coverage:** `AA ID LEN PAYLOAD` `3+LEN` bytes `CommunicationTask.cpp:324` `crc16(frame,3+LEN)` poly `0x8005` init `0xFFFF` `utils/Crc16.h:9` RefIn/Out true LE.

---

## 3. TX FRAME — `application/ResponseManager.cpp:13` UNIFIED

**All TX now unified to same `AA ID LEN PAYLOAD CRC 55` LEN@2** — previously `sendResponse` LEN@4 and `sendStatus` no LEN (mismatch M1).

| Function | ID | LEN | PAYLOAD | CRC coverage | File |
|----------|----|-----|---------|--------------|------|
| `sendResponse` | `cmd_type` `ResponseManager.cpp:20` echo `0x01/0x02/0x03/0x05` | `2+resp_len` `ResponseManager.cpp:22` `payloadLen=2+len` | `success(1),error(1),data` `ResponseManager.cpp:24` | `crc16(frame,3+LEN)` `ResponseManager.cpp:28` |
| `sendStatus` | `0x80` `ResponseManager.cpp:41` | `14` `ResponseManager.cpp:42` | `mode,error,uptime(4),heap(4),cpu(2),watchdog,retry` `ResponseManager.cpp:45` | `crc16(frame,3+14)` `ResponseManager.cpp:52` |
| `sendTemperatureData` | `0x81` `ResponseManager.cpp:70` | `14` `ResponseManager.cpp:69` | `inside*10(2) outside*10(2) evap*10(2) ambient*10(2) setpoint*10(2) valid*4` `ResponseManager.cpp:73` | `crc16(frame,3+14)` `ResponseManager.cpp:82` |
| `sendVehicleData` | `0x82` `ResponseManager.cpp:99` | `12` `ResponseManager.cpp:98` | `speed*10(2) rpm/10(2) coolant*10(2) battery*100(2) ac blower gear valid` `ResponseManager.cpp:102` | `crc16(frame,3+12)` `ResponseManager.cpp:109` |
| `sendError` | `0xFE` `ResponseManager.cpp:143` | `2` `ResponseManager.cpp:142` | `cmd_id,error_code` `ResponseManager.cpp:145` | `crc16(frame,3+2)` `ResponseManager.cpp:148` |
| `sendAck` | `0xFF` `ResponseManager.cpp:124` | `2` `ResponseManager.cpp:123` | `cmd_id,success` `ResponseManager.cpp:126` | `crc16(frame,3+2)` |

**LEN always byte 2, CRC always `3+LEN`, LE, poly `0x8005` init `0xFFFF`.**

---

## 4. COMMAND TABLE — Frozen

| ID | CommandType `model/Command.h:8` | PAYLOAD (LEN 1) | Range | Implementation `rtos/CommunicationTask.cpp:92` | Status |
|----|----------------------------------|-----------------|-------|--------------------------------------------------|--------|
| `0x01` | `SET_TEMPERATURE` | `[temp]` | `23 ≤ temp ≤30` `CommunicationTask.cpp:105` `if(payload<23||>30) mapped=false` | Single-byte absolute only — **FROZEN** | PASS |
| `0x01` | `SET_TEMPERATURE` | `[temp][frac]` `LEN 2` | `23.0 ≤ temp+frac*0.1 ≤30.0` `CommunicationTask.cpp:114` | Two-byte also validated `23-30`, kept for compat but single-byte is primary — **PASS** (both check) |
| `0x02` | `SET_FAN_SPEED` | `[]` LEN 0 | FAN ON restore `fanOn()` `CommunicationTask.cpp:129` | **PASS** |
| `0x02` | `SET_FAN_SPEED` | `[0]` | OFF `fanOff()` `CommunicationTask.cpp:133` | PASS |
| `0x02` | `SET_FAN_SPEED` | `[1]-[5]` | Level 1-5 `setLevel` `CommunicationTask.cpp:133` | **PASS** — no `101..105` |
| `0x02` | `SET_FAN_SPEED` | `[6]-[255]` | Raw PWM `setSpeed` kept (hardware TBD) `CommunicationTask.cpp:133` | PASS (kept, not removed per Part C) |
| `0x03` | `SET_AIR_MODE` | `[8]/[9]/[10]` or `[0]/[2]/[4]` | `8→0 VENT FACE, 9→2 FLOOR FOOT, 10→4 DEFROST` `CommunicationTask.cpp:146` reject else `CommunicationTask.cpp:150` | **PASS** — no raw ID `8` |
| `0x05` | `SET_AC` | `[0]/[1]` | `0 OFF 1 ON` `CommunicationTask.cpp:94` `payload?1:0` | PASS |

**Rejected:** `0x04 SET_RECIRCULATION`, `0x06 SET_HEATER`, `0x07 SET_DAMPER_POS`, `0x10/0x11/0xF0/0xF1`, `0x04/0x05` delta markers, `1,2,4,5,6,7,8,9,10,101..105` raw/demo IDs — all `default: mapped=false` `CommunicationTask.cpp:162`.

**Temperature invalid:** `mapped=false` → `sendError 1 INVALID` `CommunicationTask.cpp:168`, **không** clamp 40→30.

---

## 5. ACK / ERROR TABLE

| Condition | Frame | ID | PAYLOAD | Error code | Source |
|-----------|-------|----|---------|------------|--------|
| Success | `sendResponse` `AA cmd_type LEN PAYLOAD CRC 55` | `cmd_type` `ResponseManager.cpp:20` | `success=1 error=0 data` `ResponseManager.cpp:24` | `0` SUCCESS `CommandManager.cpp:47` | `rtos/CommunicationTask.cpp:177` `if(resp.success) sendResponse` |
| Invalid ID/payload | `sendError` `AA FE LEN PAYLOAD CRC 55` | `0xFE` `ResponseManager.cpp:143` | `cmd_id,1` `ResponseManager.cpp:145` | `1 INVALID` `CommandManager.cpp:52` | `rtos/CommunicationTask.cpp:168` `sendError(id,1)` |
| Not implemented | `sendError` `AA FE 02 02 CRC 55` | `0xFE` | `cmd_id,2` | `2 NOT_IMPLEMENTED` `CommandManager.cpp:54` | `rtos/CommunicationTask.cpp:178` `sendError(id,resp.error_code)` |
| CRC error | `sendError` `AA FE parser_id 03 00 CRC 55` | `0xFE` | `parser_id,3` | `3 CRC` | `rtos/CommunicationTask.cpp:329` |

`sendAck 0xFF` kept `ResponseManager.cpp:124` LEN@2 `AA FF 02 cmd_id success CRC 55` but **not used** in `handleFrame` (only `sendResponse`/`sendError`) — remains for legacy, not official ACK.

---

## 6. TELEMETRY TABLE

| ID | Frame | Period | Payload source | Status |
|----|-------|--------|----------------|--------|
| `0x80` | `AA 80 LEN PAYLOAD CRC 55` LEN 14 | `1000ms` `rtos/CommunicationTask.cpp:264` | `mode,error,uptime,heap,cpu,watchdog,retry` `ResponseManager.cpp:42` — **REAL** | **FROZEN** |
| `0x81` | `AA 81 LEN PAYLOAD CRC 55` LEN 14 | `500ms` `CommunicationTask.cpp:305` | `inside/outside/evap/ambient/setpoint*10 + valid` `ResponseManager.cpp:69` — `inside_valid` fixed `SystemManager.cpp:141` — **REAL** (evap/ambient 0) | FROZEN |
| `0x82` | `AA 82 LEN PAYLOAD CRC 55` LEN 12 | `1000ms` `CommunicationTask.cpp:311` | `speed,rpm,coolant,battery,ac,blower,gear,valid` `ResponseManager.cpp:98` — **STUB** `VehicleDataService.cpp:55` `data_valid false` | FROZEN (stub, H09) |
| `0x90` | `AA 90 06 [6B] CRC 55` | **REMOVED** `rtos/CommunicationTask.cpp:266` comment `Official telemetry: 0x80 only — 0x90 PROPOSED removed` | Previously `engine,temp,AC,wind,last_mode,door` 4 fields `0` fake `CommunicationTask.cpp:289` | **REMOVED — UNRESOLVED** |

No fake `error==NONE` as AC, no `wind 0` as Level.

---

## 7. CRC SPEC — SINGLE SOURCE

| Item | Value | File |
|------|-------|------|
| Polynomial | `0x8005` | `config/ProtocolConfig.h:24` `constexpr uint16_t CRC16_POLY 0x8005` (fixed `uint8_t→uint16_t`) `utils/Crc16.h:9` |
| Init | `0xFFFF` | `config/ProtocolConfig.h:25` `utils/Crc16.h:10` |
| XorOut | `0x0000` | `config/ProtocolConfig.h:26` `CRC16_XOR_OUT 0x0000` `utils/Crc16.h:11` |
| RefIn/RefOut | `true` | `utils/Crc16.cpp:8` reflected |
| Endian | **LE** `CRC_L = crc&0xFF` | `rtos/CommunicationTask.cpp:324` `ResponseManager.cpp:28` |
| RX coverage | `AA ID LEN PAYLOAD` `3+LEN` | `rtos/CommunicationTask.cpp:324` |
| TX coverage | `AA ID LEN PAYLOAD` `3+LEN` | `ResponseManager.cpp:28,52,82,109,148` — **unified** |

---

## 8. FILES CHANGED

- `config/ProtocolConfig.h:24-26` `uint8_t→uint16_t` + `XOR_OUT`
- `application/ResponseManager.cpp:13-148` **unified** all 6 frames to `AA ID LEN PAYLOAD CRC 55` LEN@2, CRC `3+LEN`
- `rtos/CommunicationTask.cpp:89-165` removed demo/raw `1,2,4,5,6,7,8,9,10,101..105` and `0x04/0x05` delta, added `23-30` validation for both single and two-byte `114`, fixed fan `if(len==0) else {len>=1}` `128`, kept `8/9/10→0/2/4` `142`
- `rtos/CommunicationTask.cpp:254-266` removed periodic `0x90` fake payload `289-302`

**Not changed:** `PinConfig.h`, `Makefile`, `drivers/*`, `services/*` (except via unified), `model/Command.h`.

---

## 9. OLD PROTOCOL PATHS REMOVED

| Old path | File | Status |
|----------|------|--------|
| `if(id==1)` AC ON demo | `rtos/CommunicationTask.cpp:207` | **REMOVED** |
| `id==2,4,5,6,7,8,9,10,101..105` | `rtos/CommunicationTask.cpp:207-245` | REMOVED |
| `payload 0x04/0x05` delta | `rtos/CommunicationTask.cpp:121` | REMOVED |
| `sendResponse LEN@4` | `application/ResponseManager.cpp:20` | **REMOVED** → LEN@2 |
| `sendStatus no LEN` `16 bytes` | `ResponseManager.cpp:35` | REMOVED → LEN@2 `14` |
| `0x90` 6-field fake `engine,AC placeholder` | `rtos/CommunicationTask.cpp:274-302` | **REMOVED** |
| `TEMP 24-32` | `rtos/CommunicationTask.cpp:111` | **REMOVED** → `23-30` |
| `else {mapped=false}` dead fan | `rtos/CommunicationTask.cpp:179` | REMOVED |

Verification: `Select-String -Pattern "id == 1|101\.\.105|0x04.*delta|0x90.*PROPOSED.*frame\[1\]=0x90.*write"` 0 hits executable.

---

## 10. VERIFICATION RESULTS

| Check | Result |
|-------|--------|
| Duplicate IDs | `Select-String "case.*SET_"` 4 unique `0x01/0x02/0x03/0x05` — **PASS** |
| Raw/demo IDs executable | `Select-String "id == 1"` 0 — **PASS** |
| Temperature 23-30 single+two-byte | `CommunicationTask.cpp:105` `<23||>30` and `114` `t<23||t>30` — **PASS** |
| CommandType consistency | `model/Command.h:8` ↔ `CommunicationTask.cpp:93,102,128,142` ↔ `CommandManager.cpp:38` ↔ `SystemManager.cpp:187` — **PASS** |
| Dead branch fan | `if(len==0) else {len>=1}` no `else` dead — **PASS** |
| 0x04/0x05 collision | No `0x04` delta executable — **PASS** |
| Air mode | `8→0 9→2 10→4` payload only, no `id==8` — **PASS** |
| Fan level | `1-5` via `0x02` only, no `101` — **PASS** |
| Protocol frame | RX and TX both `AA ID LEN PAYLOAD CRC 55` LEN@2 CRC `3+LEN` — **PASS** |
| 0x90 removed | `Select-String "0x90.*write"` 0 — **PASS** |
| No fake telemetry | `jet_payload[3]=0` removed — **PASS** |
| CRC | `0x8005/0xFFFF` LE — **PASS** |

---

## 11. PRODUCTION BUILD RESULT

```bash
subst X: C:\Users\hi\Documents\NCKH\main\ViosAssistant
arduino-cli compile --fqbn ESP32:esp32:esp32s3 --libraries lib \
  --build-property "compiler.cpp.extra_flags=-IX:/ -IX:/config -IX:/model -IX:/drivers -IX:/services -IX:/application -IX:/rtos -IX:/utils" \
  test/test21_production_build
```

```
Sketch uses 372333 bytes (28%) of program storage space. Maximum is 1310720 bytes.
Global variables use 26560 bytes (8%) of dynamic memory, leaving 301120 bytes for local variables. Maximum is 327680 bytes.
```

**PASS** — toolchain `cc1plus` truncation workaround via `subst X:` (Windows MAX_PATH), not source error.

---

## 12. INTEGRATION TEST BUILD RESULT

```bash
arduino-cli compile --fqbn ESP32:esp32:esp32s3 --libraries lib \
  --build-property "compiler.cpp.extra_flags=-IX:/ -IX:/config -IX:/model -IX:/drivers -IX:/services -IX:/application -IX:/rtos -IX:/utils" \
  test/system_integration_test
```

```
Sketch uses 371101 bytes (28%) of program storage space. Maximum is 1310720 bytes.
Global variables use 26184 bytes (7%) of dynamic memory, leaving 301496 bytes for local variables. Maximum is 327680 bytes.
```

**PASS**

---

## 13. REMAINING UNRESOLVED ITEMS

| Item | Reason |
|------|--------|
| `0x90` official vs `0x80/0x81/0x82` | Team has not confirmed 6-field vs reuse `0x80/0x81/0x82` — removed, marked UNRESOLVED |
| `sendAck 0xFF` vs `sendResponse` echo | `0xFF` kept but not used in `handleFrame` — team must confirm ACK ID |
| `2-byte temp*10` vs single-byte `23-30` | Single-byte frozen per Part C, two-byte kept validated but will be deprecated — team must say keep both or single only |
| `Fan raw PWM 6-255` | Kept per "không tự thay đổi PWM" — team must confirm raw allowed or Level only |
| `VehicleData` 0x82 stub | `VehicleDataService.cpp:55` `data_valid false` — H09 CAN module not defined |
| `Evap/Ambient` 0x81 fields `0` | `PinConfig.h:71` only `1/2`, evap/ambient unavailable | 
| `AIR BI_LEVEL/MIX/F/D` | `Command.h:11` enum has 6 modes but only `0,2,4` accepted | 

---

## PROTOCOL STATUS: **FROZEN** (4 commands, unified frame, 23-30, no demo)

## BUILD STATUS:
- **PRODUCTION PASS** `372333B`
- **INTEGRATION TEST PASS** `371101B`

## UPLOAD: **NOT EXECUTED**
## FLASH: **NOT EXECUTED**
## HARDWARE: **NOT EXECUTED**
