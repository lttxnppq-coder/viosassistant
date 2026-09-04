# H01.5 Jetson <-> ESP32 Binary Protocol Audit

**Date:** 2026-09-02  
**Mode:** READ-ONLY AUDIT — NO production code/config change, NO build, NO flash  
**Jetson source:** Demo `Serial` ASCII proposal (emulator, not spec)  
**ViosAssistant baseline:** `PinConfig.h:13` `drivers/UartDriver.*:1` `application/{CommandManager,SystemManager,ResponseManager}.*:1` `config/{ProtocolConfig,SystemConfig}.h:1` `model/Command.h:1`  
**Architecture direction (CONFIRMED per user 2026-09-02):** Jetson via `Serial1/UART1` `17/18` `115200` `SERIAL_8N1`, `Serial` for debug, binary `0xAA ... CRC16 ... 0x55`, `ProtocolConfig.h` baseline — ASCII demo NOT for production.

---

## 1. Executive Summary

Jetson demo is ASCII line protocol (`Serial 9600`, `code\r` -> `toInt()`, 6x `println`). ViosAssistant production is binary framed (`Serial1 115200` `0xAA` `CRC16` `0x55`). Architecture direction is now CONFIRMED to keep ViosAssistant binary baseline. Command semantics (11 Jetson codes) and status fields (6 `println` lines) have no confirmed binary mapping yet. RX parsing and TX periodic status are stub/discard. Result is **implementation blocker** — no end-to-end Jetson communication can run until mapping and RX/TX stubs are implemented.

**PROTOCOL NOT READY** — architecture chosen, but command mapping + status contract + RX/TX implementation incomplete.

---

## 2. Confirmed Architecture

| Item | Confirmed Value | Source |
|---|---|---|
| Jetson UART | `Serial1` / `UART1` | `PinConfig.h:13` `PIN_PI_UART_TX 17` `PIN_PI_UART_RX 18` + user decision |
| TX | GPIO17 ESP32 TX -> Jetson RX | `PinConfig.h:13` `UartDriver.h:12` |
| RX | GPIO18 ESP32 RX <- Jetson TX | `PinConfig.h:13` |
| Baud | 115200 | `UartDriver.cpp:10` `Serial1.begin(115200)` `SystemManager.cpp:106` `ProtocolConfig.h:11` `SystemConfig.h:7` |
| Format | `SERIAL_8N1` | `UartDriver.cpp:10` |
| Debug UART | `Serial` / `UART0` CH343P 43/44 reserved | `UartDriver.cpp:9` comment + `PinConfig.h:86` |
| Framing | Binary `0xAA | ID | PAYLOAD | CRC16 | 0x55` | `ProtocolConfig.h:15` `ResponseManager.cpp:16` |
| CRC | `CRC16` `POLY 0x8005` `INIT 0xFFFF` `utils/Crc16.*` | `ProtocolConfig.h:24` `ResponseManager.cpp:137` |
| Baseline | `ProtocolConfig.h` kept, ASCII demo not copied | User decision |

Items above are **A. CONFIRMED**. No GPIO change, no baud change to 9600, no `Serial` for Jetson.

---

## 3. UART Configuration

| Aspect | Jetson Demo | ViosAssistant Current | Status |
|---|---|---|---|
| Port | `Serial` (`Serial.begin(9600)`) | `Serial1` (`UartDriver.cpp:10` `Serial1.begin(115200)`) | **MISMATCH** — Jetson must move to `Serial1` 17/18 |
| Baud | 9600 | 115200 (`SystemManager.cpp:106` `ProtocolConfig.h:11`) | **MISMATCH** — unify to **115200 CONFIRMED** |
| Pins | Not specified (assumes USB) | TX 17 / RX 18 (`PinConfig.h:13`) | **TBD for Jetson wiring** — confirm Jetson side matches 17/18 |
| Debug conflict | Uses `Serial` (would collide with CH343P 43/44 debug) | `Serial` reserved for debug | **MISMATCH if Jetson stays on Serial** — CONFIRMED to keep `Serial` for debug |
| Terminator | `readStringUntil('\r')` + `trim()` | Binary `0xAA ... 0x55` no `\r`/`\n` (`ResponseManager.cpp:16`) | **MISMATCH** — binary has no line terminator |
| RX buffer | Implicit `String` | `UartDriver.cpp:28` `buffer[64]` `available()` | **TBD** — Jetson burst 6 lines vs 64B buffer |
| CAN UART | Not in demo | `GPIO11/12` `500k` `CanDriver.h:8` STUB | **No conflict** if Jetson uses 17/18 |

