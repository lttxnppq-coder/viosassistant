#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include "PinConfig.h"

// OLED SSD1306 128x64 I2C 0x3C SDA8 SCL9
#define OLED_SDA 8
#define OLED_SCL 9
#define OLED_ADDR 0x3C
#define OLED_W 128
#define OLED_H 64
Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);
bool oled_ok = false;
unsigned long last_oled = 0;

// TEST 08 - Encoder GA25 quadrature (compile/static verification)
// Phu hop: ESP32-S3 N16R8 CH343P
//
// User decision 2026-08 (confirmed):
//  - ENCODER_A = GPIO19 (PIN_ENC_A), ENCODER_B = GPIO20 (PIN_ENC_B) - GA25 quadrature
//  - PIN_ENC_BTN not used (GA25 has no push button)
//  - GPIO19/20 are USB-OTG D-/D+ on generic S3, but on this board USB is via CH343P (43/44),
//    so 19/20 re-purposed for encoder by user decision.
//    PRODUCTION/HARDWARE CONFLICT: if USB-OTG is cabled, 19/20 overlap - user confirmed assignment.
//  - Test nay CHI compile/static; chua test hardware.
//  - KHONG tu doi GPIO khac.
//
// Experimental params giu lai tu test "Dieukhiendongcoencoder":
//  - PULSES_PER_REV = 11300.0 (with note: o che do CHANGE ~204/lang doc lai)
//  - dem xung voi ISR CHANGE, tang giam theo logic 2 kenh
//  - KHONG upload/quen khong tho! Day la testcase build/static.

#ifndef ENCODER_PIN_A
#define ENCODER_PIN_A   PIN_ENC_A   // GPIO19 - user decision
#endif
#ifndef ENCODER_PIN_B
#define ENCODER_PIN_B   PIN_ENC_B   // GPIO20 - user decision
#endif
#ifndef ENCODER_PIN_BTN
#define ENCODER_PIN_BTN -1          // not used - GA25 no button
#endif

// NOTE: o che do CHANGE, so xung gap doi (test 1 vong ~204 => dien 11300.0/204.0 tuy thuoc)
// Nguon thuc nghiem: test "Dieukhiendongcoencoder" - giu nguyen
const float PULSES_PER_REV = 11300.0;

static volatile long pulseCount = 0;

#if (ENCODER_PIN_A >= 0) && (ENCODER_PIN_B >= 0)
void IRAM_ATTR readEncoder() {
    int stateA = digitalRead(ENCODER_PIN_A);
    int stateB = digitalRead(ENCODER_PIN_B);
    if (stateA == stateB) {
        pulseCount--;
    } else {
        pulseCount++;
    }
}
#endif

void updateOled() {
    if (!oled_ok) return;
    if (millis() - last_oled < 200) return;
    last_oled = millis();
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("ENCODER TEST");
    display.println("----------------");
    char buf[32];
    // Read current levels
    int a = digitalRead(ENCODER_PIN_A);
    int b = digitalRead(ENCODER_PIN_B);
    snprintf(buf, sizeof(buf), "A:%d B:%d", a, b);
    display.println(buf);
    snprintf(buf, sizeof(buf), "COUNT:%ld", pulseCount);
    display.println(buf);
    // Direction from last change
    static long lastCount = 0;
    long delta = pulseCount - lastCount;
    const char* dir = (delta > 0) ? "CW" : (delta < 0) ? "CCW" : "STOP";
    lastCount = pulseCount;
    snprintf(buf, sizeof(buf), "DIR:%s", dir);
    display.println(buf);
    display.println("PPR:11300");
    // Show deg
    float deg = (pulseCount / PULSES_PER_REV) * 360.0f;
    snprintf(buf, sizeof(buf), "POS:%.1fdeg", deg);
    display.setCursor(0, 56);
    display.print(buf);
    display.display();
}

void setup() {
    Serial.begin(115200);
    delay(100);
    while (!Serial && millis() < 3000) {}

    Serial.println();
    Serial.println("=== TEST 08 : ENCODER GA25 quadrature (compile/static) ===");
    Serial.printf("ENC_A=%d ENC_B=%d ENC_BTN=%d\r\n",
                  ENCODER_PIN_A, ENCODER_PIN_B, ENCODER_PIN_BTN);
    Serial.printf("[ENC] PinConfig: PIN_ENC_A=%d PIN_ENC_B=%d\r\n", PIN_ENC_A, PIN_ENC_B);
    Serial.println("[ENC] USER DECISION: GA25 quadrature A=GPIO19 B=GPIO20 (confirmed)");
    Serial.println("[ENC] NOTE: GPIO19/20 = USB-OTG on generic S3, CH343P board uses 43/44 for USB -> conflict accepted");
    Serial.println("[ENC] PULSES_PER_REV=11300 (chan doi xung) - giu nguyen thuc nghiem, chua do lai");

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
        display.println("ENCODER TEST");
        display.println("----------------");
        display.println("INIT");
        display.display();
        Serial.println("[OLED] INIT PASS");
    }

#if (ENCODER_PIN_A < 0) || (ENCODER_PIN_B < 0)
    Serial.println("[ENC] STATUS: PINS TBD - NOT WIRED");
    Serial.println("[ENC] This testcase is compile/static only.");
#else
    pinMode(ENCODER_PIN_A, INPUT_PULLUP);
    pinMode(ENCODER_PIN_B, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), readEncoder, CHANGE);
    Serial.println("[ENC] ISR attached (CHANGE mode) - HARDWARE: NOT EXECUTED");
#endif
}

void loop() {
    updateOled();
    delay(10);
}