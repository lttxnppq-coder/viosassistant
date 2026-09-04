# BUILD 04.4 — PROTOCOL FREEZE AUDIT

**Date:** 2026-09-04
**Scope:** `config/ProtocolConfig.h`, `model/Command.h`, `application/CommandManager.cpp`, `application/ResponseManager.cpp`, `application/SystemManager.cpp`, `rtos/CommunicationTask.h`, `rtos/CommunicationTask.cpp`, `test/system_integration_test/system_integration_test.ino`
**Mode:** AUDIT ONLY — không sửa code, không đổi ID/CRC/frame/range.

> BUILD 04.3 đã dọn sạch demo/raw ID, chỉ còn `0x01 SET_TEMPERATURE`, `0x02 SET_FAN_SPEED`, `0x03 SET_AIR_MODE`, `0x05 SET_AC` trong production `rtos/CommunicationTask.cpp:92`.

---

## 1. CURRENT RX FRAME — `rtos/CommunicationTask.h:29` / `rtos/CommunicationTask.cpp:183`

**Definition in code:**

```cpp
// rtos/CommunicationTask.h:29
// Binary frame parser for Jetson (H03) — 0xAA | ID | LEN | PAYLOAD | CRC_L | CRC_H | 0x55
enum class ParserState { WAIT_HEADER, WAIT_ID, WAIT_LEN, WAIT_PAYLOAD, WAIT_CRC_L, WAIT_CRC_H, WAIT_FOOTER };
static uint8_t parser_id_;          // rtos/CommunicationTask.cpp:18
static uint8_t parser_len_;         // rtos/CommunicationTask.cpp:19  (0..64)
static uint8_t parser_payload_[64]; // rtos/CommunicationTask.cpp:20
static uint8_t parser_crc_l_;       // rtos/CommunicationTask.cpp:22
static uint8_t parser_crc_h_;       // rtos/CommunicationTask.cpp:23
```

**Byte layout (Jetson → ESP32):**

| Byte | Field | Value / Range | Source |
|------|-------|---------------|--------|
| 0 | HEADER | `0xAA` fixed | `rtos/CommunicationTask.cpp:280` `if(byte==0xAA)` `config/ProtocolConfig.h:15` `CMD_HEADER 0xAA` |
| 1 | ID / COMMAND | `0x01`, `0x02`, `0x03`, `0x05` only (after BUILD 04.3) | `rtos/CommunicationTask.cpp:93,102,128,142` `case (uint8_t)model::CommandType::SET_*` |
| 2 | LEN | `0..64` (`CMD_MAX_PAYLOAD 64`) | `rtos/CommunicationTask.cpp:289` `if(byte>64) reset` `config/ProtocolConfig.h:17` |
| 3..3+LEN-1 | PAYLOAD | `LEN` bytes | `rtos/CommunicationTask.cpp:302` `WAIT_PAYLOAD` |
| 3+LEN | CRC_L | Low byte LE | `rtos/CommunicationTask.cpp:308` `WAIT_CRC_L` |
| 3+LEN+1 | CRC_H | High byte LE | `rtos/CommunicationTask.cpp:312` `WAIT_CRC_H` |
| 3+LEN+2 | FOOTER | `0x55` fixed | `rtos/CommunicationTask.cpp:316` `if(byte==0x55)` `config/ProtocolConfig.h:16` `CMD_FOOTER 0x55` |

**LEN position:** Byte 2 (ngay sau ID) — `rtos/CommunicationTask.h:29` comment và `rtos/CommunicationTask.cpp:293` `parser_len_ = byte`.

**CRC coverage:** `rtos/CommunicationTask.cpp:318-324`:
```cpp
frame[0]=0xAA; frame[1]=parser_id_; frame[2]=parser_len_; for(i) frame[3+i]=payload[i];
calc = crc16(frame, 3 + parser_len_); // AA + ID + LEN + PAYLOAD
recv = parser_crc_l_ | (parser_crc_h_ << 8);
if(calc==recv) handleFrame(...); else sendError(parser_id_,3);
```
CRC tính trên `HEADER+ID+LEN+PAYLOAD` (không bao gồm CRC và FOOTER).