**Verdict:** UART physical direction CONFIRMED (17/18 115200 Serial1). Demo 9600/Serial is **PROPOSAL not to be used**.

---

## 4. Binary Frame Baseline (CONFIRMED)

`config/ProtocolConfig.h:1` + `application/ResponseManager.cpp:1`:

```
[0] 0xAA HEADER
[1] FRAME/COMMAND ID
[2..n] PAYLOAD (0..64, CMD_MAX_PAYLOAD 64)
[n+1..n+2] CRC16 (LE, poly 0x8005 init 0xFFFF, over [0..n])
[n+3] 0x55 FOOTER
Timeout: 1000ms (`CMD_TIMEOUT_MS`)
```

Current `ResponseManager` implements:

- `sendResponse` `0xAA | cmd_type | success | error_code | resp_len | resp_data[0..7] | CRC | 0x55` (`ResponseManager.cpp:13`)
- `sendStatus` `0xAA 0x80 ... 0x55` 19B (`ResponseManager.cpp:31`)
- `sendTemperatureData` `0xAA 0x81 ... 0x55` 19B (`ResponseManager.cpp:57`)
- `sendVehicleData` `0xAA 0x82 ... 0x55` 17B (`ResponseManager.cpp:83`)
- `sendAck 0xFF` / `sendError 0xFE` 8B

IDs `VEHICLE_DATA_ID_BASE 0x100` etc. are defined but not used in `ResponseManager` frames (uses `0x80/0x81/0x82`). Baseline kept as-is for H01.5 — no new ID invented.

Jetson demo has **no header/footer/CRC/ID** — `code\r` + `println` — **PROPOSAL vs CONFIRMED mismatch**.

---

## 5. Jetson Command Semantic Mapping

Jetson demo `xu_ly_lenh(code)` has 11 distinct codes. ESP32 `model/Command.h:7` + `CommandManager.cpp:38` + `SystemManager.cpp:169` have 7 implemented + 4 NOT_IMPL assumption.

| Demo Code | Jetson Meaning (from demo `switch`) | ESP32 Command | Mapping | Status |
|---|---|---|---|---|
| 1 | `AC = 1` (AC ON) | `SET_AC 0x05` payload 1 | Semantic intent matches (AC ON) but Jetson sets var directly, ESP expects `payload[0]!=0` via `handleCommand` | **TBD** — needs confirmation that 1 maps to `SET_AC=1` |
| 2 | `AC = 0` (AC OFF) | `SET_AC 0x05` payload 0 | Same as above | **TBD** |
| 4 | `temperature += 2` (Temp +2 delta) | `SET_TEMPERATURE 0x01` expects absolute `payload[0]+payload[1]*0.1` (`SystemManager.cpp:172`) | **MISMATCH/TBD** — delta vs absolute, payload format not defined | **TBD** |
| 5 | `temperature -= 2` (Temp -2 delta) | `SET_TEMPERATURE` | Same | **TBD** |
| 6 | `if wind==0 wind=last` (Fan ON recover) | `SET_FAN_SPEED 0x02` absolute 0-255 (`SystemManager.cpp:177`) | **TBD** — Jetson level 0 + last storage vs ESP absolute | **TBD** |
| 7 | `if wind>0 last=wind; wind=0` (Fan OFF save) | `SET_FAN_SPEED` | Same | **TBD** |
| 8 | `last_mode = 2` (Face) | `SET_AIR_MODE 0x03` enum `VENT0/BI_LEVEL1/FLOOR2/MIX3/DEFROST4/FLOOR_DEFROST5` | **TBD** — enum values not confirmed to match 2 | **TBD** |
| 9 | `last_mode = 1` (Foot) | `SET_AIR_MODE` | **TBD** | **TBD** |
| 10 | `last_mode = 3` (Face+Foot) | `SET_AIR_MODE` | **TBD** — ESP has no 3=both, maybe `BI_LEVEL 1` or `MIX 3` | **TBD** |
| 101-105 | `wind = code-100; last=wind` (Fan level 1-5) | `SET_FAN_SPEED 0x02` range 0-255 | **TBD** — semantic requirement 1-5 vs ESP 0-255. Scale x51 (1->51 etc.) is **POSSIBILITY, NOT APPROVED / NOT SPECIFIED**, must not implement until confirmed | **TBD** |
| 324-332 | `temperature = code-300` (Set temp 24-32) | `SET_TEMPERATURE 0x01` | **TBD** — payload is single int 24-32 vs ESP 2-byte `temp*10`, range 16-30 in `SystemConfig.h:22` vs 24-32 | **TBD** |

