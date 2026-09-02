# THUYẾT MINH ĐỒ ÁN

## Hệ thống điều khiển điều hòa ô tô bằng trợ lý ảo (ViosAssistant)

---

## Phần 1 — ĐẶT VẤN ĐỀ

### 1.1. Bối cảnh

Hệ thống điều hòa trên ô tô truyền thống được điều khiển thủ công bằng núm xoay và phím bấm trên bảng táp lô. Người lái phải rời tay khỏi vô lăng để thao tác, gây mất tập trung và giảm an toàn khi lái xe.

Xu hướng hiện đại là tích hợp trợ lý ảo giọng nói (voice assistant) vào các hệ thống trên xe. Tuy nhiên, việc nâng cấp một hệ điều hòa ô tô đang lưu thông (after-market) gặp nhiều rào cản:

- Hệ thống bus điều khiển gốc của hãng khó can thiệp trực tiếp.
- Cần một phương án điều khiển cơ/tự động đáng tin cậy cho các thiết bị chấp hành.
- Yêu cầu kiến trúc phần mềm chạy trên vi điều khiển thời gian thực (real-time MCU).

### 1.2. Mục tiêu

Thiết kế và chế tạo một **bộ điều khiển điều hòa ô tô điều khiển bằng giọng nói** dựa trên nền tảng **ESP32-S3**:

1. Nhận lệnh giọng nói từ **Raspberry Pi** (xử lý STT/TTS) và các lệnh điều khiển khác.
2. Điều khiển **nhiệt độ** (cảm biến NTC), **quạt gió** (PWM + relay), **cánh gió** (motor DC + encoder), **đèn sưởi**, **rơ-le AC**.
3. Hiển thị trạng thái lên màn hình **OLED**.
4. Trao đổi dữ liệu với **module CAN/UART** để đọc dữ liệu xe.

### 1.3. Giới hạn

- Đây là giai đoạn **thiết kế phần cứng + phần mềm + kiểm thử phần mềm (software verification)**.
- **Chưa thực hiện kiểm thử phần cứng (hardware validation) H01–H09** — mọi test T01–T24 là kiểm thử compile/static/integration trên mã nguồn thật, chưa upload/flash chạy trên bo mạch thật.
- Các quyết định phần cứng còn tồn đọng (chân độ phân giải GPIO10, cực tính FET quạt, module CAN, nSLEEP motor) đang được đánh dấu **TBD** và chờ xác nhận trước khi kiểm thử phần cứng.

---

## Phần 2 — CƠ SỞ LÝ THUYẾT

### 2.1. Nền tảng ESP32-S3

- Vi điều khiển **ESP32-S3 N16R8 CH343P**: bộ đôi nhân Xtensa LX7 240 MHz, **16 MB Flash**, **8 MB PSRAM OPI**, giao tiếp nạp/debug qua **CH343P** (GPIO43/44).
- Hỗ trợ FreeRTOS, đủ ngoại vi cho bài toán: ADC (NTC), I2C (OLED), PWM/LEDC (quạt, motor), UART (Pi, CAN module), PCNT/GPIO (encoder).
- FQBN: `ESP32:esp32:esp32s3:CDCOnBoot=default,FlashSize=16M,PartitionScheme=default_8MB,UploadSpeed=921600`.

### 2.2. Các khối chức năng

- **Cảm biến NTC**: hệ số B=3950, điện trở nối tiếp 10 kΩ, nguồn 3.3 V, ADC 12-bit → chuyển đổi sang nhiệt độ.
- **Quạt gió**: relay cấp nguồn (GPIO5) + FET PWM (GPIO7, 1 kHz/8-bit) điều chỉnh tốc độ.
- **Cánh gió**: motor DC qua DRV8833 (GPIO13/14, 20 kHz/10-bit), phản hồi vị trí bằng encoder GA25 (GPIO19/20, 11300 xung/vòng).
- **Màn hình OLED**: SSD1306 128×64, địa chỉ I2C 0x3C, SDA8/SCL9.
- **Relay đầu ra**: AC (GPIO4), quạt (GPIO5), nguồn Pi (GPIO6).
- **Giao tiếp UART với Pi**: TX17/RX18, 115200 baud.
- **Module CAN/UART**: TX11/RX12, 500 k — giao tiếp qua module trung gian, **không phải CAN native TWAI**.

---

## Phần 3 — NỘI DUNG THỰC HIỆN

### 3.1. Kiến trúc phần mềm (phân tầng)

Mã nguồn **trên vi điều khiển** được tổ chức theo kiến trúc phân tầng rõ ràng nhằm đảm bảo khả năng bảo trì, kiểm thử và tách biệt phần cứng:

