#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

// TEST 09 - CAN Module UART (UART-to-CAN bridge)

// OLED SSD1306 128x64 I2C 0x3C SDA8 SCL9
#define OLED_SDA 8
#define OLED_SCL 9
#define OLED_ADDR 0x3C
#define OLED_W 128
#define OLED_H 64
Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);
bool oled_ok = false;
// Phu hop: ESP32-S3
// Pin theo PinConfig.h: PIN_CAN_UART_TX=11, PIN_CAN_UART_RX=12
//
// NOTE PRODUCTION (bao cao, khong sua code):
//  - drivers/CanDriver.cpp DANG LA STUB:
//      begin()       -> chi set initialized_=true (khong mo UART)
//      write()       -> tra initialized_ chi la true
//      read()        -> luon false
//      available()   -> luon 0
//    => Khong co thuc thi UART/CAN that su.
//  - Test nay kiem tra TUYEN UART toi module (Serial2 GPIO11/12) mot cach doc lap
//    truoc khi CanDriver duoc hoan thien.
//
// KHONG upload quen khong tho! Day la testcase build/static.

#define CAN_UART_TX   11
#define CAN_UART_RX   12
#define CAN_UART_BAUD 500000   // UART sang CAN module (KHONG bang CAN bus bitrate)

#define HEADER 0xAA
#define FOOTER 0x55
#define MAX_DLC 8
#define FRAME_MAX_SIZE  (1 + 1 + 4 + MAX_DLC + 2 + 1)

uint16_t crc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++) {
            if (crc & 0x0001) crc = (crc >> 1) ^ 0xA001;
            else              crc >>= 1;
        }
    }
    return crc;
}

void sendCanFrame(uint32_t id, const uint8_t* data, uint8_t dlc, bool extended) {
    uint8_t frame[FRAME_MAX_SIZE];
    size_t i = 0;
    frame[i++] = HEADER;
    frame[i++] = (extended ? 0x81 : 0x01) | (dlc & 0x0F);
    frame[i++] = id & 0xFF;
    frame[i++] = (id >> 8) & 0xFF;
    frame[i++] = (id >> 16) & 0xFF;
    frame[i++] = (id >> 24) & 0xFF;
    uint8_t d = dlc > MAX_DLC ? MAX_DLC : dlc;
    for (uint8_t k = 0; k < d; k++) frame[i++] = data[k];
    uint16_t crc = crc16(frame, i);
    frame[i++] = crc & 0xFF;
    frame[i++] = (crc >> 8) & 0xFF;
    frame[i++] = FOOTER;

    Serial2.write(frame, i);
    Serial.printf("[CAN-TX] id=0x%03X dlc=%u ext=%u frame=%uB crc=0x%04X\r\n",
                  (unsigned)id, d, extended ? 1 : 0, (unsigned)i, crc);
}

uint8_t seq = 0;
unsigned long last_send = 0;
uint32_t rx_count = 0;

void updateOled() {
    if (!oled_ok) return;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("CAN UART");
    display.println("----------------");
    char buf[32];
    snprintf(buf, sizeof(buf), "TX:OK RX:%s", rx_count>0 ? "OK" : "N/A");
    display.println(buf);
    if (rx_count>0) snprintf(buf, sizeof(buf), "RX:%lu", (unsigned long)rx_count);
    else snprintf(buf, sizeof(buf), "RX:N/A");
    display.println(buf);
    snprintf(buf, sizeof(buf), "LAST ID:0x%03X", (unsigned)seq);
    display.println(buf);
    display.println("BAUD:500k");
    display.setCursor(0, 56);
    display.print("11/12 UART");
    display.display();
}

void setup() {
    Serial.begin(115200);
    delay(100);
    while (!Serial && millis() < 3000) {}

    Serial.println();
    Serial.println("=== TEST 09 : CAN MODULE UART (Serial2 11/12) ===");
    Serial.printf("TX=GPIO%u RX=GPIO%u baud=%u\r\n",
                  CAN_UART_TX, CAN_UART_RX, CAN_UART_BAUD);

    // OLED init
    Wire.begin(OLED_SDA, OLED_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        oled_ok = false;
        Serial.println("[OLED] INIT FAIL");
    } else {
        oled_ok = true;
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("CAN UART");
        display.println("----------------");
        display.println("INIT");
        display.display();
        Serial.println("[OLED] INIT PASS");
    }

    Serial2.begin(CAN_UART_BAUD, SERIAL_8N1, CAN_UART_RX, CAN_UART_TX);
    Serial.println("[CAN] Serial2 init OK (raw UART to CAN module)");
    Serial.println("[CAN] PRODUCTION NOTE: CanDriver.cpp is a STUB - see file comments");
}

unsigned long last_oled = 0;
void loop() {
    unsigned long now = millis();
    if (now - last_send >= 2000) {
        last_send = now;
        uint8_t payload[8];
        for (uint8_t k = 0; k < 8; k++) payload[k] = seq + k;
        sendCanFrame(0x123, payload, 8, false);  // can id nho
        if ((seq & 0x03) == 0) {
            sendCanFrame(0x1FFFFFFF, payload, 8, true);  // id mo rong
        }
        seq++;
    }

    while (Serial2.available() > 0) {
        uint8_t b = Serial2.read();
        rx_count++;
        if (rx_count % 16 == 0) {
            Serial.printf("[CAN-RX] total=%lu last=0x%02X\r\n", rx_count, b);
        }
    }
    if (now - last_oled >= 300) {
        last_oled = now;
        updateOled();
    }
}