**ESP-only not in Jetson demo:** `SET_RECIRCULATION 0x04`, `SET_HEATER 0x06`, `SET_DAMPER_POS 0x07` (NOT_IMPL), `REQUEST_STATUS 0x10`, `REQUEST_VEHICLE_DATA 0x11`, `SYSTEM_RESET 0xF0`, `FACTORY_RESET 0xF1` — document as ESP capability, Jetson may not need.

**No status** in table is **CONFIRMED** for 101-105 scale note. All mappings above are **TBD** except architecture.

---

## 6. Jetson Status Requirements

Jetson demo **expects** ESP to send 6 lines every 100ms (`loop: Serial.println`):

| Demo Field | Meaning (inferred from var name) | ESP32 Data Source | Binary Field | Status |
|---|---|---|---|---|
| `engine` | `int engine = 1` — unknown, seen as 0/1 | No direct source; maybe `SystemState.mode` or `VehicleData.ignition_on` | Not defined in `ResponseManager` | **TBD** — semantics need Jetson confirmation |
| `temperature` | `int temperature = 24` | `TemperatureData.inside_temp_c` / `setpoint_temp_c` (`SystemManager.cpp:129` NTC1) / `ClimateController` | `sendTemperatureData 0x81` has 5 temps `*10` | **TBD** — which temperature? |
| `AC` | `int AC = 0/1` | `ClimateController.getAC()` / `SystemState` | `sendStatus` / `sendResponse` has `ac` via `VehicleData`? Not 1:1 | **TBD** |
| `wind_value` | `0-5` fan level | `FanController.getSpeed()` 0-255 / `ClimateController` | `sendVehicleData` has `blower_active` bool, not level | **TBD** — representation not defined |
| `last_mode` | `0-3` air mode | `ClimateController.getAirMode()` enum | `sendStatus` has `mode` but not air mode | **TBD** |
| `door` | `int door = 1` | No source; `VehicleData` has no door field (`VehicleData.h:1` has speed/rpm/coolant/battery/ac/blower/gear) | Not defined | **TBD** |

No binary field currently matches 6-line order. **No fake field created.**

Additional TBD: delimiter `\n` vs `0xAA/0x55`, frequency 100ms vs `CommunicationTask:74` 50ms/500ms/1000ms timers (currently no send), ACK/error — Jetson demo has **no ACK handling**, ESP has `sendAck 0xFF`/`sendError 0xFE` and `sendResponse`.

All rows **TBD / NEEDS CONFIRMATION**.

---

## 7. RX Architecture