```
rtos/           (Task FreeRTOS: CommunicationTask, ControlTask, OledTask)
   ↑
application/    (SystemManager, CommandManager, ResponseManager)
   ↑
services/       (Climate, Fan, MotorPosition, VehicleData, AirMode)
   ↑
drivers/        (NTC, OLED, Relay, PWM, Motor, Encoder, CAN, UART)
   ↑
model/ utils/ config/   -- dữ liệu, tiện ích, cấu hình
```

| Tầng | Thư mục | Nội dung chính |
|---|---|---|
| Task (RTOS) | `rtos/` | CommunicationTask, ControlTask, OledTask |
| Ứng dụng | `application/` | SystemManager, CommandManager, ResponseManager |
| Dịch vụ | `services/` | ClimateController, FanController, MotorPositionController, VehicleDataService, AirModeController |
| Driver | `drivers/` | NtcDriver, OledDriver, RelayDriver, PwmDriver, MotorDriver, EncoderDriver, CanDriver, UartDriver |
| Mô hình dữ liệu | `model/` | Command, SystemState, TemperatureData, VehicleData, Snapshot |
| Cấu hình | `config/` | SystemConfig, ProtocolConfig |
| Tiện ích | `utils/` | Logger, Filter (Moving/Exp/Median), CRC16 |

**Quy tắc phụ thuộc:** tầng trên được phép sử dụng tầng dưới, không phụ thuộc ngược lại. `config/`, `model/`, `utils/` là tầng nền dùng chung.

### 3.2. Cấu hình chân (PinConfig.h) — nguồn duy nhất

`PinConfig.h` là **single source of truth** cho toàn bộ gán GPIO. Mọi driver truy cập qua macro `PIN_*`, không hardcode số chân trong code:

| Chức năng | Chân | Chức năng | Chân |
|---|---|---|---|
| NTC1 ADC | GPIO1 | Motor IN1 | GPIO13 |
| NTC2 ADC | GPIO2 | Motor IN2 | GPIO14 |
| Relay AC | GPIO4 | Pi UART TX | GPIO17 |
| Relay quạt | GPIO5 | Pi UART RX | GPIO18 |
| Relay nguồn Pi | GPIO6 | Encoder A (GA25) | GPIO19 |
| FET quạt PWM | GPIO7 | Encoder B (GA25) | GPIO20 |
| OLED SDA | GPIO8 | CH343P TX | GPIO43 (RESERVED) |
| OLED SCL | GPIO9 | CH343P RX | GPIO44 (RESERVED) |
| Đầu vào ON/OFF | GPIO10 (mode TBD) | Strapping | GPIO0/3/45/46 (DO NOT USE) |
| CAN UART TX | GPIO11 | | |
| CAN UART RX | GPIO12 | | |

> Lưu ý: GPIO19/20 = USB-OTG D−/D+ trên S3 chuẩn; trên bo N16R8 này USB dùng CH343P (43/44) nên 19/20 được cấp cho encoder theo **quyết định người dùng đã xác nhận** (cảnh báo nếu gắn cáp USB-OTG).

### 3.3. Logic điều khiển tiêu biểu

- **Fan hysteresis (hai ngưỡng):** `FAN_ON=25.5 °C`, `FAN_OFF=24.5 °C`, đặt tại `SystemConfig.h`, quyết định duy nhất tại `ClimateController::update()` (có chốt giữ 24.5…25.5, tránh dao động, hợp lệ `inside_valid`).
- **Bảo vệ an toàn:** driver stub trả `FAIL > fake success` (vd CanDriver `write()` luôn false), CommandManager phân biệt thành công / lệnh hợp lệ-chưa triển khai / lệnh không hợp lệ / queue đầy.
- **Phân biệt PWM:** quạt FET 1 kHz/8-bit (GPIO7) tách biệt với motor 20 kHz/10-bit (GPIO13/14).

### 3.4. Kiểm thử phần mềm T01–T24

Toàn bộ test verify mục tiêu kiểm thực, đối chiếu với API production và thông số thực nghiệm. **Tất cả là compile/static/integration, `HARDWARE: NOT EXECUTED`.**

| Nhóm | Testcase ID | Nội dung |
|---|---|---|
| Giai đoạn 3 — Ngoại vi | T01–T09 | OLED, NTC, GPIO output, GPIO input, Pi UART, Fan PWM, Motor DRV8833, Encoder, CAN UART |
| Giai đoạn 4 — Tiện ích/Dịch vụ | T10–T16 | Filter, CRC16, VehicleDataService, MotorPosition, FanController, CommandManager, ResponseManager |
| Giai đoạn 5 — Tích hợp | T17–T20 | SystemManager, Sensor→Service→App, Command→Controller→Driver, Hardware Abstraction |
| Giai đoạn 6 — Build/Kiểm tra | T21–T24 | Full production build (link 22 .cpp), Pin/Config audit, Dependency audit, Architecture audit |

