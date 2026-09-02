# CAN BUS LAB GUIDE – ESP32 TWAI
### Sổ tay thực hành lab (Workbook)

> Thực hành CAN Bus trên ESP32 (PlatformIO + Arduino framework + `driver/twai.h`).
> File này là **sổ tay** — bạn thực hiện từng lab, đánh dấu checkbox, dán output Serial thật vào chỗ trống.
> Khi gặp FAIL, ghi hiện tượng và nguyên nhân theo hướng dẫn trước khi qua lab kế.

---

## 📌 Bảng theo dõi chuỗi lab

| # | Lab | Trạng thái | Kết quả |
|---|---|---|---|
| 1 | CAN TX/RX cơ bản | ⬜ | |
| 2 | CAN Data | ⬜ | |
| 3 | CAN ID | ⬜ | |
| 4 | CAN Filter | ⬜ | |
| 5 | DLC | ⬜ | |
| 6 | TX Queue & Transmission Status | ⬜ | |
| 7 | CANH/CANL | ⬜ | |
| 8 | Termination | ⬜ | |
| 9 | CAN Error / Status | ⬜ | |
| 10 | Mini ECU Project | ⬜ | |

---

## 1. Mục tiêu tổng quát

Sau khi hoàn thành chuỗi lab:

- Lắp ráp mạng CAN 2 node: **Node 1 (Transmitter)** ↔ Transceiver ↔ CANH/CANL ↔ **Node 2 (Receiver)**.
- Cấu hình TWAI driver 500 kbps, Standard Frame, Accept-All → chuyển sang filter theo ID.
- Giải thích chính xác: ID, DLC, Data, baudrate, CANH/CANL, ACK, arbitration, error, termination.
- Phân biệt: frame vào TX queue ≠ transmission hoàn thành trên bus.
- Đọc & hiểu `twai_status_info_t`, alerts, error counters, bus-off.
- Decode dữ liệu nhiều ID (mô phỏng ECU).

## 2. Phần cứng

| Thành phần | Chi tiết |
|---|---|
| ESP32 Node 1 | Transmitter — DoIt DevKit V1 |
| ESP32 Node 2 | Receiver — DoIt DevKit V1 |
| CAN Transceiver | TJA1050 / SN65HVD230 / MCP2551 — mỗi node 1 cái |
| CANH / CANL | 2 dây vi sai nối 2 transceiver |
| GND | Chung giữa 2 node |
| TX GPIO | **GPIO 5** (cố định) |
| RX GPIO | **GPIO 4** |
| Baudrate | **500 kbps** |
| Frame | Standard, ID 11-bit |
| CAN ID ban đầu | `0x036` |
| DLC | 8 |
| Termination | 120Ω × 2 (đầu bus) |

⚠️ **Cảnh báo:** GPIO 5/4 là chân 3.3V. **KHÔNG nối CANH/CANL trực tiếp vào GPIO** — bắt buộc qua CAN Transceiver. KHÔNG chạm/chập dây khi đang cấp nguồn.

## 3. Cấu trúc project & cách chạy chung

```
CAN/
├── CAN_Node_1/  → platformio.ini + src/main.cpp   (Transmitter)
├── CAN_Node_2/  → platformio.ini + src/main.cpp   (Receiver)
└── CAN_BUS_LAB_GUIDE.md  ← sổ tay này
```

`platformio.ini` (2 node giống nhau):
```ini
[env:esp32doit-devkit-v1]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
monitor_speed = 115200
```

**Cách mở Serial Monitor (nhắc lại mọi lab):**
1. Cắm 2 ESP32 → xem cổng: `pio device list` (hoặc PlatformIO → Devices).
2. Mở project `CAN_Node_X` trong VS Code → nút **plug** trên PlatformIO toolbar (hoặc terminal: `pio device monitor -p COMx`).
3. Một cổng COM chỉ 1 chương trình mở được → cần 2 cổng cho 2 node.
4. Build/flash: `pio run -t upload`.

**Lưu ý build:** define chân GPIO phải cast `gpio_num_t`:
```cpp
#define CAN_TX_PIN (gpio_num_t)5
#define CAN_RX_PIN (gpio_num_t)4
```

**Cách đấu dây chung (dùng cho mọi lab):**

| ESP32 | Transceiver |
|---|---|
| GPIO 5 | TXD |
| GPIO 4 | RXD |
| GND | GND |
| 3.3V/5V | VCC (theo loại) |

CANH–CANH, CANL–CANL giữa 2 transceiver, GND chung, 120Ω mỗi đầu bus.

---

# LAB 1 – CAN TX/RX cơ bản

## 🎯 Mục tiêu
Xác nhận toàn bộ đường truyền hoạt động: Node 1 gửi được frame, Node 2 nhận được frame giống hệt.

## 🔧 Chuẩn bị
- **Node bật:** Cả 2 node.
- **Phần cứng:** 2 ESP32, 2 CAN Transceiver, 2 điện trở 120Ω, dây nối.
- **Đấu dây:** Theo bảng "Cách đấu dây chung" mục 3.
- **Code:** Không sửa — `CAN_Node_1/src/main.cpp` và `CAN_Node_2/src/main.cpp` (đã build OK).

## 📝 Các bước thực hiện

### Bước 1
[ ] Cắm 2 ESP32 vào máy (2 cổng COM), ghi lại cổng: Node 1 = COM__15____, Node 2 = COM__9____

### Bước 2
[ ] Mở Serial Monitor Node 1 (project `CAN_Node_1`) — tần số 115200

### Bước 3
[ ] Mở Serial Monitor Node 2 (project `CAN_Node_2`) — tần số 115200

### Bước 4
[ ] Bật nguồn cả 2 node cùng lúc, quan sát đủ 10 giây (≈ 10 frame)

### Bước 5
[ ] Dán output thật của cả 2 node vào mục "Kết quả thực tế" bên dưới

## 🖥️ Output mong đợi

**Node 1 – mỗi giây:**
```text
[TX] Queued to TX queue (chua xac nhan len bus)
[TX] Frame details:
ID: 0x036
DLC: 8
DATA: 01 02 03 04 05 06 07 08
[TX] TWAI_ALERT_TX_SUCCESS: transmission completed
```

**Node 2 – mỗi giây:**
```text
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 01 02 03 04 05 06 07 08
```

## 📝 Kết quả thực tế của tôi