```
Jetson Nano
  -> Serial1 TX17/RX18 115200 8N1 (CONFIRMED)
  -> UartDriver.cpp:10 Serial1 (CONFIRMED, exists)
  -> CommunicationTask.cpp:62 processUartMessages() (EXISTS but STUB)
     - reads buffer[64] via UartDriver:28
     - // Process UART frame (DISCARD) — NO parser
  -> parser (MISSING — BLOCKER)
  -> CommandManager.h:9 queue 16, processCommand validates 7 vs 4 (EXISTS, assumption)
  -> SystemManager.cpp:157 handleCommand (EXISTS, 7 implemented)
  -> Services/Controllers (ClimateController, FanController, etc. EXISTS)
  -> Drivers (RelayDriver, PwmDriver, MotorDriver EXISTS)
  -> Hardware
```

**Nodes existing:** UartDriver, CommandManager queue/validate, SystemManager dispatch, Services/Drivers.  
**Nodes stub/missing:** **Parser** (`CommunicationTask:62` discard) — **P0 BLOCKER** — no ASCII `code\r` nor binary `0xAA` parsing, no `toInt()` nor `Command` creation.

---

## 8. TX Architecture

```
Hardware/Services (NTC, VehicleData, SystemState EXISTS)
  -> SystemManager.cpp:121 update() polls VehicleDataService, NTC, Climate
  -> ResponseManager.cpp:13 sendResponse / sendStatus 0x80 / sendTemperature 0x81 / sendVehicle 0x82 / sendAck 0xFF / sendError 0xFE (EXISTS, binary)
  -> UartDriver.cpp:20 write() Serial1 (EXISTS)
  -> Serial1 17/18 115200 (CONFIRMED)
  -> Jetson Nano
```

**Existing:** ResponseManager binary frames with CRC, UartDriver write.  
**Stub/Missing:** `CommunicationTask:74 sendPeriodicUpdates()` has **timers but no `ResponseManager` calls** — never sends `sendStatus`/`sendTemperatureData`/`sendVehicleData`. No 6-line `println` generator. Jetson 6-line expectation has **no binary mapping** — **P0 BLOCKER** if Jetson expects 6 lines.

Current TX path **exists at driver level** but **not wired** to periodic status.

---

## 9. Blockers

### P0 — Must fix before any end-to-end Jetson communication

- **P0-1 RX parser missing** `CommunicationTask.cpp:62` — no handling of Jetson `code` (neither `code\r` ASCII nor `0xAA` binary) → commands never reach `CommandManager`. **Blocks all 11 Jetson codes.**
- **P0-2 TX periodic status not wired** `CommunicationTask:74` — `sendPeriodicUpdates` empty → Jetson never receives status, 6-line expectation unmet, binary status never sent. **Blocks feedback loop.**
- **P0-3 Command mapping TBD** — 11 Jetson semantics have no confirmed binary encoding (Table §5 all TBD). Without mapping, parser cannot be implemented. **Blocks RX implementation.**
- **P0-4 Status field contract TBD** — 6 `engine/temperature/AC/wind/last_mode/door` have no confirmed binary fields or delimiters (§6 all TBD). **Blocks TX implementation.**

### P1 — Required before H04-H09 hardware tests with Jetson

- **P1-1 Baud/pins confirmation** — Jetson side must be confirmed to use `Serial1` 17/18 115200 (not `Serial` 9600). Wiring TX17->RX Jetson, RX18<-TX Jetson, common GND.
- **P1-2 Fan level 1-5 vs 0-255** — scale decision needed before `SET_FAN_SPEED` integration.
- **P1-3 Temp delta vs absolute** — `+2/-2` vs `324-332` vs `SET_TEMPERATURE` payload — choose one.
- **P1-4 Air mode enum** — 1/2/3 vs `AirMode` 0-5.

### P2 — Nice to have / future

- **P2-1 ESP-only commands** (`RECIRCULATION`, `HEATER`, etc.) — Jetson may ignore.
- **P2-2 `engine`/`door` source** — define if from `VehicleData` or `SystemState` or new sensor.
- **P2-3 Frequency** — 100ms 6-line vs binary 50/500/1000ms.
- **P2-4 ACK/error** — Jetson currently has none; ESP has `0xFF/0xFE` — decide if Jetson needs it.

---

## 10. TBD Requiring Team Confirmation

**For Jetson teammate — please confirm:**

