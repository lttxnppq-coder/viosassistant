// #include <Arduino.h>

// // --- Định nghĩa chân theo sơ đồ Schematic.pdf ---
// #define EN_A  19//16
// #define EN_B  20//17 
// #define MOTOR_PWM 40
// #define MOTOR_IN1 13  //38  
// #define MOTOR_IN2 14 //39

// // --- Cấu hình PWM cho ESP32 ---
// const int freq = 5000;
// const int pwmChannel = 0;
// const int resolution = 8; // Phân giải 8-bit (0-255)

// // --- Thông số Động cơ ---
// // LƯU Ý: Ở chế độ CHANGE, số xung sẽ gấp đôi (nếu test 1 vòng ra khoảng 204 thì bạn điền 204.0 nhé)
// const float PULSES_PER_REV = 11300.0; 

// // --- Biến toàn cục ---
// volatile long pulseCount = 0; 
// float currentAngle = 0.0;
// float targetAngle = 180.0;    // Đặt mục tiêu xoay 180 độ

// // --- Thông số bộ điều khiển PID ---
// float Kp = 14; // sai số ít nhất :)
// float Ki = 0; // để 0 thì sẽ dừng hẩn không có dao động qua lại
// float Kd = 0;
// float previousError = 0.0;
// float integral = 0.0;

// // --- Bộ đệm Serial để nhận lệnh góc từ Serial Monitor ---
// String serialBuffer = "";

// // ==========================================
// // 1. Hàm ngắt Encoder (Đã nâng cấp chống nhiễu)
// // ==========================================
// void IRAM_ATTR readEncoder() {
//   int stateA = digitalRead(EN_A);
//   int stateB = digitalRead(EN_B);

//   // So sánh logic 2 kênh để đếm đúng chiều và lọc rung
//   if (stateA == stateB) {
//     pulseCount--; 
//   } else {
//     pulseCount++; 
//   }
// }

// // ==========================================
// // 2. Hàm điều khiển driver TB6612
// // ==========================================
// void setMotor(float speed) {
//   int pwmValue = constrain((int)speed, -255, 255); 

//   if (pwmValue > 0) {
//     // Chạy tới
//     digitalWrite(MOTOR_IN1, LOW);
//     digitalWrite(MOTOR_IN2, HIGH);
//     ledcWrite(pwmChannel, pwmValue);
//   } else if (pwmValue < 0) {
//     // Chạy lùi
//     digitalWrite(MOTOR_IN1, HIGH);
//     digitalWrite(MOTOR_IN2, LOW);
//     ledcWrite(pwmChannel, -pwmValue); 
//   } else {
//     // Dừng động cơ (Phanh cứng)
//     digitalWrite(MOTOR_IN1, LOW);
//     digitalWrite(MOTOR_IN2, LOW);
//     ledcWrite(pwmChannel, 0);
//   }
// }

// // ==========================================
// // 3. Xử lý lệnh góc từ Serial Monitor
// // ==========================================
// void processSerialLine(String line) {
//   line.trim();
//   if (line.length() == 0) return;

//   int dotCount = 0;
//   int digitCount = 0;
//   for (unsigned int i = 0; i < line.length(); i++) {
//     char ch = line[i];
//     if (ch >= '0' && ch <= '9') {
//       digitCount++;
//     } else if (ch == '.') {
//       dotCount++;
//     } else {
//       Serial.println("Error: invalid input, enter a number 0-180");
//       return;
//     }
//   }
//   if (dotCount > 1 || digitCount == 0) {
//     Serial.println("Error: invalid input");
//     return;
//   }

//   float angle = line.toFloat();
//   if (angle < 0.0 || angle > 180.0) {
//     Serial.println("Error: angle must be 0-180");
//     return;
//   }

//   targetAngle = angle;
//   Serial.printf("Input angle: %.1f°\n", angle);
//   Serial.printf("Commanding to: %.1f°\n", angle);
// }

// void setup() {
//   Serial.begin(115200);

//   // Setup chân Encoder
//   pinMode(EN_A, INPUT_PULLUP);
//   pinMode(EN_B, INPUT_PULLUP);
  
//   // Đọc ngắt CHANGE (cả sườn lên và sườn xuống) để tăng độ mượt
//   attachInterrupt(digitalPinToInterrupt(EN_A), readEncoder, CHANGE);

//   // Setup chân điều khiển TB6612
//   pinMode(MOTOR_IN1, OUTPUT);
//   pinMode(MOTOR_IN2, OUTPUT);
  
//   // Setup PWM ESP32
//   ledcSetup(pwmChannel, freq, resolution);
//   ledcAttachPin(MOTOR_PWM, pwmChannel);
// }

// void loop() {
//   // BƯỚC 0: Nhận lệnh từ Serial Monitor
//   while (Serial.available()) {
//     char c = (char)Serial.read();
//     if (c == '\n') {
//       processSerialLine(serialBuffer);
//       serialBuffer = "";
//     } else {
//       serialBuffer += c;
//     }
//   }

//   // BƯỚC A: Tính góc hiện tại của động cơ
//   currentAngle = (pulseCount / PULSES_PER_REV) * 360.0;