**CRC spec:** `utils/Crc16.cpp:35` `POLY 0x8005` `INIT 0xFFFF` `REF_IN true REF_OUT true` (Modbus reflected), `config/ProtocolConfig.h:24-25` `CRC16_POLY 0x8005` `CRC16_INIT 0xFFFF` (type `uint8_t` bug nhưng `utils/Crc16.h:9` đúng), endian **LE** `frame[CRC_L]=crc&0xFF` `frame[CRC_H]=crc>>8`.

**Payload maximum:** `64` `config/ProtocolConfig.h:17` `CMD_MAX_PAYLOAD`, enforce `rtos/CommunicationTask.cpp:289` `>64 → reset to WAIT_HEADER`.

**Behavior khi LEN invalid:** `rtos/CommunicationTask.cpp:289-291` nếu `LEN>64` → `parser_state_=WAIT_HEADER` (discard, không `sendError`).

**Behavior khi CRC sai:** `rtos/CommunicationTask.cpp:328` `if(calc!=recv) if(resp_mgr_) resp_mgr_->sendError(parser_id_,3)` — error code `3 = CRC error` (không phải `1 INVALID`).

**Behavior khi footer sai:** `rtos/CommunicationTask.cpp:332` `if(byte!=0x55)` thì **không** gọi `handleFrame`, chỉ `parser_state_=WAIT_HEADER` — silent discard, không `sendError`.

**Timeout:** `config/ProtocolConfig.h:18` `CMD_TIMEOUT_MS 1000` defined nhưng **không sử dụng** trong `rtos/CommunicationTask.cpp` (không có timer cho incomplete frame).

---

## 2. CURRENT TX FRAMES — `application/ResponseManager.cpp:13`

Tất cả TX qua `drivers/UartDriver.cpp:20` `Serial1.write(frame,len)` `115200 8N1` `PinConfig.h:13` `TX17 RX18`.

### 2.1 `sendResponse()` — `application/ResponseManager.cpp:13`

| Byte | Field | Value |
|------|-------|-------|
| 0 | HEADER | `0xAA` `ResponseManager.cpp:16` |
| 1 | ID | `response.cmd_type` `ResponseManager.cpp:17` (echo `0x01/0x02/0x03/0x05` của request) |
| 2 | SUCCESS | `0x01` success / `0x00` fail `ResponseManager.cpp:18` |
| 3 | ERROR_CODE | `0 SUCCESS,1 INVALID,2 NOT_IMPLEMENTED,3 CRC` `ResponseManager.cpp:19` `CommandManager.cpp:38` |
| 4 | LEN | `response_len` `0..8` `ResponseManager.cpp:20` |
| 5..5+LEN-1 | PAYLOAD | `response_data[0..LEN-1]` `ResponseManager.cpp:21` |
| 5+LEN | CRC_L | LE `ResponseManager.cpp:25` `crc&0xFF` |
| 5+LEN+1 | CRC_H | LE `ResponseManager.cpp:26` `crc>>8` |
| 5+LEN+2 | FOOTER | `0x55` `ResponseManager.cpp:27` |

**CRC coverage:** `ResponseManager.cpp:24` `crc16(frame,5+LEN)` = `HEADER+ID+SUCCESS+ERROR+LEN+PAYLOAD` (khác RX `HEADER+ID+LEN+PAYLOAD`).

**LEN position:** Byte 4 (khác RX byte 2).

### 2.2 `sendError()` — `application/ResponseManager.cpp:122`

| Byte | Field |
|------|-------|
| 0 | `0xAA` `ResponseManager.cpp:125` |
| 1 | `0xFE` fixed `ResponseManager.cpp:126` |
| 2 | `cmd_id` (ID gốc của request) `ResponseManager.cpp:127` |
| 3 | `error_code` `ResponseManager.cpp:128` |
| 4 | `0x00` LEN `ResponseManager.cpp:129` |
| 5 | CRC_L `ResponseManager.cpp:131` `crc&0xFF` |
| 6 | CRC_H `ResponseManager.cpp:132` `crc>>8` |
| 7 | `0x55` `ResponseManager.cpp:133` |