**Node 1:**
```text
[TX] Frame details:
ID: 0x036
DLC: 8
DATA: 38 39 3A 3B 3C 3D 3E 3F 
[TX] TWAI_ALERT_TX_SUCCESS: transmission completed
[TX] Queued to TX queue (chua xac nhan len bus)
[TX] Frame details:
ID: 0x036
DLC: 8
DATA: 39 3A 3B 3C 3D 3E 3F 40 
[TX] TWAI_ALERT_TX_SUCCESS: transmission completed
[TX] Queued to TX queue (chua xac nhan len bus)
[TX] Frame details:
ID: 0x036
DLC: 8
DATA: 3A 3B 3C 3D 3E 3F 40 41 
[TX] TWAI_ALERT_TX_SUCCESS: transmission completed
[TX] Queued to TX queue (chua xac nhan len bus)
[TX] Frame details:
ID: 0x036
DLC: 8
DATA: 3B 3C 3D 3E 3F 40 41 42 
[TX] TWAI_ALERT_TX_SUCCESS: transmission completed
[TX] Queued to TX queue (chua xac nhan len bus)
[TX] Frame details:
ID: 0x036
DLC: 8
DATA: 3C 3D 3E 3F 40 41 42 43 
[TX] TWAI_ALERT_TX_SUCCESS: transmission completed
[TX] Queued to TX queue (chua xac nhan len bus)
[TX] Frame details:
ID: 0x036
DLC: 8
DATA: 3D 3E 3F 40 41 42 43 44 
[TX] TWAI_ALERT_TX_SUCCESS: transmission completed
[TX] Queued to TX queue (chua xac nhan len bus)
[TX] Frame details:
ID: 0x036
DLC: 8
DATA: 3E 3F 40 41 42 43 44 45 
[TX] TWAI_ALERT_TX_SUCCESS: transmission completed
[TX] Queued to TX queue (chua xac nhan len bus)
[TX] Frame details:
ID: 0x036
DLC: 8
DATA: 3F 40 41 42 43 44 45 46 
[TX] TWAI_ALERT_TX_SUCCESS: transmission completed
[TX] Queued to TX queue (chua xac nhan len bus)
[TX] Frame details:
ID: 0x036
DLC: 8
DATA: 40 41 42 43 44 45 46 47 
[TX] TWAI_ALERT_TX_SUCCESS: transmission completed
[TX] Queued to TX queue (chua xac nhan len bus)
[TX] Frame details:
ID: 0x036
DLC: 8
DATA: 41 42 43 44 45 46 47 48 
```

**Node 2:**
```text
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 32 33 34 35 36 37 38 39 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 33 34 35 36 37 38 39 3A 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 34 35 36 37 38 39 3A 3B 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 35 36 37 38 39 3A 3B 3C 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 36 37 38 39 3A 3B 3C 3D 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 37 38 39 3A 3B 3C 3D 3E 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 38 39 3A 3B 3C 3D 3E 3F 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 39 3A 3B 3C 3D 3E 3F 40 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 3A 3B 3C 3D 3E 3F 40 41 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 3B 3C 3D 3E 3F 40 41 42 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 3C 3D 3E 3F 40 41 42 43 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 3D 3E 3F 40 41 42 43 44 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 3E 3F 40 41 42 43 44 45 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 3F 40 41 42 43 44 45 46 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 40 41 42 43 44 45 46 47 
```

## ✅ Kết luận
- [x] PASS — cả 2 node in đúng, data giống hệt
- [ ] FAIL — mô tả hiện tượng & nghi ngờ: ______________________
- [ ] Nếu FAIL: làm theo Checklist Troubleshooting (mục 15) rồi quay lại Bước 1

## 🧠 Kiến thức rút ra
- `ESP_OK` từ `twai_transmit()` = frame **vào TX queue**, chưa khẳng định đã lên bus.
- `TWAI_ALERT_TX_SUCCESS` = transmission hoàn thành theo TWAI.
- Mọi mắt xích (GPIO ↔ transceiver ↔ CANH/CANL ↔ termination ↔ GND) phải đủ thì mới truyền được.

---

# LAB 2 – CAN Data thay đổi theo counter

## 🎯 Mục tiêu
Quan sát 8 byte Data đổi mỗi giây theo `counter` và kiểm tra không mất frame.

## 🔧 Chuẩn bị
- **Node bật:** Cả 2 node.
- **Phần cứng:** Như LAB 1.
- **Code:** Không sửa. Node 1 đang tự tạo: `msg.data[i] = counter + i;` mỗi giây `counter++`.

## 📝 Các bước thực hiện

### Bước 1
[x] Bật cả 2 node, quan sát Node 2 liên tục **60 giây**

### Bước 2
[x] Ghi lại 5 frame đầu tiên (chuỗi byte) đúng thứ tự

### Bước 3
[x] Đếm tổng số frame Node 2 nhận trong 60 giây → số lượng: ______ (dự kiến ≈ 60)

### Bước 4
[x] Kiểm tra chuỗi data có **liên tục** không (không nhảy cóc: 01..08 → 03..0A là mất frame)

## 🖥️ Output mong đợi

```text
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 01 02 03 04 05 06 07 08
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 02 03 04 05 06 07 08 09
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 03 04 05 06 07 08 09 0A
```

## 🔍 Giải thích
- **Byte nào đang thay đổi?** → Cả 8 byte tăng đều 1/giây vì toàn bộ data = `counter + i`.
- **`counter` hoạt động thế nào?** → Biến `uint8_t` tăng 1 sau mỗi lần gửi; tràn tại 255 → quay về 0.
- **Node 2 nhận đúng không?** → Mỗi frame in ra phải là 8 byte liên tiếp từ `counter` tương ứng.
- **Mất frame?** → Chuỗi đếm tăng dần là "số thứ tự": nhảy cóc = mất frame; đếm frame/60s ≈ 60 = không mất.

## 📝 Kết quả thực tế của tôi

