#include <Arduino.h>
#include "PinConfig.h"

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
    // compile/static chi - khong bao cao moi khi pins TBD
    delay(1000);
}