CRC `ResponseManager.cpp:130` `crc16(frame,5)` = `AA FE cmd_id error 00`.

### 2.3 `sendStatus()` — `application/ResponseManager.cpp:31` ID `0x80`

| Byte | Field | Source |
|------|-------|--------|
| 0 | HEADER `0xAA` | `ResponseManager.cpp:34` |
| 1 | ID `0x80` | `ResponseManager.cpp:35` |
| 2 | MODE `SystemMode` | `ResponseManager.cpp:36` |
| 3 | ERROR `ErrorCode` | `ResponseManager.cpp:37` |
| 4-7 | UPTIME `uint32 LE` | `ResponseManager.cpp:38-41` |
| 8-11 | FREE_HEAP `uint32 LE` | `ResponseManager.cpp:42-45` |
| 12-13 | CPU_USAGE `uint16 LE` | `ResponseManager.cpp:46-47` |
| 14 | WATCHDOG `0/1` | `ResponseManager.cpp:48` |
| 15 | RETRY `uint8` | `ResponseManager.cpp:49` |
| 16-17 | CRC LE | `ResponseManager.cpp:51` `crc&0xFF` `crc>>8` |
| 18 | FOOTER `0x55` | `ResponseManager.cpp:53` |

**LEN:** **Không có** LEN field — length implicit 14 payload + 2 CRC. **Khác RX** (RX có LEN).

**CRC coverage:** `ResponseManager.cpp:50` `crc16(frame,16)` = `AA 80 + 14B payload` (không gồm CRC/FOOTER).

### 2.4 `sendTemperatureData()` — `application/ResponseManager.cpp:57` ID `0x81`

| Byte | Field |
|------|-------|
| 0 | `0xAA` `57:60` |
| 1 | `0x81` `57:61` |
| 2-3 | INSIDE `int16 LE` `*10` `57:62` |
| 4-5 | OUTSIDE `int16 LE` `57:64` |
| 6-7 | EVAP `int16 LE` `57:66` |
| 8-9 | AMBIENT `int16 LE` `57:68` |
| 10-11 | SETPOINT `int16 LE` `57:70` |
| 12 | INSIDE_VALID `0/1` `57:72` |
| 13 | OUTSIDE_VALID `0/1` `57:73` |
| 14 | EVAP_VALID `0/1` `57:74` |
| 15 | AMBIENT_VALID `0/1` `57:75` |
| 16-17 | CRC LE `57:76` `crc16(frame,16)` |
| 18 | `0x55` `57:79` |

**LEN:** Không có.

### 2.5 `sendVehicleData()` — `application/ResponseManager.cpp:83` ID `0x82`

| Byte | Field |
|------|-------|
| 0 | `0xAA` `83:86` |
| 1 | `0x82` `83:87` |
| 2-3 | SPEED `int16 LE` `*10` `83:88` |
| 4-5 | RPM `int16 LE` `/10` `83:90` |
| 6-7 | COOLANT `int16 LE` `*10` `83:92` |
| 8-9 | BATTERY `int16 LE` `*100` `83:94` |
| 10 | AC `0/1` `83:96` |
| 11 | BLOWER `0/1` `83:97` |
| 12 | GEAR `uint8` `83:98` |
| 13 | VALID `0/1` `83:99` |
| 14-15 | CRC LE `83:100` `crc16(frame,14)` |
| 16 | `0x55` `83:103` |

### 2.6 `sendAck()` — `application/ResponseManager.cpp:107` ID `0xFF` (không dùng trong `handleFrame`, chỉ `ResponseManager` internal)

`AA FF cmd_id success 00 CRC 55` `ResponseManager.cpp:110-118` `crc16(frame,5)`.

### 2.7 Periodic `0x90` — `rtos/CommunicationTask.cpp:292` (PROPOSED, UNRESOLVED)