```text không? [x] Có [ ] Không

## ✅ Kết luận
- [x] PASS — chuỗi liên tục, đủ số frame
- [ ] FAIL — mô tả: ______________________

## 🧠 Kiến thức rút ra
- Data 8 byte truyền nguyên vẹn qua bus khi DLC = 8.
- Dữ liệu dạng "đếm tăng dần" là kỹ thuật chuẩn để phát hiện mất frame.
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 0D 0E 0F 10 11 12 13 14 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 0E 0F 10 11 12 13 14 15 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 0F 10 11 12 13 14 15 16 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 10 11 12 13 14 15 16 17 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 11 12 13 14 15 16 17 18 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 12 13 14 15 16 17 18 19 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 13 14 15 16 17 18 19 1A 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 14 15 16 17 18 19 1A 1B 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 15 16 17 18 19 1A 1B 1C 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 16 17 18 19 1A 1B 1C 1D 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 17 18 19 1A 1B 1C 1D 1E 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 18 19 1A 1B 1C 1D 1E 1F 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 19 1A 1B 1C 1D 1E 1F 20 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 1A 1B 1C 1D 1E 1F 20 21 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 1B 1C 1D 1E 1F 20 21 22 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 1C 1D 1E 1F 20 21 22 23 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 1D 1E 1F 20 21 22 23 24 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 1E 1F 20 21 22 23 24 25 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 1F 20 21 22 23 24 25 26 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 20 21 22 23 24 25 26 27 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 21 22 23 24 25 26 27 28 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 22 23 24 25 26 27 28 29 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 23 24 25 26 27 28 29 2A 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 24 25 26 27 28 29 2A 2B 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 25 26 27 28 29 2A 2B 2C 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 26 27 28 29 2A 2B 2C 2D 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 27 28 29 2A 2B 2C 2D 2E 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 28 29 2A 2B 2C 2D 2E 2F 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 29 2A 2B 2C 2D 2E 2F 30 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 2A 2B 2C 2D 2E 2F 30 31 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 2B 2C 2D 2E 2F 30 31 32 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 2C 2D 2E 2F 30 31 32 33 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 2D 2E 2F 30 31 32 33 34 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 2E 2F 30 31 32 33 34 35 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 2F 30 31 32 33 34 35 36 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 30 31 32 33 34 35 36 37 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 31 32 33 34 35 36 37 38 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 32 33 34 35 36 37 38 39 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 33 34 35 36 37 38 39 3A 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 34 35 36 37 38 39 3A 3B 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 35 36 37 38 39 3A 3B 3C 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 36 37 38 39 3A 3B 3C 3D 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 37 38 39 3A 3B 3C 3D 3E 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 38 39 3A 3B 3C 3D 3E 3F 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 39 3A 3B 3C 3D 3E 3F 40 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 3A 3B 3C 3D 3E 3F 40 41 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 3B 3C 3D 3E 3F 40 41 42 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 3C 3D 3E 3F 40 41 42 43 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 3D 3E 3F 40 41 42 43 44 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 3E 3F 40 41 42 43 44 45 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 3F 40 41 42 43 44 45 46 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 40 41 42 43 44 45 46 47 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 41 42 43 44 45 46 47 48 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 42 43 44 45 46 47 48 49 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 43 44 45 46 47 48 49 4A 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 44 45 46 47 48 49 4A 4B 
[RX] No frame within 1s (bus idle or not connected)
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 45 46 47 48 49 4A 4B 4C 
[RX] No frame within 1s (bus idle or not connected)
```

- Tổng frame / 60 giây: ______ (dự kiến ≈ 60)
- Chuỗi data có liên tục

---

# LAB 3 – CAN ID

## 🎯 Mục tiêu
Hiểu ID định danh frame: đổi ID Node 1 và quan sát Node 2 vẫn nhận (vì Accept-All).

## 🔧 Chuẩn bị
- **Node bật:** Cả 2 node.
- **Phần cứng:** Như LAB 1.
- **Code sửa:** Chỉ sửa **Node 1**, dòng trong `loop()`:

```cpp
msg.identifier = 0x036;   // ← đổi dòng này cho mỗi bước
```

## 📝 Các bước thực hiện

### Bước 1
[ ] Sửa Node 1: `msg.identifier = 0x100;` → flash (`pio run -t upload`)
[ ] Ghi lại ID Node 2 hiển thị: ______ (dự kiến `0x100`)

### Bước 2
[ ] Sửa Node 1: `msg.identifier = 0x200;` → flash
[ ] Ghi lại ID Node 2 hiển thị: ______ (dự kiến `0x200`)

### Bước 3
[ ] Sửa Node 1: `msg.identifier = 0x7FF;` → flash
[ ] Ghi lại ID Node 2 hiển thị: ______ (dự kiến `0x7FF`)

### Bước 4 — bài tập nhỏ (đoán trước khi chạy)
[ ] **Đoán trước:** đổi ID thành `0x123`, Node 2 sẽ in ID nào? → Dự đoán: ______
[ ] Chạy thật → ID Node 2 in: ______ — dự đoán đúng chưa? [ ] Đúng [ ] Sai (giải thích: ________________)

## 🖥️ Output mong đợi

```text
[RX] CAN Frame Received
ID: 0x100
DLC: 8
DATA: ...
[RX] CAN Frame Received
ID: 0x200
DLC: 8
DATA: ...
[RX] CAN Frame Received
ID: 0x7FF
DLC: 8
DATA: ...
```

## 🔍 Giải thích
- **Standard CAN ID 11 bit:** giá trị `0x000`–`0x7FF` (2048 ID). ID = định danh "chủ đề" dữ liệu trên mạng.
- **Vì sao Node 2 nhận mọi ID?** → `TWAI_FILTER_CONFIG_ACCEPT_ALL`: driver không lọc, mọi frame hợp lệ vào RX queue.
- Sau LAB 4, Node 2 sẽ lọc theo ID — lúc đó ID trở nên quan trọng.

## 📝 Bảng kết quả test

| ID Node 1 gửi | ID Node 2 nhận | Khớp? |
|---|---|---|
| 0x036 | 0x036 |
| 0x100 | 0x100 |
| 0x200 | 0x200 |
| 0x7FF | 0x7FF |
| 0x123 (bài tập) | 0x123 |

## ✅ Kết luận
- [x] PASS — Node 2 in đúng mọi ID gửi
- [ ] FAIL — mô tả: ______________________

## 🧠 Kiến thức rút ra
- ID 11-bit = định danh + (sẽ học LAB 6) độ ưu tiên arbitration.
- Accept-All nhận mọi ID — vô dụng khi mạng có nhiều loại dữ liệu → cần filter.
---

# LAB 4 – CAN Filter

## 🎯 Mục tiêu
Lọc frame ngay trong controller: Node 2 chỉ nhận ID `0x036` dù Node 1 gửi nhiều ID.

## 🔧 Chuẩn bị
- **Node bật:** Cả 2 node.
- **Phần cứng:** Như LAB 1.
- **Code sửa:**
  - **Node 2:** thay filter trong `setup()`:

> ⚠️ **Lưu ý version API:** framework của bạn (ESP-IDF v4.4.7 / Arduino core 3.2.x) **KHÔNG có** macro `TWAI_FILTER_CONFIG_SINGLE` và hằng `STD_MSG`/`EXT_MSG` (chúng chỉ xuất hiện từ ESP-IDF v5.x). Cách đúng cho v4.4: dùng `TWAI_FILTER_CONFIG_ACCEPT_ALL()` làm khởi tạo rồi **ghi đè** `acceptance_code`/`acceptance_mask`:

```cpp
// thay: twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();  // code=0, mask=0xFFFFFFFF
f_config.acceptance_code = 0x036 << 21;   // chỉ khớp ID 0x036 (standard frame: ID nằm ở 11 bit cao)
f_config.acceptance_mask  = 0x7FF << 21;  // so đủ 11 bit ID
```

  - **Node 1:** gửi xen kẽ 3 ID — trong `loop()`, chọn ID theo bộ đếm giây: giây thứ k: k%3==0 → `0x036`, ==1 → `0x100`, ==2 → `0x200`.

## 📝 Các bước thực hiện

### Bước 1
[ ] Sửa Node 2 theo code trên → build & flash

### Bước 2
[ ] Sửa Node 1 để gửi xen kẽ `0x036 / 0x100 / 0x200` → build & flash

### Bước 3
[ ] Bật cả 2 node, quan sát Node 2 trong 15 giây (> 15 frame)

