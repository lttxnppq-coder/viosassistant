#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

// TEST 07 - Motor Driver / H-Bridge (DRV8833 Parallel Mode)

// OLED SSD1306 128x64 I2C 0x3C SDA8 SCL9
#define OLED_SDA 8
#define OLED_SCL 9
#define OLED_ADDR 0x3C
#define OLED_W 128
#define OLED_H 64
Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);
bool oled_ok = false;
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

void updateOled() {
    if (!oled_ok) return;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("MOTOR TEST");
    display.println("----------------");
    char buf[32];
    snprintf(buf, sizeof(buf), "CMD:%d", current_speed);
    display.println(buf);
    // PWM from current_speed
    int pwm = 0;
    if (current_speed != 0) pwm = map(abs(current_speed), 0, SPEED_MAX, 0, PWM_MAX_DUTY);
    snprintf(buf, sizeof(buf), "PWM:%d", pwm);
    display.println(buf);
    const char* dir = (current_speed > 0) ? "CW" : (current_speed < 0) ? "CCW" : "STOP";
    const char* state = (current_speed == 0) ? "STOP" : "RUN";
    snprintf(buf, sizeof(buf), "DIR:%s", dir);
    display.println(buf);
    display.print("STATE:");
    display.println(state);
    // IN1/IN2
    snprintf(buf, sizeof(buf), "IN1:%d IN2:%d", digitalRead(PIN_IN1), digitalRead(PIN_IN2));
    display.setCursor(0, 56);
    display.print(buf);
    display.display();
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
        display.println("MOTOR TEST");
        display.println("----------------");
        display.println("INIT");
        display.display();
        Serial.println("[OLED] INIT PASS");
    }

    if (!ledcAttach(PIN_IN1, PWM_FREQ_HZ, PWM_RES_BITS) ||
        !ledcAttach(PIN_IN2, PWM_FREQ_HZ, PWM_RES_BITS)) {
        Serial.println("[MOTOR] ERROR: ledcAttach failed");
        if (oled_ok) {
            display.clearDisplay();
            display.setCursor(0, 0);
            display.println("MOTOR TEST");
            display.println("ERROR: ledcAttach");
            display.display();
        }
        return;
    }
    ledcWrite(PIN_IN1, 0);
    ledcWrite(PIN_IN2, 0);
    Serial.println("[MOTOR] PWM OK - motor coasting");
    printHelp();
    updateOled();
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
            updateOled();
        } else if (input == "S") {
            coast();
            updateOled();
        } else if (input == "BR") {
            brake();
            updateOled();
        } else if (input == "ST") {
            printStatus();
            updateOled();
        } else if (input == "H") {
            printHelp();
        } else {
            Serial.println("[MOTOR] unknown - type H");
        }
    }
}