| Byte | Field | Value | Status |
|------|-------|-------|--------|
| 0 | HEADER | `0xAA` `CommunicationTask.cpp:294` | PROPOSED |
| 1 | ID | `0x90` `CommunicationTask.cpp:295` | **Không có trong `Command.h`/`ProtocolConfig.h`** |
| 2 | LEN | `6` `CommunicationTask.cpp:296` | PROPOSED |
| 3 | ENGINE | `mode!=OFF?1:0` `CommunicationTask.cpp:276` | PLACEHOLDER |
| 4 | TEMP | `inside_valid?inside:setpoint` `CommunicationTask.cpp:278` | REAL (nếu valid) |
| 5 | AC | `error==NONE?1:0` placeholder `CommunicationTask.cpp:282` | **FAKE** (dùng error proxy) |
| 6 | WIND | `0` `CommunicationTask.cpp:289` | PLACEHOLDER (TBD) |
| 7 | LAST_MODE | `0` `CommunicationTask.cpp:290` | PLACEHOLDER |
| 8 | DOOR | `0` `CommunicationTask.cpp:291` | PLACEHOLDER |
| 9-10 | CRC LE | `crc16(frame,9)` `CommunicationTask.cpp:298` `AA 90 06 +6B` | PROPOSED |
| 11 | FOOTER | `0x55` `CommunicationTask.cpp:301` | PROPOSED |

**CRC coverage:** `CommunicationTask.cpp:298` `crc16(frame,9)` = `AA 90 06 +6B`.

**Kết luận TX:** `sendResponse`/`sendError` có LEN tại byte 4, còn `sendStatus`/`sendTemperature`/`sendVehicle` **không có LEN** — **khác RX** (RX LEN byte 2). `0x90` thì LEN byte 2 giống RX nhưng là frame riêng.

---

## 3. COMMAND TABLE — Production contract sau BUILD 04.3

| Command ID | CommandType `model/Command.h:8` | Payload (RX) | Range / Rule | Current implementation `rtos/CommunicationTask.cpp:92` | Status |
|------------|----------------------------------|--------------|--------------|----------------------------------------------------------|--------|
| `0x01` | `SET_TEMPERATURE` | `LEN 1: [temp]` single-byte absolute | `23 ≤ temp ≤ 30` reject `mapped=false` `CommunicationTask.cpp:103` | `if(payload[0]<23||>30) mapped=false else payload[0],0 len2` | **FROZEN 23-30 PASS** |
| `0x01` | `SET_TEMPERATURE` | `LEN 2: [temp][frac]` `temp+frac*0.1` | `23.0 ≤ t ≤30.0` reject `CommunicationTask.cpp:113` | `float t=payload[0]+payload[1]*0.1; if(t<23||t>30) mapped=false` | **FROZEN PASS** |
| `0x01` | `SET_TEMPERATURE` | `LEN 0` | — | `mapped=false` `CommunicationTask.cpp:124` | PASS (reject) |
| `0x02` | `SET_FAN_SPEED` | `LEN 0: []` | FAN ON restore `FanController::fanOn()` | `CommunicationTask.cpp:128` `len==0 → payload_len 0` `SystemManager.cpp:199` `fanOn()` | **PASS** |
| `0x02` | `SET_FAN_SPEED` | `LEN 1: [v]` `v=0` | OFF `fanOff()` | `CommunicationTask.cpp:133` `v=0 → payload 0` `SystemManager.cpp:204` `fanOff()` | PASS |
| `0x02` | `SET_FAN_SPEED` | `LEN 1: [v] 1-5` | Level 1-5 `setLevel(v)` | `CommunicationTask.cpp:133` `v<=5` `SystemManager.cpp:208` `setLevel` `FanController.cpp:63` `level*51` (calibration TBD) | PASS |
| `0x02` | `SET_FAN_SPEED` | `LEN 1: [v] 6-255` | Raw PWM `setSpeed(v)` | `CommunicationTask.cpp:133` `else raw` `SystemManager.cpp:211` `setSpeed` | PASS (kept, hardware TBD) |
| `0x03` | `SET_AIR_MODE` | `LEN 1: [mode]` `8/9/10` or `0/2/4` | `8→0 VENT FACE`, `9→2 FLOOR FOOT`, `10→4 DEFROST` `CommunicationTask.cpp:146` `if(mode==8) mode=0` etc, reject `mode!=0,2,4` `CommunicationTask.cpp:150` | PASS |
| `0x05` | `SET_AC` | `LEN 1: [v]` | `0=OFF 1=ON` `payload[0]?1:0` | `CommunicationTask.cpp:93` `SET_AC 0x05` `SystemManager.cpp:225` `setAC(payload!=0)` `RelayDriver GPIO4` | PASS |
| `0x04` | `SET_RECIRCULATION` | — | Defined `Command.h:11` but **not handled** in `CommunicationTask` | `default → mapped=false` `CommunicationTask.cpp:162` `sendError 1` | **NOT EXPOSED** (CommandManager would mark NOT_IMPLEMENTED `2` but never reached) |
| `0x06` | `SET_HEATER` | — | Defined but not in `CommunicationTask` | `default` reject | NOT EXPOSED |
| `0x07` | `SET_DAMPER_POS` | — | `NOT_IMPLEMENTED` `CommandManager.cpp:49` | `default` reject | NOT EXPOSED |
| `0x10/0x11/0xF0/0xF1` | `REQUEST_*` | — | Not in `CommunicationTask` | `default` reject | NOT EXPOSED |