### Bước 4
[ ] Đếm: tổng frame in ra ______ ; trong đó bao nhiêu frame `0x036`? ______
[ ] Có bao giờ thấy `0x100` hoặc `0x200` không? [ ] Không [ ] Có (nếu có → kiểm tra lại code/cấu hình)

## 🖥️ Output mong đợi

```text
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: ...
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: ...
(… không bao giờ có 0x100 / 0x200 …)
```

## 🔍 Giải thích
- **Khác nhau giữa 2 filter:**
  - `TWAI_FILTER_CONFIG_ACCEPT_ALL()` → code/mask = 0 → nhận MỌI frame hợp lệ.
  - Filter đơn → chỉ frame khớp `acceptance_code` (qua `acceptance_mask`) mới được đẩy vào RX queue.
- **Vì sao Node 2 chỉ hiển thị `0x036`?** → Bộ lọc nằm **trong TWAI controller** (phần cứng): frame không khớp bị loại trước khi tới driver/firmware — không tốn RX queue, không bị in ra.
- Ghi chú kỹ thuật: ESP-IDF v4.4 lưu ID standard ở 11 bit cao (`<< 21`); mask `0x7FF << 21` = so sánh đủ 11 bit.

## 📝 Kết quả thực tế của tôi

```text
<!-- DÁN OUTPUT NODE 2 VÀO ĐÂY -->
```

## ✅ Kết luận
- [ ] PASS — chỉ có `0x036` lọt qua
- [ ] FAIL — mô tả: ______________________

## 🧠 Kiến thức rút ra
- Lọc ID diễn ra ở tầng phần cứng controller, không phải trong code đọc RX.
- Filter giảm tải CPU/RX queue khi mạng đông node.
---

# LAB 5 – DLC

## 🎯 Mục tiêu
Hiểu DLC = số byte Data; đọc đúng số byte thay vì luôn đọc 8.

## 🔧 Chuẩn bị
- **Node bật:** Cả 2 node.
- **Phần cứng:** Như LAB 1.
- **Code sửa:** Chỉ **Node 1** — đổi `msg.data_length_code` và chuẩn bị data tương ứng:

```cpp
// Bước 1: DLC = 1
msg.data_length_code = 1;
msg.data[0] = 0x11;

// Bước 2: DLC = 2
msg.data_length_code = 2;
msg.data[0] = 0x11; msg.data[1] = 0x22;

// Bước 3: DLC = 4  → data: 11 22 33 44
// Bước 4: DLC = 8  → data: 11 22 33 44 55 66 77 88
```

## 📝 Các bước thực hiện

### Bước 1
[ ] Sửa Node 1 DLC=1 → flash → quan sát Node 2, ghi `DLC:` và `DATA:` thật vào bảng kết quả

### Bước 2
[ ] Sửa Node 1 DLC=2 → flash → ghi kết quả

### Bước 3
[ ] Sửa Node 1 DLC=4 → flash → ghi kết quả

### Bước 4
[ ] Sửa Node 1 DLC=8 → flash → ghi kết quả

## 🖥️ Output mong đợi

```text
[RX] CAN Frame Received
ID: 0x036
DLC: 1
DATA: 11
[RX] CAN Frame Received
ID: 0x036
DLC: 2
DATA: 11 22
[RX] CAN Frame Received
ID: 0x036
DLC: 4
DATA: 11 22 33 44
[RX] CAN Frame Received
ID: 0x036
DLC: 8
DATA: 11 22 33 44 55 66 77 88
```

## 🔍 Giải thích
- **DLC là gì?** Data Length Code — trường trong header frame, khai báo số byte Data đi theo (0–8).
- **DLC = 8 nghĩa là gì?** Frame mang đúng 8 byte. Receiver chỉ nên đọc `msg.data_length_code` byte.
- **Đọc đủ 8 byte dù DLC nhỏ thì sao?** → Đọc nhầm byte rác ngoài vùng data của frame (không phải dữ liệu từ bus). Đây là bug hay gặp khi code cứng `for (i = 0; i < 8; ...)`.

## 📝 Bảng kết quả test

| DLC | Data Node 2 nhận | Đúng số byte? |
|---|---|---|
| 1 | | |
| 2 | | |
| 4 | | |
| 8 | | |

## ✅ Kết luận
- [ ] PASS — số byte in ra luôn khớp DLC
- [ ] FAIL — mô tả: ______________________

## 🧠 Kiến thức rút ra
- Luôn in/đọc theo `data_length_code`, không hard-code số byte.

---

# LAB 6 – TX Queue & Transmission Status

## 🎯 Mục tiêu
Phân biệt chính xác: frame **vào TX queue** / transmission **hoàn thành** / transmission **thất bại** — bằng cách bật/tắt Node 2.

## 🔧 Chuẩn bị
- **Node bật:** Tùy bước (xem bên dưới) — Node 1 luôn bật.
- **Phần cứng:** Như LAB 1.
- **Code:** Không sửa — Node 1 đã có sẵn đọc alerts + in `twai_status_info_t` khi `TX_FAILED`.

## 📝 Các bước thực hiện

### Bước 1 — Chỉ bật Node 1
[ ] Tắt nguồn Node 2 (hoặc rút dây bus phía Node 2 — an toàn, đã tắt nguồn)
[ ] Quan sát Node 1 15 giây — ghi các dòng xuất hiện:
  - [ ] `[TX] Queued to TX queue...` có không? [ ] Có [ ] Không
  - [ ] `TWAI_ALERT_TX_SUCCESS` có không? [ ] Có [ ] Không
  - [ ] `TWAI_ALERT_TX_FAILED` có không? [ ] Có [ ] Không
  - [ ] `tx_err` / `bus_err` counters in ra giá trị gì? ______

### Bước 2 — Bật lại Node 2
[ ] Bật nguồn Node 2
[ ] Quan sát Node 1 15 giây — ghi lại:
  - [ ] `TWAI_ALERT_TX_SUCCESS` xuất hiện đều không? [ ] Có [ ] Không
  - [ ] `TX_FAILED` còn xuất hiện không? [ ] Hết [ ] Còn

## 🖥️ Output mong đợi

**Bước 1 (1 mình):** queue OK nhưng transmission không hoàn thành -> `TX_FAILED` + counters tăng dần.
**Bước 2 (2 node):** `TX_SUCCESS` đều đặn, `TX_FAILED` biến mất, counters về/giữ 0.

## 🔍 Giải thích (giới hạn đúng dữ liệu API cung cấp)

```
twai_transmit(msg, timeout)          → ESP_OK: frame VÀO TX QUEUE
        ↓
TX Queue → TWAI Controller → Transceiver → CAN Bus
        ↓
trạng thái truyền:
  TWAI_ALERT_TX_SUCCESS  → transmission hoàn thành (theo TWAI)
  TWAI_ALERT_TX_FAILED   → transmission thất bại
```

