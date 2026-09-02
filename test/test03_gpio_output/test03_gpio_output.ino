#include <Arduino.h>

// TEST 03 - GPIO OUTPUT / Relay
// Phu hop: ESP32-S3 N16R8 CH343P
// Muc dich: xu ly tat ca output don gian (Relay/FET) theo PinConfig.h
//
// KHONG test PWM tai day (xem TEST 06 PWM/Fan).
// KHONG dieu khien Motor tai day (xem TEST 07 Motor Driver).
// KHONG upload quen khong tho! Day la testcase build/static.

#define PIN_AC_RELAY        4
#define PIN_FAN_RELAY       5
#define PIN_PI_POWER_RELAY  6
#define PIN_FAN_FET_PWM     7

#define STEP_MS       1000UL
#define CYCLE_WAIT_MS 4000UL

const uint8_t kOutputPins[] = {
    PIN_AC_RELAY,
    PIN_FAN_RELAY,
    PIN_PI_POWER_RELAY,
    PIN_FAN_FET_PWM,
};
const char* kOutputNames[] = {
    "AC_RELAY",
    "FAN_RELAY",
    "PI_POWER_RELAY",
    "FAN_FET_PWM",
};
const size_t kOutputCount = sizeof(kOutputPins) / sizeof(kOutputPins[0]);

uint8_t step = 0;
unsigned long last_toggle = 0;
unsigned long cycle_start = 0;

void setup() {
    Serial.begin(115200);
    delay(100);
    while (!Serial && millis() < 3000) {}

    Serial.println();
    Serial.println("=== TEST 03 : GPIO OUTPUT / RELAY ===");
    Serial.printf("ESP32-S3 | outputs: %u\r\n", (unsigned)kOutputCount);

    for (size_t i = 0; i < kOutputCount; i++) {
        pinMode(kOutputPins[i], OUTPUT);
        digitalWrite(kOutputPins[i], LOW);
    }
    Serial.println("[GPIO-OUT] All outputs initialized LOW (safe state)");

    // Ghi chu an toan nguon (khong thuc thi tren bo mang):
    //  AC_RELAY / FAN_RELAY / PI_POWER_RELAY dieu khien nguon AC/DC
    //  FAN_FET_PWM o day chi dung nhu output ON/OFF (test pwm xem TEST06)
    Serial.println("[NOTE] Relays switch AC/DC power - ensure safe load before hardware run");
}

void setAllLow() {
    for (size_t i = 0; i < kOutputCount; i++) {
        digitalWrite(kOutputPins[i], LOW);
    }
}

void printStates() {
    for (size_t i = 0; i < kOutputCount; i++) {
        Serial.printf("  %-14s GPIO%2u : %s\r\n",
                      kOutputNames[i],
                      kOutputPins[i],
                      digitalRead(kOutputPins[i]) ? "HIGH" : "LOW");
    }
}

void loop() {
    unsigned long now = millis();

    if (now - last_toggle >= STEP_MS) {
        last_toggle = now;

        if (step >= kOutputCount) {
            // cuoi chu ky: tat het, cho 1 nhin
            setAllLow();
            Serial.println("---- CYCLE COMPLETE: all outputs LOW ----");
            step = 0;
            cycle_start = now;  // bat dau chu ky moi
        }

        if (now - cycle_start >= CYCLE_WAIT_MS) {
            // bat tuong tu 1 output
            setAllLow();
            digitalWrite(kOutputPins[step], HIGH);
            Serial.printf("[%u] %s ON (GPIO%u)\r\n",
                          (unsigned)(step + 1), kOutputNames[step], kOutputPins[step]);
            printStates();
            step++;
        }
    }
}