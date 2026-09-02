#include <Arduino.h>

// TEST 04 - GPIO INPUT (ON/OFF)
// Phu hop: ESP32-S3 N16R8 CH343P
// Muc dich: doc trang thai cua PIN_ON_OFF_INPUT (GPIO10)
//
// NOTE PRODUCTION:
//  - PinConfig.h ghi "Input mode (INPUT, INPUT_PULLUP, INPUT_PULLDOWN) TBD
//    - hardware confirmation needed". KHONG sua PinConfig.
//  - Test nay cho phep chon mode mac dinh qua INPUT_MODE define.
//
// KHONG upload quen khong tho! Day la testcase build/static.

#ifndef INPUT_MODE
#define INPUT_MODE INPUT_PULLUP   // hardcode tam - can xac nhan phan cung
#endif

#define PIN_ON_OFF 10

#define POLL_MS 100UL
#define MIN_REPORT_MS 500UL

unsigned long last_report = 0;

void setup() {
    Serial.begin(115200);
    delay(100);
    while (!Serial && millis() < 3000) {}

    Serial.println();
    Serial.println("=== TEST 04 : GPIO INPUT (ON/OFF) ===");
    pinMode(PIN_ON_OFF, OUTPUT);
    digitalWrite(PIN_ON_OFF, LOW);
    delay(10);
    pinMode(PIN_ON_OFF, INPUT_MODE);  // che do input tu PinConfig note

    Serial.printf("[GPIO-IN] PIN_ON_OFF = GPIO%u | mode = %d\r\n",
                  PIN_ON_OFF, (int)INPUT_MODE);
    Serial.println("[NOTE] Input mode TBD in PinConfig - verify wiring before hardware run");
}

void loop() {
    unsigned long now = millis();
    int level = digitalRead(PIN_ON_OFF);

    if (now - last_report >= MIN_REPORT_MS) {
        last_report = now;
        Serial.printf("[GPIO-IN] GPIO%u level = %d (%s)\r\n",
                      PIN_ON_OFF, level, level ? "HIGH/ON" : "LOW/OFF");
    }
}