| Trạng thái | Dấu hiệu | Được phép kết luận |
|---|---|---|
| Trong TX queue | `ESP_OK` | CHỈ: frame đã vào queue. KHÔNG khẳng định đã lên bus |
| Hoàn thành | `TWAI_ALERT_TX_SUCCESS` | Transmission hoàn thành theo trạng thái TWAI. KHÔNG khẳng định tuyệt đối về nguồn ACK (API không cung cấp bằng chứng) |
| Thất bại | `TWAI_ALERT_TX_FAILED` | Transmission thất bại. KHÔNG mặc định nguyên nhân — xem counters để đối chiếu |

**Phân biệt lý thuyết (để đối chiếu, không phải kết luận tự động):**
- **Arbitration loss** — cơ chế arbitration bình thường: 2 node gửi cùng lúc, ID thấp thắng.
- **ACK error** — loại lỗi riêng (frame không được node nào xác nhận ở vùng ACK).
- **Bus error / bus-off** — các trạng thái lỗi khác nhau.

Trích comment `driver/twai.h` (ESP-IDF v4.4.7):
- `TWAI_ALERT_TX_SUCCESS`: "The previous transmission was successful"
- `TWAI_ALERT_TX_FAILED`: "The previous transmission has failed (for single shot transmission)"

Kết luận thực nghiệm KHÔNG suy diễn: một node đơn độc không tạo được giao tiếp CAN hoàn chỉnh — được minh chứng bằng TX_FAILED + counters tăng, không cần bịa nguyên nhân.

## 📝 Kết quả thực tế của tôi

**Bước 1 — Node 1 một mình:**
```text
<!-- DÁN OUTPUT VÀO ĐÂY -->
```

**Bước 2 — Có Node 2:**
```text
<!-- DÁN OUTPUT VÀO ĐÂY -->
```

## ✅ Kết luận
- [ ] PASS — quan sát được sự khác biệt rõ ràng giữa 1 node và 2 node
- [ ] FAIL — mô tả: ______________________

## 🧠 Kiến thức rút ra
- `ESP_OK` ≠ xong: queue và transmission là 2 mức khác nhau.
- Alerts là nguồn sự thật của driver; counters là bằng chứng để đối chiếu, không suy diễn.

---

# LAB 7 – CANH/CANL

## 🎯 Mục tiêu
Hiểu tín hiệu vi sai và hậu quả khi đảo dây.

## 🔧 Chuẩn bị
- **Node bật:** Cả 2 node (bật/tắt nguồn theo từng bước — an toàn).
- **Phần cứng:** Như LAB 1.
- **Code:** Không sửa.

## 📝 Các bước thực hiện

### Bước 1 — Trạng thái đúng
[ ] Đấu đúng (CANH–CANH, CANL–CANL), bật cả 2 node xác nhận hoạt động bình thường như LAB 1

### Bước 2 — Đảo dây (an toàn: đã tắt nguồn)
[ ] TẮT NGUỒN cả 2 node
[ ] Tháo 2 dây CANH/CANL, hoán đổi: node 1 CANH → node 2 CANL và ngược lại
[ ] (Không chạm tay vào dây đồng trần khi có nguồn; không chập CANH–CANL với nguồn)
[ ] Bật nguồn cả 2 node, quan sát 15 giây — ghi hiện tượng:

### Bước 3 — Ghi bằng chứng
[ ] Node 1: có `TWAI_ALERT_TX_FAILED` không? [ ] Có [ ] Không
[ ] Node 1: `tx_err`, `bus_err`, `arb_lost` counters in ra là bao nhiêu? ______
[ ] Node 2: có `[RX]` frame nào không? [ ] Có [ ] Không (dự kiến: không)

### Bước 4 — Phục hồi
[ ] TẮT NGUỒN cả 2 node
[ ] Đấu lại đúng: CANH→CANH, CANL→CANL
[ ] Bật nguồn 2 node → xác nhận hoạt động bình thường trở lại

## 🔍 Giải thích
- **CANH (CAN High) / CANL (CAN Low):** cặp dây vi sai. Bit dominant (0): CANH ≈ 3.5V, CANL ≈ 1.5V → hiệu **+2V**; bit recessive (1): cả 2 ≈ 2.5V → hiệu **0V**.
- **Đảo dây ⇒** polarity đảo: "dominant" do cả 2 node hiểu lệch → tranh chấp bit, lỗi truyền. Dự kiến: TX_FAILED, counters TEC/bus_error tăng, node còn lại không nhận được frame hợp lệ.
- **KHÔNG kết luận nguyên nhân chỉ từ 1 alert** — kết hợp counters + hiện tượng 2 node như trên.
- Sau thử nghiệm: **đấu đúng lại** là bắt buộc.

## 📝 Kết quả thực tế của tôi

```text
<!-- DÁN OUTPUT 2 NODE (lúc đảo dây) VÀO ĐÂY -->
```

## ✅ Kết luận
- [ ] PASS — quan sát thấy khác biệt đúng/đảo và phục hồi sau khi đấu lại
- [ ] FAIL — mô tả: ______________________

## 🧠 Kiến thức rút ra
- Vi sai = đo hiệu CANH−CANL → nguồn miễn nhiễu tốt nhưng "đảo cực" tức lỗi lập tức.
- Đấu dây đúng chiều là chuẩn đầu tiên khi debug mạng CAN.

---

# LAB 8 – Termination

## 🎯 Mục tiêu
Hiểu điện trở kết thúc 120Ω và cách kiểm tra bằng đo đạc an toàn.

## 🔧 Chuẩn bị
- **Node bật:** Tắt nguồn khi đo; 2 node để test chạy.
- **Phần cứng:** Đồng hồ vạn năng (thang đo điện trở), 2 điện trở 120Ω (nếu module chưa có sẵn).
- **Code:** Không sửa.

## 📝 Các bước thực hiện

### Bước 1 — Xác định module có termination chưa
[ ] Đọc datasheet module transceiver của bạn: có ghi vị trí/resistor 120Ω hàm sẵn? [ ] Có [ ] Không [ ] Không rõ
[ ] Nếu module có cầu hàn/vị trí kèm 120Ω → đảm bảo đã hàn/đóng (nếu thiết kế cho phép)

### Bước 2 — Đo tổng trở bus (AN TOÀN: tắt nguồn!)
[ ] TẮT NGUỒN cả 2 node
[ ] Đặt đồng hồ ở thang Ω, đo giữa **CANH và CANL** tại một node
[ ] Đọc kết quả: R = ______ Ω
  - ≈ 60Ω → 2 đầu đều có 120Ω ✓ (chuẩn)
  - ≈ 120Ω → chỉ 1 đầu có → thêm 120Ω đầu còn lại
  - ∞ (OL) → chưa có → lắp 2 điện trở 120Ω
[ ] (Không đo khi bus đang có nguồn — ghi sai và nguy hiểm)

### Bước 3 — Chạy kiểm tra với termination đầy đủ
[ ] Bật 2 node → xác nhận hoạt động như LAB 1 (TX_SUCCESS / RX đều)

