// ==========================================================
// NODE 2 - UART RECEIVER (ESP32 DOIT DEVKIT V1, 38 chân)
// ----------------------------------------------------------
// Nhận packet 11 byte từ Node 1 qua UART2, state machine
// chờ 0xAA -> 0x55 -> nhận đủ 8 byte -> kiểm tra checksum
// -> decode và hiển thị đúng giá trị Node 1 gửi.
//
// UART2: TX = GPIO17, RX = GPIO16, 115200 baud, 8N1
// Không cộng dồn, không tự tăng giá trị.
// ==========================================================

#include <Arduino.h>

// ---------------- UART2 ----------------
#define UART_BAUD      115200
#define UART_RX_PIN    16
#define UART_TX_PIN    17

#define PACKET_SIZE    11

// ---------------- Format packet 11 byte ----------------
// Byte 0     = 0xAA
// Byte 1     = 0x55
// Byte 2-3   = Pedal (uint16_t little-endian)
// Byte 4-5   = RPM   (uint16_t little-endian)
// Byte 6-7   = Speed (uint16_t little-endian)
// Byte 8-9   = 0x00  0x00 (reserved)
// Byte 10    = Checksum (tổng byte 2->9, lấy 8 bit thấp)
#define HEADER_1 0xAA
#define HEADER_2 0x55

// Tính checksum: tổng byte 2 -> 9
static uint8_t calcChecksum(const uint8_t *packet) {
    uint8_t sum = 0;
    for (uint8_t i = 2; i <= 9; i++) {
        sum += packet[i];
    }
    return sum;
}

// State machine nhận packet
enum RxState {
    RX_WAIT_AA,     // chờ byte đầu 0xAA
    RX_WAIT_55,     // chờ byte 0x55
    RX_DATA,        // nhận 8 byte dữ liệu (index 2..9)
    RX_CHECKSUM     // nhận byte checksum
};

void setup() {
    Serial.begin(115200);
    delay(200);

    // UART2: RX = GPIO16, TX = GPIO17, 115200 8N1
    Serial2.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

    Serial.println("== NODE 2 - UART RECEIVER ==");
    Serial.printf("UART2: RX=GPIO%d TX=GPIO%d  %d baud 8N1\n",
                  UART_RX_PIN, UART_TX_PIN, UART_BAUD);
}

void loop() {
    static uint8_t rxPacket[PACKET_SIZE];
    static RxState state = RX_WAIT_AA;
    static uint8_t idx = 0;

    // Đọc TỪNG byte một (không giả định mỗi lần đọc đủ cả packet)
    while (Serial2.available() > 0) {
        uint8_t b = (uint8_t)Serial2.read();

        switch (state) {
            case RX_WAIT_AA:
                // Chờ byte đầu 0xAA
                if (b == HEADER_1) {
                    state = RX_WAIT_55;
                }
                break;

            case RX_WAIT_55:
                if (b == HEADER_2) {
                    idx = 2;            // bắt đầu lưu từ byte 2
                    state = RX_DATA;
                } else if (b != HEADER_1) {
                    state = RX_WAIT_AA; // không phải AA 55 -> tìm lại
                }
                // Gặp thêm 0xAA thì cứ chờ tiếp 0x55 (tự đồng bộ lại)
                break;

            case RX_DATA:
                rxPacket[idx++] = b;
                if (idx == 10) {
                    state = RX_CHECKSUM;   // đã đủ byte 2..9
                }
                break;

            case RX_CHECKSUM: {
                rxPacket[10] = b;

                uint8_t expected = calcChecksum(rxPacket);

                if (b == expected) {
                    // Checksum đúng -> decode little-endian
                    uint16_t pedal = rxPacket[2] | ((uint16_t)rxPacket[3] << 8);
                    uint16_t rpm   = rxPacket[4] | ((uint16_t)rxPacket[5] << 8);
                    uint16_t speed = rxPacket[6] | ((uint16_t)rxPacket[7] << 8);

                    Serial.println();
                    Serial.println("[UART RX]");
                    Serial.println("Checksum: OK");
                    Serial.println();
                    Serial.printf("Pedal: %u %%\n", (unsigned int)pedal);
                    Serial.printf("RPM: %u\n", (unsigned int)rpm);
                    Serial.printf("Speed: %u km/h\n", (unsigned int)speed);
                    Serial.println();
                    Serial.print("DATA: ");
                    for (uint8_t i = 0; i < PACKET_SIZE; i++) {
                        Serial.printf("%02X ", (unsigned int)rxPacket[i]);
                    }
                    Serial.println();
                } else {
                    // Checksum sai -> báo lỗi, KHÔNG decode dữ liệu lỗi
                    Serial.println();
                    Serial.println("[UART RX]");
                    Serial.println("Checksum ERROR");
                    Serial.printf("Received: %02X  Expected: %02X\n",
                                  (unsigned int)b, (unsigned int)expected);
                }

                state = RX_WAIT_AA;      // reset, chờ packet kế tiếp
                break;
            }
        }
    }
}
