/*
 * ============================================================
 * TEST 1 - Arduino Mega 2560 / ATmega2560
 * Mục tiêu: kiểm tra boot, Serial Monitor, CPU chạy, lệnh nhận.
 * CHỈ test Arduino Mega. Chưa ESP32, CAN, UART, motor, PID.
 * Nguồn: Báo cáo NCKH (UART2 D16/D17 @9600, relay D50, blower D46).
 * ============================================================
 */
#include <Arduino.h>

/* ================= CẤU HÌNH CHÂN (đã xác định) ================= */
#define PIN_RELAY_HTR      50      // Relay HTR: HIGH = relay đóng (theo tài liệu)
#define PIN_BLOWER_PWM     46      // MOSFET Blower: chỉ set thấp ở TEST 1

/* ============ CHÂN CHƯA XÁC ĐỊNH (TODO_PIN) ==================== */
// Cần người dùng xác nhận trước TEST 8 (encoder) và TEST 9 (L298N)
#define PIN_MOTOR_IN1      -1      // TODO_PIN: L298N IN1
#define PIN_MOTOR_IN2      -1      // TODO_PIN: L298N IN2
#define PIN_MOTOR_ENA      -1      // TODO_PIN: L298N ENA/ENB
#define PIN_ENCODER_A      -1      // TODO_PIN: Encoder kênh A
#define PIN_ENCODER_B      -1      // TODO_PIN: Encoder kênh B

/* ==================== CẤU HÌNH GIAO TIẾP ======================= */
#define SERIAL_BAUDRATE    115200 // Serial Monitor
#define UART2_BAUDRATE     9600   // UART với ESP32 (chưa dùng ở TEST 1)

/* ========================= TRẠNG THÁI =========================== */
#define RELAY_OFF          LOW
#define RELAY_ON           HIGH
#define CMD_BUFFER_SIZE    40      // Kích thước buffer lệnh
#define CMD_TIMEOUT_MS     200     // Hủy lệnh gửi dang dở sau timeout

/* ===================== BIẾN TRẠNG THÁI ========================== */
static bool  relayState  = false;  // Mặc định boot: Relay OFF
static bool  motorStop   = true;   // Mặc định boot: Motor STOP
static int   blowerDuty  = 0;      // Mặc định boot: Blower 0%
static char  cmdBuffer[CMD_BUFFER_SIZE];
static int   cmdIndex    = 0;
static unsigned long lastCharMs = 0;

/* ===================== HÀM IN =================================== */
static void inBannerBoot() {
    Serial.println("================================");
    Serial.println("ARDUINO MEGA HARDWARE TEST");
    Serial.println("System boot OK");
    Serial.println("Board: ATmega2560");
    Serial.println("================================");
    Serial.println("STATUS: READY");
}

static void inHelp() {
    Serial.println("[HELP] Danh sach lenh:");
    Serial.println("  help    - hien thi tro giup");
    Serial.println("  status  - hien thi trang thai he thong");
    Serial.println("  (TEST 2 se bo sung: relay on / relay off / emergency stop)");
}

static void inStatus() {
    Serial.println("[STATUS] Trang thai he thong:");
    Serial.print  ("[STATUS] Relay:  ");
    Serial.println(relayState ? "ON" : "OFF");
    Serial.print  ("[STATUS] Motor:  ");
    Serial.println(motorStop ? "STOP" : "RUN");
    Serial.print  ("[STATUS] Blower: ");
    Serial.print  (blowerDuty);
    Serial.println("%");
    Serial.print  ("[STATUS] PWM:    ");
    Serial.println("0");
    Serial.print  ("[STATUS] Uptime: ");
    Serial.print  (millis());
    Serial.println(" ms");
}

/* ==================== XỬ LÝ LỆNH ================================ */
static void xuLyLenh(const char *cmd) {
    if (strcmp(cmd, "help") == 0) {
        inHelp();
    } else if (strcmp(cmd, "status") == 0) {
        inStatus();
    } else if (strlen(cmd) == 0) {
        /* bỏ qua dòng trống */
    } else {
        Serial.print("[ERROR] Lenh khong hop le: ");
        Serial.println(cmd);
    }
}

/* ============== ĐỌC LỆNH KHÔNG CHẶN (millis) ==================== */
static void docLenhSerial() {
    while (Serial.available() > 0) {
        char c = (char)Serial.read();

        if (c == '\n' || c == '\r') {
            if (cmdIndex > 0) {
                cmdBuffer[cmdIndex] = '\0';
                xuLyLenh(cmdBuffer);
                cmdIndex = 0;
            }
            continue;
        }

        if (cmdIndex < CMD_BUFFER_SIZE - 1) {
            cmdBuffer[cmdIndex++] = c;
            lastCharMs = millis();
        } else {
            cmdIndex = 0;   // tràn buffer -> reset
        }
    }

    /* hủy lệnh gửi dang dở sau timeout (tránh kẹt buffer) */
    if (cmdIndex > 0 && (millis() - lastCharMs) > CMD_TIMEOUT_MS) {
        cmdIndex = 0;
    }
}

/* ========================= SETUP ================================= */
void setup() {
    Serial.begin(SERIAL_BAUDRATE);
    Serial2.begin(UART2_BAUDRATE);

    /* chân đã xác định -> trạng thái an toàn khi boot */
    pinMode(PIN_RELAY_HTR, OUTPUT);
    digitalWrite(PIN_RELAY_HTR, RELAY_OFF);     // Relay = OFF
    pinMode(PIN_BLOWER_PWM, OUTPUT);
    digitalWrite(PIN_BLOWER_PWM, LOW);          // Blower = 0%

    /* chân TODO_PIN: chưa khởi tạo, chờ người dùng xác nhận
       (PIN_MOTOR_IN1/IN2/ENA, PIN_ENCODER_A/B) */

    inBannerBoot();
    Serial.println("[INFO] System boot");
    Serial.println("[UART] UART2 initialized 9600 baud");
    Serial.println("[SAFETY] Motor = STOP");
    Serial.println("[SAFETY] Relay = OFF");
    Serial.println("[SAFETY] Blower = 0%");
}

/* ========================= LOOP ================================== */
void loop() {
    docLenhSerial();   // non-blocking, không dùng delay()
}