### Bước 4 — (Tùy chọn, an toàn) thử thiếu 1 termination
[ ] Tắt nguồn, tháo điện trở 120Ω ở 1 đầu (nếu rời)
[ ] Bật 2 node, quan sát 15 giây — có lỗi/tăng counters không? Ghi: ______
[ ] Tắt nguồn, lắp lại → xác nhận phục hồi

## 🖥️ Output mong đợi

**Bước 2:** R ≈ 60Ω (nối CANH–CANL, nguồn tắt).
**Bước 4 (nếu thử):** tùy dây dài/chất lượng — có thể xuất hiện lỗi bit; nếu dây ngắn có thể vẫn chạy được (ghi nhận đây là hành vi thực tế, không suy diễn nguyên nhân tuyệt đối).

## 🔍 Giải thích
- **120Ω dùng để làm gì?** Khớp trở kháng đặc tính cáp (~120Ω) → tiêu hao phản xạ cuối đường truyền, giữ biên độ tín hiệu sạch ở tốc độ cao.
- **Vì sao 2 đầu?** Phản xạ phát sinh ở cả 2 đầu vật lý của cáp; thiếu 1 đầu → phản xạ ở đầu hở.
- **Tổng trở đo ≈ 60Ω** = hai điện trở 120Ω song song (`120//120 = 60`). Đây là cách kiểm tra nhanh tính toàn vẹn bus.

## 📝 Kết quả thực tế của tôi

```text
<!-- R đo được: ______ Ω  (nguồn đã tắt) -->
```

## ✅ Kết luận
- [ ] PASS — đo được ≈ 60Ω và hiểu cách xử lý nếu số đo khác
- [ ] FAIL — mô tả: ______________________

## 🧠 Kiến thức rút ra
- Bus CAN chuẩn: 120Ω mỗi đầu → đo ≈ 60Ω (nguồn tắt).
- Đo CANH–CANL bằng đồng hồ Ω là bài kiểm tra cáp nhanh không cần code.

---

# LAB 9 – CAN Error / Status

## 🎯 Mục tiêu
Đọc `twai_status_info_t` và theo dõi error counters qua 3 kịch bản: mạng khỏe / mất node / đảo dây.

## 🔧 Chuẩn bị
- **Node bật:** Cả 2 node (một số bước tắt Node 2).
- **Phần cứng:** Như LAB 1.
- **Code sửa:** Node 1 — thêm in status định kỳ vào `loop()` (hiện Node 1 đã in khi `TX_FAILED`; thêm bản in mỗi 5 giây):

```cpp
static unsigned long lastStatus = 0;
if (now - lastStatus >= 5000) {
    lastStatus = now;
    twai_status_info_t st;
    if (twai_get_status_info(&st) == ESP_OK) {
        Serial.printf("[STATUS] state=%d tx_err=%u rx_err=%u "
                      "msgs_to_tx=%u msgs_to_rx=%u arb_lost=%u bus_err=%u tx_failed=%u\n",
                      (int)st.state, st.tx_error_counter, st.rx_error_counter,
                      st.msgs_to_tx, st.msgs_to_rx,
                      st.arb_lost_count, st.bus_error_count, st.tx_failed_count);
    }
}
```

> API đã kiểm tra đúng trên framework này (ESP-IDF v4.4.7). Các field **thực sự tồn tại**: `state, msgs_to_tx, msgs_to_rx, tx_error_counter, rx_error_counter, tx_failed_count, rx_missed_count, rx_overrun_count, arb_lost_count, bus_error_count`. KHÔNG có `twai_error_state_t` — mức lỗi theo dõi qua alerts + counters.

## 📝 Các bước thực hiện

### Bước 1 — Mạng khỏe mạnh
[ ] Bật cả 2 node 30 giây → ghi `[STATUS]` (state, tx_err, rx_err, arb_lost, bus_err) xuống kết quả
  - Dự kiến: state=1 (RUNNING), các counters = 0

### Bước 2 — Mất node kia (hoặc rút dây bus 1 bên — tắt nguồn trước)
[ ] Tắt Node 2 (nguồn) → quan sát Node 1 30 giây
[ ] Ghi counters: tx_err ______, bus_err ______, tx_failed ______ — có tăng không?
[ ] Có thấy `ERR_PASS` / `BUS_OFF` trong 30 giây không? [ ] Không [ ] Có (ghi thời điểm: ______)

### Bước 3 — Đảo dây (như LAB 7, tắt nguồn trước)
[ ] Đảo CANH/CANL → bật 2 node → ghi counters sau 30 giây: ______
[ ] Tắt nguồn, đấu lại đúng

### Bước 4 — Hồi phục
[ ] Bật lại 2 node → theo dõi counters có dần trở về 0 không? [ ] Có [ ] Không
[ ] state khi hồi phục là mấy? ______

## 🖥️ Output mong đợi

```text
[STATUS] state=1 tx_err=0 rx_err=0 msgs_to_tx=0 msgs_to_rx=0 arb_lost=0 bus_err=0 tx_failed=0   ← mạng khỏe
[STATUS] state=1 tx_err=1__ rx_err=0 ... bus_err=2__ ...                                      ← mất node: counters tăng
```

## 🔍 Giải thích (đúng với API framework hiện tại)

`twai_state_t` (field `state`):

| Giá trị | Tên | Ý nghĩa |
|---|---|---|
| 0 | `TWAI_STATE_STOPPED` | Controller dừng, không tham gia bus |
| 1 | `TWAI_STATE_RUNNING` | Đang chạy, TX/RX bình thường |
| 2 | `TWAI_STATE_BUS_OFF` | Bus-off — ngừng tham gia bus |
| 3 | `TWAI_STATE_RECOVERING` | Đang hồi phục |

**4 mức trạng thái lỗi CAN 2.0** (kiến thức chuẩn — framework này theo dõi bằng counters + alerts, không có enum riêng):

| Mức | Điều kiện (TEC/REC) | Hoạt động |
|---|---|---|
| ERROR-ACTIVE | cả 2 < 128 | Bình thường, phát error frame chủ động |
| ERROR-WARNING | 1 counter ≥ 96 | Alert `TWAI_ALERT_ABOVE_ERR_WARN` |
| ERROR-PASSIVE | 1 counter ≥ 128 | Alert `TWAI_ALERT_ERR_PASS`; chỉ error frame thụ động |
| BUS-OFF | TEC ≥ 256 | Alert `TWAI_ALERT_BUS_OFF`; ngừng hẳn cho tới khi hồi phục |

- **TX/RX error counter** — hai bộ đếm lỗi chuẩn của CAN; tăng theo loại lỗi, giảm khi truyền thành công.
- **Arbitration lost** (`arb_lost_count`) — đếm số lần thua arbitration (cơ chế bình thường, không phải "lỗi cần sửa").
- **Bus error** (`bus_error_count`) — số lần lỗi vật lý (bit/stuff/CRC/form/ACK).
- `msgs_to_tx` / `msgs_to_rx` — lượng message chờ trong queue → thấy áp lực driver.

