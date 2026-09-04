#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

// TEST 04 - GPIO INPUT (ON/OFF)
// Phu hop: ESP32-S3 N16R8 CH343P
// Muc dich: doc trang thai cua PIN_ON_OFF_INPUT (GPIO10)

// OLED SSD1306 128x64 I2C 0x3C SDA8 SCL9
#define OLED_SDA 8
#define OLED_SCL 9
#define OLED_ADDR 0x3C
#define OLED_W 128
#define OLED_H 64
Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);
bool oled_ok = false;
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
        display.println("INPUT TEST");
        display.println("----------------");
        display.print("PIN: GPIO");
        display.println(PIN_ON_OFF);
        display.display();
        Serial.println("[OLED] INIT PASS");
    }
}

void updateOled(int level) {
    if (!oled_ok) return;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("INPUT TEST");
    display.println("----------------");
    display.print("PIN: GPIO");
    display.println(PIN_ON_OFF);
    display.println("");
    display.print("STATE:");
    display.println(level ? "ON" : "OFF");
    display.print(level ? "HIGH" : "LOW");
    display.print(" (");
    display.print(level);
    display.println(")");
    // Show raw
    char buf[32];
    snprintf(buf, sizeof(buf), "GPIO10:%d", level);
    display.setCursor(0, 56);
    display.print(buf);
    display.display();
}

void loop() {
    unsigned long now = millis();
    int level = digitalRead(PIN_ON_OFF);

    if (now - last_report >= MIN_REPORT_MS) {
        last_report = now;
        Serial.printf("[GPIO-IN] GPIO%u level = %d (%s)\r\n",
                      PIN_ON_OFF, level, level ? "HIGH/ON" : "LOW/OFF");
        updateOled(level);
    }
}