1. **UART:** Agree to use `Serial1` `17/18` `115200` `8N1`, `Serial` for debug — confirm Jetson side pins/baud.
2. **Framing:** Confirm binary `0xAA ... CRC16 ... 0x55` as production (not ASCII `code\r`).
3. **Fan 1-5 (101-105):** What should ESP receive for level 1-5? Confirm mapping to `SET_FAN_SPEED` 0-255 is **TBD** — do you expect ESP to map 1-5 linearly, or should Jetson send 0-255 directly? `x51` is **not approved**.
4. **Temp:** Confirm `4/5` delta vs `324-332` absolute — which is production intent? What payload for `SET_TEMPERATURE` (single byte 24-32 vs 2-byte `*10`)?
5. **Air mode 8/9/10:** Confirm `1=Foot,2=Face,3=Both` mapping to ESP `AirMode` enum values.
6. **AC 1/2:** Confirm `1=ON,2=OFF` maps to `SET_AC 1/0`.
7. **Fan 6/7:** Confirm `6=ON recover last` and `7=OFF save` semantics vs ESP `FanController` `enable`/`setSpeed`.
8. **Status fields:** Confirm meaning of `engine` and `door`, and whether `wind_value` is 0-5 or 0-255, `last_mode` values, `temperature` which sensor, desired delimiter (`\n` vs binary) and period.
9. **Response type:** Does Jetson expect 6-line `println` or binary `0x80/0x81/0x82` or `sendResponse` ACK?
10. **Protocol IDs:** If binary, confirm `ProtocolConfig` `0xAA HEADER` etc. vs new IDs for Jetson commands.

No code will be changed until above confirmed.

---

## 11. Recommended Implementation Plan (No code change in H01.5)

**Phase A — Contract (team):** Agree on §10 items, produce one-page `Jetson-ESP32 Binary Contract` with table §5 exact mapping (e.g., `101 -> SET_FAN_SPEED payload 1`, not 51, until confirmed) and §6 status fields with binary layout.

**Phase B — RX (ESP):** Implement `CommunicationTask::processUartMessages` parser for `0xAA ... 0x55` (use `ProtocolConfig.h:15` CRC), create `Command` with `CommandType` per contract, `queueCommand` → `SystemManager::handleCommand`. Keep ASCII `code\r` only as debug fallback, not production.

**Phase C — TX (ESP):** Wire `CommunicationTask::sendPeriodicUpdates` to call `ResponseManager::sendStatus` / `sendTemperatureData` / `sendVehicleData` at confirmed period, or implement new 6-field binary frame if Jetson requires. Remove 6-line `println` expectation from Jetson side if binary chosen.

**Phase D — Jetson:** Update Jetson demo to `Serial1` (or `/dev/ttyTHS1`) `115200`, send binary frames per contract, parse binary responses, remove `9600` and `6 println`.

**Phase E — Test:** Loopback `UartDriver` test `T05` style on 17/18, then `T15` `CommandManager` with new codes, then `T21` production link, then H05 Pi/Jetson UART hardware test.

All phases keep `PinConfig.h:13` 17/18, no new GPIO, no TWAI, no `PinConfig` change.

---

## 12. Final Status

**PROTOCOL NOT READY**

**Reason:** Protocol architecture is now CONFIRMED (Serial1 17/18 115200 binary `0xAA ... CRC ... 0x55` per `ProtocolConfig.h`), but **command mapping for 11 Jetson codes (§5 all TBD)**, **status field contract for 6 `println` lines (§6 all TBD)**, and **RX parser + TX periodic wiring (§7-8 stubs)** remain incomplete. No end-to-end Jetson↔ESP32 communication can succeed until P0 blockers (§9) are resolved with team confirmation (§10).

---

**Verification (post-create):**

- No sentence claims `101->51` etc. as official — only `POSSIBILITY, NOT APPROVED`.
- No new GPIO, no new `CommandType` ID, no new payload format invented.
- No production file modified in H01.5 (checked `git diff` — only this report).

*End of audit — await team confirmation before implementation.*