## 📝 Kết quả thực tế của tôi

```text
<!-- DÁN [STATUS] 3 KỊCH BẢN VÀO ĐÂY -->
```

Bảng tóm tắt:

| Kịch bản | state | tx_err | rx_err | arb_lost | bus_err | tx_failed |
|---|---|---|---|---|---|---|
| 2 node khỏe | | | | | | |
| Mất Node 2 | | | | | | |
| Đảo dây | | | | | | |
| Sau hồi phục | | | | | | |

## ✅ Kết luận
- [ ] PASS — đọc hiểu counters và mô tả được diễn biến 3 kịch bản
- [ ] FAIL — mô tả: ______________________

## 🧠 Kiến thức rút ra
- Counters là "hộp đen" chuẩn cho chẩn đoán lớp vật lý CAN.
- Phân biệt: arbitration lost = hành vi bình thường; bus error/bus-off = vấn đề thật.

---

# LAB 10 – Mini ECU Project

## 🎯 Mục tiêu
Tổng hợp toàn bộ: Node 1 mô phỏng ECU gửi 3 tín hiệu, Node 2 phân loại theo ID và decode ra giá trị.

## 🔧 Chuẩn bị
- **Node bật:** Cả 2 node.
- **Phần cứng:** Như LAB 1.
- **Code:** Viết mới `loop()` 2 node (theo Level bên dưới).

## 📊 Giao thức message (dùng chung)

| CAN ID | Ý nghĩa | DLC | Data format |
|---|---|---|---|
| `0x100` | RPM động cơ | 2 | 2 byte little-endian (ví dụ `09 C4` = 0x09C4 = 2500 RPM) |
| `0x101` | Nhiệt độ | 2 | byte0 = giá trị + offset 40 (75°C → byte0 = 115 = 0x73) |
| `0x102` | Trạng thái | 1 | 0 = OFF, 1 = IDLE, 2 = RUNNING, 3 = ERROR |

- **Encode (Node 1):** từ giá trị vật lý → byte theo format.
- **Decode (Node 2):** theo ID → đọc byte → tính giá trị vật lý.

## 🖥️ Output mong đợi Node 2

```text
[RX] ID: 0x100  RPM: 2500
[RX] ID: 0x101  Temperature: 75 C
[RX] ID: 0x102  Status: RUNNING
```

## 📝 Các bước thực hiện (theo 4 Level)

### Level 1 — Tự làm (bắt buộc thử trước)
[ ] Tự viết code cả 2 node. Gợi ý tối thiểu: dùng bảng ID–format ở trên; Node 2 dùng `switch (msg.identifier)`.
[ ] Chạy thử và ghi kết quả (đúng/sai từng dòng) vào mục kết quả

### Level 2 — Gợi ý (mở ra sau khi tự thử)
- [ ] Gợi ý 1: RPM = `data[0] | (data[1] << 8)` (little-endian).
- [ ] Gợi ý 2: Temp = `data[0] - 40`.
- [ ] Gợi ý 3: Status dùng mảng tên: `{"OFF","IDLE","RUNNING","ERROR"}` + `data[0]` làm index (kiểm tra giới hạn!).
- [ ] Thử lại với mấy giá trị âm (byte0 < 40) để chắc chắn decode đúng.

### Level 3 — Code mẫu (mở ra khi bí)
```cpp
// ——— Node 1 (trích) ———
twai_message_t msg = {};
switch (frameCase % 3) {
  case 0: msg.identifier = 0x100; msg.data_length_code = 2;
          uint16_t rpm = 2500; msg.data[0] = rpm & 0xFF; msg.data[1] = rpm >> 8; break;
  case 1: msg.identifier = 0x101; msg.data_length_code = 2;
          int8_t t = 75; msg.data[0] = t + 40; break;
  case 2: msg.identifier = 0x102; msg.data_length_code = 1;
          msg.data[0] = 2; break;        // RUNNING
}
frameCase++;
twai_transmit(&msg, pdMS_TO_TICKS(100));

// ——— Node 2 (trích) ———
switch (msg.identifier) {
  case 0x100: Serial.printf("[RX] ID: 0x100  RPM: %u\n",
                            (uint16_t)(msg.data[0] | (msg.data[1] << 8))); break;
  case 0x101: Serial.printf("[RX] ID: 0x101  Temperature: %d C\n",
                            (int8_t)msg.data[0] - 40); break;
  case 0x102: { const char* st[] = {"OFF","IDLE","RUNNING","ERROR"};
                uint8_t i = msg.data[0]; if (i > 3) i = 3;
                Serial.printf("[RX] ID: 0x102  Status: %s\n", st[i]); } break;
  default:    Serial.printf("[RX] ID: 0x%03X  Unknown ID\n", msg.identifier);
}
```

### Level 4 — Kiểm tra & sửa lỗi
[ ] Dán code + output của bạn vào kết quả → nhờ review, sửa lỗi theo góp ý
[ ] Chạy lại cho tới khi 3 dòng output đúng

## 📝 Kết quả thực tế của tôi

**Output Node 2:**
```text
<!-- DÁN OUTPUT VÀO ĐÂY -->
```

**Code cuối cùng:**
```cpp
<!-- DÁN CODE ĐÃ CHẠY THÀNH CÔNG (nếu có) -->
```

## ✅ Kết luận
- [ ] PASS — 3 ID decode đúng giá trị
- [ ] FAIL — chưa xong, cần hỗ trợ (liệt kê lỗi: ________________)

## 🧠 Kiến thức rút ra
- Trên một bus: **ID = "chủ đề"**, **Data = "giá trị"** theo format đã thỏa thuận giữa 2 bên.
- Decode phải dựa vào ID trước, sau đó mới biết cách diễn giải byte.

---

## 14. Bảng tổng hợp kiến thức (tự đánh dấu khi đã test)

| Khái niệm | Đã test? | Ý nghĩa |
|---|---|---|
| CAN ID | [ ] | Định danh frame; standard 11-bit (0x000–0x7FF) |
| Standard Frame | [ ] | Frame chuẩn 11-bit ID (vs Extended 29-bit) |
| DLC | [ ] | Số byte Data theo frame (0–8) |
| Data | [ ] | Payload 8 byte truyền nguyên vẹn |
| Baudrate | [ ] | 500 kbps — toàn mạng phải đồng nhất |
| CANH | [ ] | Dây high cặp vi sai (dominant: cao) |
| CANL | [ ] | Dây low cặp vi sai (dominant: thấp) |
| ACK | [ ] | Vùng ACK trong frame; xác nhận từ node khác |
| Arbitration | [ ] | ID thấp thắng; mất arbitration là bình thường |
| CAN Filter | [ ] | Lọc ID trong controller trước RX queue |
| TX Queue | [ ] | Hàng đợi driver; ESP_OK ≠ lên bus |
| Error State | [ ] | Active/Warning/Passive theo counters (IDF 4.4: không có enum riêng) |
| Bus-Off | [ ] | TEC ≥ 256, ngừng tham gia bus |
| Termination | [ ] | 120Ω 2 đầu, đo ≈ 60Ω (nguồn tắt) |