**Temperature rule:** `23-30` **reject** tại boundary `CommunicationTask.cpp:105,116`, không để `ClimateController.cpp:34` `clamp 16-30` tự clamp 40→30 rồi coi valid — đúng spec BUILD 04.3 `3`.

**Fan rule:** Chỉ `0x02`, đã xóa `101..105` `CommunicationTask.cpp:241` cũ, dead `else` `CommunicationTask.cpp:179` cũ đã sửa `CommunicationTask.cpp:133` `if(len==0) else { // len>=1`.

**Air rule:** Chỉ `0x03` với payload `8/9/10`, đã xóa `id==8` raw-ID `CommunicationTask.cpp:232` cũ.

---

## 4. ACK / ERROR TABLE

| Condition | ESP32 → Jetson frame | ID | Payload | Source |
|-----------|----------------------|----|---------|--------|
| **Success** `resp.success==true` | `sendResponse` `AA cmd_type success error len payload CRC 55` | `response.cmd_type` `ResponseManager.cpp:17` echo `0x01/0x02/0x03/0x05` | `success=0x01` `error=0` `len 0..8` `ResponseManager.cpp:18-20` | `rtos/CommunicationTask.cpp:177` `if(resp.success) sendResponse(resp)` |
| **INVALID ID** `mapped==false` (unknown ID) | `sendError` `AA FE cmd_id 01 00 CRC 55` | `0xFE` `ResponseManager.cpp:126` | `error=1` INVALID `rtos/CommunicationTask.cpp:168` `sendError(id,1)` | `rtos/CommunicationTask.cpp:168` |
| **INVALID payload** (temp <23||>30, air mode !=0/2/4) | `sendError` `AA FE cmd_id 01 00 CRC 55` | `0xFE` | `1` | `rtos/CommunicationTask.cpp:168` |
| **NOT_IMPLEMENTED** (`SET_DAMPER_POS` etc via `CommandManager`) | `sendError` `AA FE cmd_id 02 00 CRC 55` | `0xFE` `2` NOT_IMPLEMENTED `CommandManager.cpp:54` | `rtos/CommunicationTask.cpp:178` `sendError(id,resp.error_code)` | `rtos/CommunicationTask.cpp:178` |
| **CRC error** | `sendError` `AA FE parser_id 03 00 CRC 55` | `0xFE` `3` CRC | `rtos/CommunicationTask.cpp:329` `sendError(parser_id_,3)` | `rtos/CommunicationTask.cpp:329` |
| **Success ACK via `sendAck` `0xFF`** | `AA FF cmd_id success 00 CRC 55` `ResponseManager.cpp:107` | `0xFF` | **Không dùng** trong `handleFrame` (chỉ `sendResponse`) | `ResponseManager.cpp:107` dead |

