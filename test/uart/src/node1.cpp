// ==========================================================
// NODE 1 - UART TRANSMITTER (ESP32 DOIT DEVKIT V1, 38 chân)
// ----------------------------------------------------------
// Đọc 3 biến trở mỗi 100 ms, map ADC 0-4095 sang giá trị
// thực, đóng gói packet 11 byte và gửi qua UART2.
//
// UART2: TX = GPIO17, RX = GPIO16, 115200 baud, 8N1
// Không dùng counter, không cộng dồn giá trị.
// ==========================================================

#include <Arduino.h>

// ---------------- Biến trở ----------------
#define POT_PEDAL_PIN  32   // Pedal  -> 0-100 %
#define POT_RPM_PIN    33   // RPM    -> 0-4000
#define POT_SPEED_PIN  34   // Speed  -> 0-200 km/h

// ---------------- UART2 ----------------
#define UART_BAUD      115200
#define UART_RX_PIN    16
#define UART_TX_PIN    17

#define PACKET_SIZE    11
#define TX_INTERVAL_MS 100UL   // gửi mỗi 100 ms

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

// Đóng gói 3 giá trị uint16_t vào packet (little-endian)
static void buildPacket(uint8_t *packet, uint16_t pedal, uint16_t rpm, uint16_t speed) {
    packet[0] = HEADER_1;
    packet[1] = HEADER_2;

    packet[2] = (uint8_t)(pedal & 0xFF);   // Pedal LOW
    packet[3] = (uint8_t)(pedal >> 8);     // Pedal HIGH

    packet[4] = (uint8_t)(rpm & 0xFF);     // RPM LOW
    packet[5] = (uint8_t)(rpm >> 8);       // RPM HIGH

    packet[6] = (uint8_t)(speed & 0xFF);   // Speed LOW
    packet[7] = (uint8_t)(speed >> 8);     // Speed HIGH

    packet[8] = 0x00;
    packet[9] = 0x00;

    packet[10] = calcChecksum(packet);
}

void setup() {
    Serial.begin(115200);
    delay(200);

    // UART2: RX = GPIO16, TX = GPIO17, 115200 8N1
    Serial2.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

    Serial.println("== NODE 1 - UART TRANSMITTER ==");
    Serial.printf("UART2: RX=GPIO%d TX=GPIO%d  %d baud 8N1\n",
                  UART_RX_PIN, UART_TX_PIN, UART_BAUD);
    Serial.println("Pots:  GPIO32 (Pedal)  GPIO33 (RPM)  GPIO34 (Speed)");
}

void loop() {
    static unsigned long lastTx = 0;

    // Gửi mỗi 100 ms bằng millis (không delay dài, không counter)
    if (millis() - lastTx >= TX_INTERVAL_MS) {
        lastTx = millis();

        // Đọc ADC (0-4095) và map sang đơn vị thực
        uint16_t pedal = (uint16_t)map(analogRead(POT_PEDAL_PIN), 0, 4095, 0, 100);
        uint16_t rpm   = (uint16_t)map(analogRead(POT_RPM_PIN),   0, 4095, 0, 4000);
        uint16_t speed = (uint16_t)map(analogRead(POT_SPEED_PIN), 0, 4095, 0, 200);

        uint8_t packet[PACKET_SIZE];
        buildPacket(packet, pedal, rpm, speed);

        // In Serial Monitor trước khi gửi
        Serial.println();
        Serial.println("[UART TX]");
        Serial.printf("Pedal: %u %%\n", (unsigned int)pedal);
        Serial.printf("RPM: %u\n", (unsigned int)rpm);
        Serial.printf("Speed: %u km/h\n", (unsigned int)speed);
        Serial.print("DATA: ");
        for (uint8_t i = 0; i < PACKET_SIZE; i++) {
            Serial.printf("%02X ", (unsigned int)packet[i]);
        }
        Serial.println();

        // Gửi 11 byte qua UART2
        Serial2.write(packet, PACKET_SIZE);
    }
}