## 15. Troubleshooting — Node 1 "gửi OK" nhưng Node 2 không nhận

Đi theo thứ tự này:

- [ ] **Nguồn:** Transceiver 2 node đủ VCC đúng loại (3.3V/5V)?
- [ ] **GND:** 2 node chung GND?
- [ ] **TX/RX:** GPIO 5 = TX, GPIO 4 = RX ở CẢ 2 node, không đảo?
- [ ] **CANH/CANL:** CANH→CANH, CANL→CANL (không đảo)?
- [ ] **Baudrate:** 2 node cùng `TWAI_TIMING_CONFIG_500KBITS`?
- [ ] **Transceiver:** TXD/RXD cắm đúng chiều? RXD 5V (TJA1050) cần mạch tương thích 3.3V?
- [ ] **Termination:** 120Ω × 2 đầu? (đo CANH–CANL ≈ 60Ω, nguồn tắt)
- [ ] **TWAI driver:** `[INIT] twai_driver_install OK` và `[INIT] twai_start OK` ở cả 2 node?
- [ ] **CAN ID:** Node 2 có filter đang chặn ID đang gửi? (Accept-All thì luôn nhận được ID gửi đúng chuẩn)
- [ ] **DLC:** Node 2 có in theo `data_length_code` (không hard-code 8)?
- [ ] **CAN Bus state:** Node 1 có `TX_FAILED`? counters tăng? → vấn đề lớp vật lý.
- [ ] **Error counter:** `tx_err`/`bus_err` tăng → kiểm tra dây/termination/nguồn transceiver.
- [ ] **Serial:** Mở đúng cổng của đúng node chưa? (2 monitor nhầm cổng rất phổ biến)

**Lịch sử lỗi đã gặp:**

| Ngày | Hiện tượng | Nguyên nhân | Cách sửa |
|---|---|---|---|
| | | | |
| | `TWAI_FILTER_CONFIG_SINGLE(STD_MSG, 0)` không tồn tại → compile error | Macro chỉ có từ ESP-IDF v5.x; framework đang dùng là v4.4.7 | Dùng `TWAI_FILTER_CONFIG_ACCEPT_ALL()` rồi ghi đè `acceptance_code = 0x036 << 21; acceptance_mask = 0x7FF << 21;` |
| | Build lỗi `invalid conversion from 'int' to 'gpio_num_t'` | Arduino-ESP32 core 3.x yêu cầu `gpio_num_t` | Cast `(gpio_num_t)` trong define chân GPIO |

## 16. Tổng kết luồng kiến thức

```text
ESP32
  └ TWAI (driver/twai.h — bộ điều khiển CAN trong chip)
      └ CAN Controller (timing, filter, arbitration, error counters)
          └ CAN Transceiver (logic 3.3V ↔ vi sai)
              └ CANH/CANL (vi sai; dominant +2V, recessive 0V hiệu)
                  └ CAN Frame (Arbitration, Control, Data, CRC, ACK, EOF)
                      └ ID (11-bit định danh + ưu tiên)
                          └ DLC (số byte data 0–8)
                              └ Data (payload)
                                  └ Arbitration (thấp ID thắng)
                                      └ ACK (node khác xác nhận)
                                          └ Error (TEC/REC → warning/passive/bus-off)
                                              └ Filter (lọc ID trong controller)
                                                  └ ECU communication (ID = chủ đề, Data = giá trị)
```

## 17. Bài tập tự luyện (tự làm, chưa cho đáp án — hỏi khi tắc)

1. **Đổi CAN ID (dễ):** Node 1 gửi `0x42A`; đoán trước + xác nhận ID Node 2 in.
2. **Đổi Data (dễ):** Node 1 gửi data cố định `DE AD BE EF CA FE BA BE`; Node 2 nhận đủ 8 byte?
3. **Đổi DLC (dễ):** Node 1 gửi DLC=3, data `AA BB CC`; Node 2 in đúng 3 byte?
4. **Filter (TB):** Node 2 chỉ nhận `0x200`; Node 1 gửi xen kẽ 0x100/0x200/0x300 → chỉ 0x200 lọt?
5. **Decode (TB):** `0x100` mang 2 byte RPM little-endian; Node 2 in RPM = 0x09C4 = 2500?
6. **Nhiều ID (TB):** Node 1 gửi 2 ID xen kẽ; Node 2 phân biệt ID và in kèm kiểu dữ liệu đúng.
7. **Mô phỏng ECU (khá):** `0x101` nhiệt độ offset −40; test giá trị âm (byte0 < 40) hiển thị đúng °C?
8. **Phát hiện lỗi (khá):** Node 1 một mình — ghi chuỗi counters tăng qua từng giây; phát hiện thời điểm ERR_PASS/BUS_OFF trong Serial.
9. **Thiết kế format (khó):** Tự thiết kế 1 byte = 2 tín hiệu 4 bit trong 1 ID; Node 2 tách 2 tín hiệu.
10. **Tổng hợp (khó):** 3 ID với tần suất khác nhau (0x100/100ms, 0x101/500ms, 0x102/2s); Node 2 thống kê frame/giây mỗi ID và phát hiện mất frame.

## 18. Roadmap học tiếp (dễ → khó)

1. CAN 2.0 chi tiết: bit timing (sample point, SJW), ảnh hưởng chiều dài cáp.
2. Extended frame 29-bit: `msg.extd = 1`, filter mask 29 bit.
3. CAN FD (DLC 0–64): TWAI ESP32 không hỗ trợ → cần MCP2517/2518 qua SPI.
4. Tầng ứng dụng: CANopen (PDO/SDO), J1939 (SPN/PGN).
5. OBD-II (SAE J1979): đọc dữ liệu xe thật kết hợp ESP32 + module OBD.
6. Đo lường: oscilloscope/logic analyzer đo CANH/CANL, so với registration của driver.
7. Công cụ: PCAN-View/CANalyzer, ESP32 làm CAN "sniffer" (mode LISTEN_ONLY).
8. RTOS: TX/RX trong FreeRTOS task riêng, đo jitter, xử lý burst frame.
9. Xử lý lỗi nâng cao: khôi phục bus-off tự động, NO_ACK mode, LISTEN_ONLY (tap không gây nhiễu).
10. Mạng đa node (3–5 node): quan sát arbitration thật khi 2 node gửi đồng thời.

---
*Sổ tay cập nhật xuyên suốt: mỗi lab xong thì tick bảng theo dõi đầu file + bảng kiến thức mục 14. Mọi khung `<!-- DÁN OUTPUT -->` chờ output thật của bạn.*