**Chưa có spec cho ACK ID chính thức:** `sendResponse` dùng `cmd_type` làm ID, `sendError` dùng `0xFE`, `sendAck` `0xFF` không dùng — **UNRESOLVED** nếu Jetson expect ACK `0xFF` hay `cmd_type`.

---

## 5. TELEMETRY TABLE

| ID | Name | Period | Frame | Fields | Status |
|----|------|--------|-------|--------|--------|
| `0x80` | `sendStatus` `ResponseManager.cpp:31` | `1000ms` `rtos/CommunicationTask.cpp:349` `if(now-last_status>1000)` | `AA 80 mode error uptime heap cpu watchdog retry CRC 55` `ResponseManager.cpp:34-53` 19B | **REAL** (mode, error, uptime, heap) + **PLACEHOLDER** `cpu_usage 0` `retry 0` |
| `0x81` | `sendTemperatureData` `ResponseManager.cpp:57` | `500ms` `rtos/CommunicationTask.cpp:390` | `AA 81 inside*10 outside*10 evap*10 ambient*10 setpoint*10 valid*4 CRC 55` `ResponseManager.cpp:60-79` 19B | **REAL** `inside/outside/setpoint` + `inside_valid` (đã fix `SystemManager.cpp:141`) + **UNAVAILABLE** `evap/ambient` luôn `0` `SystemManager.cpp:149` comment |
| `0x82` | `sendVehicleData` `ResponseManager.cpp:83` | `1000ms` `rtos/CommunicationTask.cpp:396` | `AA 82 speed*10 rpm/10 coolant*10 battery*100 ac blower gear valid CRC 55` `ResponseManager.cpp:86-103` 17B | **STUB** `services/VehicleDataService.cpp:55` `parseCanFrame false` → `data_valid false` luôn `0` |
| `0x90` | `CommunicationTask.cpp:292` periodic `1000ms` | `1000ms` cùng `sendStatus` `rtos/CommunicationTask.cpp:349` | `AA 90 06 [engine,temp,AC,wind,last_mode,door] CRC 55` `CommunicationTask.cpp:293-301` 12B | **PROPOSED/UNRESOLVED** — 4/6 fields hardcoded `0` `CommunicationTask.cpp:289-291` `wind 0 last_mode 0 door 0`, `AC` fake `error==NONE?1:0` `CommunicationTask.cpp:282` |

**Hardcoded/placeholder:** `0x90` `engine` `PROPOSED` `wind 0` `last_mode 0` `door 0` — `BUILD 04.3` giữ nguyên, không coi official.

---

## 6. CRC SPEC

| Item | Value | Source |
|------|-------|--------|
| Polynomial | `0x8005` | `config/ProtocolConfig.h:24` `CRC16_POLY` (type `uint8_t` bug) + `utils/Crc16.h:9` `POLY 0x8005` correct |
| Init | `0xFFFF` | `config/ProtocolConfig.h:25` `CRC16_INIT` `utils/Crc16.h:10` |
| RefIn/RefOut | `true/true` reflected | `utils/Crc16.cpp:8` table `index=(crc^data)&0xFF` `crc>>8 ^ table` |
| XOR Out | `0x0000` | `utils/Crc16.h:11` |
| Endian | **LE** `CRC_L = crc&0xFF` `CRC_H = crc>>8` | `rtos/CommunicationTask.cpp:324` `recv = crc_l | crc_h<<8` `ResponseManager.cpp:25` `crc&0xFF` `crc>>8` |
| RX coverage | `AA ID LEN PAYLOAD` `3+LEN` bytes | `rtos/CommunicationTask.cpp:324` `crc16(frame,3+LEN)` |
| TX `sendResponse` coverage | `AA cmd_type success error len payload` `5+LEN` | `ResponseManager.cpp:24` `crc16(frame,5+LEN)` — **khác RX** |
| TX `sendStatus` coverage | `AA 80 +14B` `16` bytes | `ResponseManager.cpp:50` `crc16(frame,16)` |
| TX `0x90` coverage | `AA 90 06 +6B` `9` bytes | `rtos/CommunicationTask.cpp:298` `crc16(frame,9)` |

---

