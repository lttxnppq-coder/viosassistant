#include <Arduino.h>

// TEST 07 - Motor Driver / H-Bridge (DRV8833 Parallel Mode)
// Phu hop: ESP32-S3
// Pin theo PinConfig.h: PIN_MOTOR_IN1=13, PIN_MOTOR_IN2=14
// API theo drivers/MotorDriver:
//   Config { in1=13, in2=14, pwm_freq=20000, pwm_resolution=10, ch in1=0, ch in2=1 }
//   setSpeed(signed -1000..+1000), brake(), coast()
//
// Note:
//  - Khong dung nSLEEP bang GPIO (PinConfig note: hardware-controlled / pending verification)
//  - Bo loi NFAULT doc tren GPIO7 (xung dot voi FAN_FET_PWM=7) cua test cu "Dong co"
//  - Experimental (giu lai): PWM_FREQ_HZ=20000, DIRECTION_CHANGE_DELAY_MS=75
//  - Production dung 10-bit (max 1023); test cu dung 8-bit -> nay theo MotorDriver
//
// KHONG upload quen khong tho! Day la testcase build/static.

#define PIN_IN1   13
#define PIN_IN2   14

#define PWM_FREQ_HZ        20000
#define PWM_RES_BITS       10
#define PWM_MAX_DUTY       ((1u << PWM_RES_BITS) - 1)

#define SPEED_MAX          1000
#define DIR_CHANGE_DELAY_MS 75   // experimental: giu tu test cu

String input = "";
int16_t current_speed = 0;

void motorApply(int16_t speed) {
    int16_t spd = constrain(speed, -SPEED_MAX, SPEED_MAX);

    if (spd > 0) {
        if (current_speed < 0) { coast(); delay(DIR_CHANGE_DELAY_MS); }
        uint16_t duty = map(abs(spd), 0, SPEED_MAX, 0, PWM_MAX_DUTY);
        ledcWrite(PIN_IN1, duty);
        ledcWrite(PIN_IN2, 0);
    } else if (spd < 0) {
        if (current_speed > 0) { coast(); delay(DIR_CHANGE_DELAY_MS); }
        uint16_t duty = map(abs(spd), 0, SPEED_MAX, 0, PWM_MAX_DUTY);
        ledcWrite(PIN_IN1, 0);
        ledcWrite(PIN_IN2, duty);
    } else {
        ledcWrite(PIN_IN1, 0);
        ledcWrite(PIN_IN2, 0);
    }
    current_speed = spd;
}

void coast() {
    ledcWrite(PIN_IN1, 0);
    ledcWrite(PIN_IN2, 0);
    current_speed = 0;
    Serial.printf("[MOTOR] COAST (IN1=0 IN2=0)\r\n");
}

void brake() {
    ledcWrite(PIN_IN1, PWM_MAX_DUTY);
    ledcWrite(PIN_IN2, PWM_MAX_DUTY);
    current_speed = 0;
    Serial.printf("[MOTOR] BRAKE (IN1=MAX IN2=MAX)\r\n");
}

void printStatus() {
    Serial.printf("[MOTOR] speed=%d/1000 IN1=%u IN2=%u\r\n",
                  current_speed,
                  PIN_IN1, PIN_IN2);
}

void printHelp() {
    Serial.println("[CMD] F<speed> : forward 0..1000");
    Serial.println("[CMD] B<speed> : backward 0..1000");
    Serial.println("[CMD] S        : stop (coast)");
    Serial.println("[CMD] BR       : brake");
    Serial.println("[CMD] ST       : status");
    Serial.println("[CMD] H        : help");
}

void setup() {
    Serial.begin(115200);
    delay(100);
    while (!Serial && millis() < 3000) {}

    Serial.println();
    Serial.println("=== TEST 07 : MOTOR / DRV8833 (Parallel) ===");
    Serial.printf("IN1=GPIO%u IN2=GPIO%u PWM=%uHz %u-bit\r\n",
                  PIN_IN1, PIN_IN2, PWM_FREQ_HZ, PWM_RES_BITS);

    pinMode(PIN_IN1, OUTPUT);
    pinMode(PIN_IN2, OUTPUT);
    digitalWrite(PIN_IN1, LOW);
    digitalWrite(PIN_IN2, LOW);

    if (!ledcAttach(PIN_IN1, PWM_FREQ_HZ, PWM_RES_BITS) ||
        !ledcAttach(PIN_IN2, PWM_FREQ_HZ, PWM_RES_BITS)) {
        Serial.println("[MOTOR] ERROR: ledcAttach failed");
        return;
    }
    ledcWrite(PIN_IN1, 0);
    ledcWrite(PIN_IN2, 0);
    Serial.println("[MOTOR] PWM OK - motor coasting");
    printHelp();
}

void loop() {
    if (Serial.available()) {
        input = Serial.readStringUntil('\n');
        input.trim();
        input.toUpperCase();
        if (input.length() == 0) return;

        char type = input.charAt(0);

        if ((type == 'F' || type == 'B') && input.length() > 1) {
            int16_t speed = (int16_t)input.substring(1).toInt();
            speed = constrain(speed, 0, SPEED_MAX);
            int16_t signed_speed = (type == 'F') ? speed : -speed;
            motorApply(signed_speed);
            Serial.printf("[MOTOR] %s speed=%d\r\n",
                          (signed_speed < 0) ? "BACKWARD" : "FORWARD", speed);
        } else if (input == "S") {
            coast();
        } else if (input == "BR") {
            brake();
        } else if (input == "ST") {
            printStatus();
        } else if (input == "H") {
            printHelp();
        } else {
            Serial.println("[MOTOR] unknown - type H");
        }
    }
}