//   // BƯỚC B: Tính toán PID
//   float error = targetAngle - currentAngle;
//   integral += error;
//   float derivative = error - previousError;
  
//   float controlOutput = (Kp * error) + (Ki * integral) + (Kd * derivative);
//   previousError = error;

//   // BƯỚC C: Xuất tín hiệu điều khiển ra động cơ
//   setMotor(controlOutput);

//   // BƯỚC D: In thông số ra màn hình để theo dõi và Tuning
//   Serial.print("Target:");
//   Serial.print(targetAngle);
//   Serial.print("\tCurrent:");
//   Serial.print(currentAngle);
//   Serial.print("\tPWM_Output:");
//   Serial.println(controlOutput);

//   Serial.printf("Target angle: %.1f°\tEncoder angle: %.1f°\tError: %.1f°\n",
//                 targetAngle, currentAngle, targetAngle - currentAngle);

//   // Delay để chu kỳ lấy mẫu PID đều đặn
//   delay(10); 
// }
// // ============================================================
// // ESP32-S3 + DRV8833 + GA25-370 ENCODER
// // PID POSITION CONTROL - TB6612 ALIGNED BASELINE
// // ============================================================
// // #include <Arduino.h>

// // // ===============================
// // // ENCODER
// // // ===============================
// // #define EN_A 19
// // #define EN_B 20

// // volatile long pulseCount = 0;

// // // ===============================
// // // ENCODER INTERRUPT
// // // ===============================
// // void IRAM_ATTR readEncoder()
// // {
// //     int A = digitalRead(EN_A);
// //     int B = digitalRead(EN_B);

// //     // Chiều đếm
// //     if (A == B)
// //     {
// //         pulseCount++;
// //     }
// //     else
// //     {
// //         pulseCount--;
// //     }
// // }

// // void setup()
// // {
// //     Serial.begin(115200);

// //     delay(1000);

// //     pinMode(EN_A, INPUT_PULLUP);
// //     pinMode(EN_B, INPUT_PULLUP);

// //     pulseCount = 0;

// //     attachInterrupt(
// //         digitalPinToInterrupt(EN_A),
// //         readEncoder,
// //         CHANGE
// //     );

// //     Serial.println();
// //     Serial.println("==============================");
// //     Serial.println("ESP32-S3 ENCODER TEST");
// //     Serial.println("==============================");
// //     Serial.println();
// //     Serial.println("Khong quay motor de test...");
// // }

// // void loop()
// // {
// //     static unsigned long lastPrint = 0;

// //     long count;

// //     noInterrupts();
// //     count = pulseCount;
// //     interrupts();

// //     if (millis() - lastPrint >= 500)
// //     {
// //         lastPrint = millis();

// //         Serial.print("A=");
// //         Serial.print(digitalRead(EN_A));

// //         Serial.print("  B=");
// //         Serial.print(digitalRead(EN_B));

// //         Serial.print("  Count=");
// //         Serial.println(count);
// //     }
// // }
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

// TEST 03 - GPIO OUTPUT / Relay
// Phu hop: ESP32-S3 N16R8 CH343P
// Muc dich: xu ly tat ca output don gian (Relay/FET) theo PinConfig.h
//
// KHONG test PWM tai day (xem TEST 06 PWM/Fan).
// KHONG dieu khien Motor tai day (xem TEST 07 Motor Driver).
// KHONG upload quen khong tho! Day la testcase build/static.

// OLED SSD1306 128x64 I2C 0x3C SDA8 SCL9
#define OLED_SDA 8
#define OLED_SCL 9
#define OLED_ADDR 0x3C
#define OLED_W 128
#define OLED_H 64
Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);
bool oled_ok = false;

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
        display.println("OUTPUT TEST");
        display.println("----------------");
        display.println("INIT LOW");
        display.display();
        Serial.println("[OLED] INIT PASS");
    }

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

void updateOled() {
    if (!oled_ok) return;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("OUTPUT TEST");
    display.println("----------------");
    // Show current test if step < count
    if (step < kOutputCount) {
        bool isOn = digitalRead(kOutputPins[step]) == HIGH;
        display.print("TEST:");
        display.println(kOutputNames[step]);
        display.print("PIN: GPIO");
        display.println(kOutputPins[step]);
        display.print("STATE:");
        display.println(isOn ? "ON" : "OFF");
        display.print(isOn ? "HIGH" : "LOW");
    } else {
        display.println("ALL LOW");
        display.println("CYCLE DONE");
    }
    // Show all states compact
    char buf[32];
    snprintf(buf, sizeof(buf), "AC:%s FAN:%s",
             digitalRead(PIN_AC_RELAY) ? "ON" : "OFF",
             digitalRead(PIN_FAN_RELAY) ? "ON" : "OFF");
    display.setCursor(0, 56);
    display.print(buf);
    display.display();
}

unsigned long last_oled = 0;
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
    // OLED refresh 200ms, non-blocking
    if (now - last_oled >= 200) {
        last_oled = now;
        updateOled();
    }
}