## 7. MISMATCH LIST

| # | Area | Current A | Current B | Impact | File |
|---|------|-----------|-----------|--------|------|
| M1 | **LEN position** | RX `LEN@2` `CommunicationTask.h:29` | TX `sendResponse` `LEN@4` `ResponseManager.cpp:20`, TX `0x80/0x81/0x82` **no LEN** `ResponseManager.cpp:35,61,87` | Jetson cần 2 parser | `rtos/CommunicationTask.h:29` vs `application/ResponseManager.cpp:20,35` |
| M2 | **CRC coverage** | RX `3+LEN` `CommunicationTask.cpp:324` | TX `sendResponse` `5+LEN` `ResponseManager.cpp:24` vs `sendStatus` `16` `ResponseManager.cpp:50` vs `0x90` `9` `CommunicationTask.cpp:298` | Không unified | — |
| M3 | **Payload max** | `ProtocolConfig.h:17` `64` enforce `CommunicationTask.cpp:289` `>64 reset` | `ResponseManager` `sendResponse` `response_len 0..8` `ResponseManager.cpp:20` `i<8` — OK nhưng TX `0x80` 14B >8 không qua `sendResponse` | Consistent nhưng TX `0x80` không qua check `64` | — |
| M4 | **ID bases** | `ProtocolConfig.h:20-22` `VEHICLE 0x100 CLIMATE 0x200 SYSTEM 0x300` **không dùng** | Production dùng `0x01/0x02/0x03/0x05/0x80/0x81/0x82/0xFE/0x90` | `ProtocolConfig` dead code | `config/ProtocolConfig.h:20` |
| M5 | **Timeout** | `ProtocolConfig.h:18` `1000ms` defined | `CommunicationTask.cpp` **không dùng** timeout cho incomplete frame | Dead param | `config/ProtocolConfig.h:18` |
| M6 | **CRC poly type** | `ProtocolConfig.h:24` `uint8_t 0x8005` truncates `0x05` | `utils/Crc16.h:9` `uint16_t 0x8005` correct | Bug nếu dùng `ProtocolConfig::CRC16_POLY` | `config/ProtocolConfig.h:24` |
| M7 | **Temperature storage** | `CommunicationTask.cpp:103` store `payload[0],0 len2` single-byte | `SystemManager.cpp:189` `if(payload_len>=2) temp=payload[0]+payload[1]*0.1 else if(len==1) temp=payload[0]` — 2 path | Redundant but consistent | `rtos/CommunicationTask.cpp:103` vs `application/SystemManager.cpp:189` |
| M8 | **Test vs production** | `test/system_integration_test.ino:16` includes 22 `*.cpp` absolute | Production `ViosAssistant.ino:8` separate objects | Test bypasses `find_src.pl` search, but OK for build | `test/...ino:1` |

---

## 8. UNRESOLVED ITEMS

| Item | Current | Why unresolved |
|------|---------|---------------|
| **0x90 telemetry format** | `CommunicationTask.cpp:292` `AA 90 06 [6B] CRC 55` 4 fields `0` + AC fake | `PROPOSED` from H03, không có spec, `H01_5` chưa confirm engine/wind/last_mode/door source |
| **TX frame format freeze** | `sendResponse` LEN@4 vs `sendStatus` no LEN vs RX LEN@2 | Team chưa quyết unified `AA ID LEN PAYLOAD CRC 55` hay keep `ResponseManager` legacy |
| **ACK ID** | `sendResponse` echo `cmd_type` vs `sendAck 0xFF` unused | Chưa spec Jetson expect gì |
| **Temperature 2-byte encoding** | `payload[0]+payload[1]*0.1` `CommunicationTask.cpp:114` | Spec nói single-byte `23-30` FROZEN, nhưng 2-byte vẫn chấp nhận và validate — chưa rõ Jetson sẽ gửi loại nào |
| **Fan raw PWM 6-255** | `CommunicationTask.cpp:133` `else raw` kept | Spec nói Level 1-5 nhưng raw vẫn executable — chưa freeze có cho phép raw không |
| **Air mode `BI_LEVEL/MIX/F/D`** | `model/Command.h` enum có 6 mode nhưng `CommunicationTask.cpp:150` chỉ accept `0,2,4` | Chưa quyết có dùng 3 mode còn lại không |
| **Vehicle data** | `VehicleDataService.cpp:55` stub `data_valid false` | H09 CAN module chưa định nghĩa |
| **Evap/Ambient NTC** | `SystemManager.cpp:149` comment `PIN_NTC_EVAP not defined` | PinConfig chỉ có `1/2`, evap/ambient `0` |
| **CRC poly type bug** | `ProtocolConfig.h:24` `uint8_t` | Chưa sửa để giữ `UNRESOLVED` |