**Kết quả T01–T24:** BUILD PASS **24/24**, static/logic PASS theo từng testcase, kích thước chương trình production link 366.813 bytes (~10% flash, ~7% RAM). Không có BUILD FAIL vĩnh viễn (chỉ lỗi môi trường toolchain `cc1plus CreateProcess` — cần retry, KHÔNG phải lỗi nguồn).

---

## Phần 4 — KẾT QUẢ ĐẠT ĐƯỢC

### 4.1. Kết quả

| Hạng mục | Trạng thái |
|---|---|
| Kiến trúc phần cứng | **HOÀN THÀNH** |
| Cấu hình chân (PinConfig.h) | **HOÀN THÀNH** — nguồn duy nhất |
| Kiến trúc phần mềm | **HOÀN THÀNH** |
| Kiểm thử phần mềm T01–T24 | **HOÀN THÀNH / ĐÃ KIỂM TRA** (compile/static/integration) |
| Build Production | **HOÀN THÀNH** — PASS (link đủ 22 .cpp) |
| Kiểm thử phần cứng (H01–H09) | **CHƯA THỰC HIỆN** |
| Milestone hiện tại | Hoàn tất kiểm thử phần mềm / chuẩn bị kiểm thử phần cứng |

### 4.2. Nhận xét

- Code được tách tầng, dùng macro `PIN_*`, có audit phụ thuộc và kiến trúc (T22–T24) cho thấy mã nguồn **sạch, nhất quán, dễ mở rộng**.
- Toàn bộ tham số thực nghiệm được pin: NTC B3950/10k/3.3V/12-bit; motor 20kHz/10-bit, đổi chiều 75ms; quạt 1kHz/8-bit; encoder 11300 PPR; Pi 115200; CAN 500k; CRC16 0xA001 init FFFF.
- **Không khai báo phần cứng PASS** vì chưa test phần cứng. `BUILD PASS ≠ HARDWARE PASS`.

---

## Phần 5 — HẠN CHẾ VÀ HƯỚNG PHÁT TRIỂN

### 5.1. Hạn chế / việc còn tồn đọng (BLOCKED)

1. **GPIO10 (đầu vào ON/OFF)** chưa xác định chế độ `INPUT / INPUT_PULLUP / INPUT_PULLDOWN` và cực tính — chờ đo mạch thực (H04).
2. **Cực tính FET quạt (GPIO7)** đang giả định active-HIGH — cần scope/DMM xác nhận (H06).
3. **CanDriver / VehicleDataService** là **stub**: `write()` trả false, parser chưa triển khai — chờ xác định spec module UART-to-CAN và termination 120 Ω (H09).
4. **Motor nSLEEP** điều khiển phần cứng, chưa code — chờ kiểm tra điện (H07).
5. **CommandManager**: 4 lệnh hợp lệ chưa triển khai (SET_DAMPER_POS, REQUEST_STATUS, REQUEST_VEHICLE_DATA, FACTORY_RESET) — trả mã lỗi 2, không giả thành công.
6. **Partition**: BUILD02 dùng `default_8MB`, Makefile dùng `default_16MB` — cần chốt khi flash production.

### 5.2. Hướng phát triển

- Thực hiện kiểm thử phần cứng H01→H09 theo ma trận đã lập, theo thứ tự ưu tiên và cách ly giữa motor/encoder.
- Triển khai đầy đủ `CanDriver` (mở UART) và parser `VehicleDataService` sau khi có spec module.
- Tích hợp và chạy thực tế luồng giọng nói: Pi (STT Vosk / TTS Piper) → lệnh → ESP32-S3 → chấp hành.
- Đóng gói nhúng: kiểm tra nguồn ổn định, cách ly 12V, ESD, watchdog, PSRAM.

---

## Phần 6 — KẾT LUẬN

Đồ án đã đạt được mục tiêu **thiết kế kiến trúc phần mềm hoàn chỉnh** cho bộ điều khiển điều hòa ô tô bằng trợ lý ảo trên nền ESP32-S3, với cấu hình chân tập trung, phân tầng rõ ràng và bộ **24 testcheck (T01–T24) đều PASS** về mặt biên dịch/tĩnh/tích hợp đối chiếu mã nguồn production thật.

**Phần mềm đang ở trạng thái sẵn sàng cho kiểm thử phần cứng** nhưng **chưa thực hiện kiểm thử phần cứng** do một số tham số (GPIO10, cực tính FET, spec CAN, nSLEEP) đang chờ xác nhận đo lường. Khi các tham số này được xác nhận, hệ thống có thể tiến hành H01–H09 để hoàn thiện sản phẩm.

---

*ViosAssistant — ESP32-S3 N16R8 CH343P — Master Thesis / Đồ án*
*Nguồn evidence: `FINAL_REPORT_T01-T24.md`, `FINAL_PRE_HARDWARE_REVIEW.md`, `FINALIZE_PRODUCTION_BEFORE_HARDWARE.md`, `UPDATE_REPORT_ENCODER_FAN_HYST.md`.*