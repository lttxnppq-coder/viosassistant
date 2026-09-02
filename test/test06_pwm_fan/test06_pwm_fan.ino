#include <Arduino.h>

// TEST 06 - PWM / Fan
// Phu hop: ESP32-S3
// Pin theo PinConfig.h: PIN_FAN_FET_PWM = 7
// API theo drivers/PwmDriver: ledcAttach(pin, freq, resolution) - core 3.x
//   ChannelConfig { pin=7, freq=1000, resolution=8 }
//
// NOTE:
//  - Motor cung dung LEDC 20kHz/10-bit (xem TEST 07) - hai driver dung LEDC
//    rieng nhu nhau trong core 3.x (ledcAttach duoc gan channel noi bo).
//  - Khi chay dong tho: Fan Relay (GPIO5, TEST03) phai ON de cap nguon quat,
//    FET nay chi dieu che PWM toc do.
//
// KHONG upload quen khong tho! Day la testcase build/static.

#define PIN_FAN_FET  7

#define PWM_FREQ_HZ      1000
#define PWM_RES_BITS     8

#define STEP_MS       500UL
#define DUTY_MAX      255U

uint8_t duty = 0;
bool rising = true;
unsigned long last_step = 0;

void setup() {
    Serial.begin(115200);
    delay(100);
    while (!Serial && millis() < 3000) {}

    Serial.println();
    Serial.println("=== TEST 06 : PWM / FAN (GPIO7) ===");
    Serial.printf("ledcAttach(GPIO%u, %uHz, %u-bit)\r\n",
                  PIN_FAN_FET, PWM_FREQ_HZ, PWM_RES_BITS);

    if (!ledcAttach(PIN_FAN_FET, PWM_FREQ_HZ, PWM_RES_BITS)) {
        Serial.println("[PWM] ERROR: ledcAttach failed");
        return;  // setup exit -> loop khong lam gi (build/static ok)
    }
    ledcWrite(PIN_FAN_FET, 0);
    Serial.println("[PWM] ledcAttach OK, initial duty=0");
}

void loop() {
    unsigned long now = millis();
    if (now - last_step < STEP_MS) return;
    last_step = now;

    if (rising) {
        duty += 17;   // ~0-255 ramp len
        if (duty >= DUTY_MAX) { duty = DUTY_MAX; rising = false; }
    } else {
        duty -= 17;   // ramp xuong
        if (duty <= 0) { duty = 0; rising = true; }
    }

    ledcWrite(PIN_FAN_FET, duty);
    Serial.printf("[PWM] GPIO%u duty=%u (%u%%)\r\n",
                  PIN_FAN_FET, duty, (unsigned)(duty * 100UL / DUTY_MAX));
}