---

## 9. PROPOSED FREEZE CHECKLIST

| ITEM | CURRENT | REQUIRED DECISION | STATUS |
|------|---------|-------------------|--------|
| Frame format | RX `AA ID LEN PAYLOAD CRC LE 55` LEN@2 `CommunicationTask.h:29` | Team confirm `LEN@2` cho cả RX/TX hay giữ TX legacy (`LEN@4`/`no LEN`) | **UNRESOLVED** |
| Temperature | `23-30` single `23-30` và two-byte `23.0-30.0` validate `CommunicationTask.cpp:103,114` FROZEN | Team confirm `23-30` đã chốt | **PASS** (BUILD 04.1/04.3) |
| CRC | `0x8005 / 0xFFFF` LE over `HEADER+ID+LEN+PAYLOAD` (RX) vs `HEADER+ID+success+error+LEN+PAYLOAD` (TX response) `utils/Crc16.h:9` | Team confirm poly/init/endian/coverage unified | **UNRESOLVED** |
| TX response format | `AA cmd_type success error len payload CRC 55` LEN@4 `ResponseManager.cpp:13` vs `AA 0x80/0x81/0x82` no LEN | Team decision: unified `AA ID LEN PAYLOAD CRC 55` cho cả `0x80/0x81/0x82` hay giữ legacy | **UNRESOLVED** |
| 0x90 telemetry | `AA 90 06 [6B] CRC 55` 4 fields `0` `CommunicationTask.cpp:292` | Team decision: official hay bỏ, nếu official thì define từng field source (engine/wind/last_mode/door/AC) | **UNRESOLVED — PROPOSED** |
| Command IDs | `0x01/0x02/0x03/0x05` `model/Command.h:8` `CommunicationTask.cpp:92` | Team confirm 4 IDs đủ, có thêm `0x04/0x06/0x07` không | **PASS** (4 IDs) |
| Air mode | `8→0 9→2 10→4` `CommunicationTask.cpp:146` | Team confirm `8=FACE 9=FOOT 10=DEFROST` payload | **PASS** (FROZEN) |
| Fan level | `1-5` via `0x02` `CommunicationTask.cpp:133` | Team confirm `1-5` vs `101-105` đã xóa | **PASS** |
| ACK/ERROR | `sendResponse` `AA cmd_type ...` và `sendError AA FE cmd_id error 00 CRC 55` `ResponseManager.cpp:122` `CommunicationTask.cpp:168` | Team confirm Jetson sẽ parse `FE`/`cmd_type` | **UNRESOLVED** (0xFF unused) |
| Payload max | `64` `ProtocolConfig.h:17` `CommunicationTask.cpp:289` | Team confirm `64` | **UNRESOLVED** (chưa dùng hết) |
| Timeout | `1000ms` `ProtocolConfig.h:18` unused | Team confirm có cần timeout cho incomplete frame | **UNRESOLVED** |
| Telemetry 0x80/0x81/0x82 | `0x80 status 19B`, `0x81 temp 19B`, `0x82 vehicle 17B` `ResponseManager.cpp:31,57,83` | Team confirm 3 IDs và payload layout | **UNRESOLVED** |

---

**Ghi chú:** BUILD 04.4 là AUDIT ONLY, không sửa code, không đổi ID/CRC/frame/range, không build/upload. Đã tạo `BUILD_04_4_PROTOCOL_FREEZE_AUDIT.md:1` này để chuẩn bị freeze.
