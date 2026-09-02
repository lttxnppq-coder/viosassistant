#include <Arduino.h>

// TEST 05 - Raspberry Pi UART (Application UART) - loopback test
// Phu hop: ESP32-S3 N16R8
// Pin theo PinConfig.h:
//   PIN_PI_UART_TX  17  (ESP32 TX -> Pi RX)
//   PIN_PI_UART_RX  18  (ESP32 RX <- Pi TX)
// Framing theo ProtocolConfig.h:
//   UART_BAUD_RATE = 115200
//   CMD_HEADER = 0xAA, CMD_FOOTER = 0x55, CRC16 poly 0x8005/init 0xFFFF
//
// Mode:
//   * Khi noi dong RS232 xuong -> TEST RX (do nhan du lieu tone/ky tu tu Pi)
//   * Khi nao co signal TEST_TX_ENABLE=1 -> gui frame AA + payload + CRC16 + 55
//
// KHONG upload quen khong tho! Day la testcase build/static.

#define PIN_UART_TX   17
#define PIN_UART_RX   18

#define UART_BAUD     115200

#define CMD_HEADER    0xAA
#define CMD_FOOTER    0x55
#define CMD_MAX_PAYLOAD  8
#define CRC16_POLY    0xA001       // little-endian reflection of 0x8005 (CCITT-style)
#define CRC16_INIT    0xFFFF

uint16_t crc16(const uint8_t* data, size_t len) {
    uint16_t crc = CRC16_INIT;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++) {
            if (crc & 0x0001) crc = (crc >> 1) ^ CRC16_POLY;
            else              crc >>= 1;
        }
    }
    return crc;
}

void sendTestFrame(uint8_t id) {
    uint8_t payload[CMD_MAX_PAYLOAD];
    for (uint8_t i = 0; i < CMD_MAX_PAYLOAD; i++) payload[i] = id + i;

    uint16_t crc = crc16(payload, CMD_MAX_PAYLOAD);

    Serial1.write(CMD_HEADER);
    Serial1.write(CMD_MAX_PAYLOAD);
    Serial1.write(id);
    Serial1.write(payload, CMD_MAX_PAYLOAD);
    Serial1.write(crc & 0xFF);
    Serial1.write((crc >> 8) & 0xFF);
    Serial1.write(CMD_FOOTER);

    Serial.printf("[UART-TX] frame id=0x%02X len=%u crc=0x%04X\r\n",
                  id, CMD_MAX_PAYLOAD, crc);
}

uint8_t test_seq = 0;
unsigned long last_tx = 0;
uint32_t rx_count = 0;

void setup() {
    Serial.begin(115200);
    delay(100);
    while (!Serial && millis() < 3000) {}

    Serial.println();
    Serial.println("=== TEST 05 : Pi UART (Serial1) ===");
    Serial.printf("TX=GPIO%u RX=GPIO%u baud=%u\r\n", PIN_UART_TX, PIN_UART_RX, UART_BAUD);

    // Serial1: begin(baud, SERIAL_8N1, rxPin, txPin)
    Serial1.begin(UART_BAUD, SERIAL_8N1, PIN_UART_RX, PIN_UART_TX);
    Serial.println("[UART] Serial1 init OK - loopback flag (RX/TX rear)");
}

void loop() {
    unsigned long now = millis();

    // gui ok frame dinh ky
    if (now - last_tx >= 2000) {
        last_tx = now;
        sendTestFrame(test_seq++);
    }

    // nhan va dem
    while (Serial1.available() > 0) {
        uint8_t b = Serial1.read();
        rx_count++;
        if (rx_count % 16 == 0) {
            Serial.printf("[UART-RX] total=%lu last=0x%02X\r\n",
                          rx_count, b);
        }
    }
}