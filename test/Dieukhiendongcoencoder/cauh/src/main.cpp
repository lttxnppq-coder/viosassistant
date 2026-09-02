#include <Arduino.h>

// ============================================================
// PIN CONFIGURATION
// ============================================================

#define IN1_PIN       13
#define IN2_PIN       14
#define NFAULT_PIN     7

// nSLEEP của DRV8833:
// KHÔNG điều khiển bằng ESP32.
// Bạn đã nối trực tiếp nSLEEP -> 3.3V ESP32.

// ============================================================
// PWM CONFIGURATION
// ============================================================

#define PWM_CH_IN1     0
#define PWM_CH_IN2     1

#define PWM_FREQ_HZ    20000
#define PWM_RES_BITS   8

#define PWM_MAX_DUTY   255

// Thời gian chờ khi đổi chiều
#define DIRECTION_CHANGE_DELAY_MS 75


// ============================================================
// MOTOR STATE
// ============================================================

enum MotorState
{
    STOP,
    FORWARD,
    BACKWARD,
    BRAKE
};

MotorState currentState = STOP;

uint8_t currentSpeed = 0;


// ============================================================
// FUNCTION DECLARATIONS
// ============================================================

void motorForward(int speed);
void motorBackward(int speed);
void motorStop();
void motorBrake();

void checkFault();

void printStatus();
void printHelp();

void processCommand(String cmd);


// ============================================================
// SETUP
// ============================================================

void setup()
{
    // --------------------------------------------------------
    // Serial
    // --------------------------------------------------------

    Serial.begin(115200);

    delay(500);

    Serial.println();
    Serial.println("========================================");
    Serial.println(" ESP32-S3 + DRV8833 MOTOR CONTROLLER");
    Serial.println("========================================");

    // --------------------------------------------------------
    // Pin configuration
    // --------------------------------------------------------

    pinMode(IN1_PIN, OUTPUT);
    pinMode(IN2_PIN, OUTPUT);

    // nFAULT là active LOW
    // HIGH = bình thường
    // LOW  = DRV8833 báo lỗi
    pinMode(NFAULT_PIN, INPUT_PULLUP);

    // --------------------------------------------------------
    // Motor OFF khi khởi động
    // --------------------------------------------------------

    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, LOW);

    // --------------------------------------------------------
    // PWM setup
    // --------------------------------------------------------

    ledcSetup(
        PWM_CH_IN1,
        PWM_FREQ_HZ,
        PWM_RES_BITS
    );

    ledcSetup(
        PWM_CH_IN2,
        PWM_FREQ_HZ,
        PWM_RES_BITS
    );

    ledcAttachPin(
        IN1_PIN,
        PWM_CH_IN1
    );

    ledcAttachPin(
        IN2_PIN,
        PWM_CH_IN2
    );

    // PWM = 0 lúc khởi động
    ledcWrite(PWM_CH_IN1, 0);
    ledcWrite(PWM_CH_IN2, 0);
currentState = STOP;
    currentSpeed = 0;

    // --------------------------------------------------------
    // Print configuration
    // --------------------------------------------------------

    Serial.println();
    Serial.println("[PIN CONFIGURATION]");
    Serial.println("IN1     -> GPIO13");
    Serial.println("IN2     -> GPIO14");
    Serial.println("nFAULT  -> GPIO7");
    Serial.println("nSLEEP  -> 3.3V FIXED");
    Serial.println("VM      -> 9V");
    Serial.println();

    Serial.println("[PWM]");
    Serial.println("Frequency : 20 kHz");
    Serial.println("Resolution: 8 bit");
    Serial.println("Range     : 0 - 255");
    Serial.println();

    // --------------------------------------------------------
    // Check DRV8833 fault at startup
    // --------------------------------------------------------

    checkFault();

    // --------------------------------------------------------
    // Print commands
    // --------------------------------------------------------

    printHelp();
}


// ============================================================
// LOOP
// ============================================================

void loop()
{
    if (Serial.available())
    {
        String cmd = Serial.readStringUntil('\n');

        processCommand(cmd);
    }

    delay(10);
}


// ============================================================
// MOTOR FORWARD
// ============================================================

void motorForward(int speed)
{
    speed = constrain(
        speed,
        0,
        PWM_MAX_DUTY
    );

    // Nếu đang chạy ngược thì dừng trước
    if (currentState == BACKWARD)
    {
        motorStop();

        delay(DIRECTION_CHANGE_DELAY_MS);
    }

    // IN1 = PWM
    // IN2 = LOW

    ledcWrite(
        PWM_CH_IN1,
        speed
    );

    ledcWrite(
        PWM_CH_IN2,
        0
    );

    currentState = FORWARD;
    currentSpeed = speed;
}


// ============================================================
// MOTOR BACKWARD
// ============================================================

void motorBackward(int speed)
{
    speed = constrain(
        speed,
        0,
        PWM_MAX_DUTY
    );

    // Nếu đang chạy thuận thì dừng trước
    if (currentState == FORWARD)
    {
        motorStop();

        delay(DIRECTION_CHANGE_DELAY_MS);
    }

    // IN1 = LOW
    // IN2 = PWM

    ledcWrite(
        PWM_CH_IN1,
        0
    );

    ledcWrite(
        PWM_CH_IN2,
        speed
    );

    currentState = BACKWARD;
    currentSpeed = speed;
}


// ============================================================
// MOTOR STOP - COAST
// ============================================================

void motorStop()
{
    // IN1 = LOW
    // IN2 = LOW

    ledcWrite(
        PWM_CH_IN1,
        0
    );

    ledcWrite(
        PWM_CH_IN2,
        0
    );

    currentState = STOP;
    currentSpeed = 0;
}


// ============================================================
// MOTOR BRAKE
// ============================================================

void motorBrake()
{
    // IN1 = HIGH
    // IN2 = HIGH

    ledcWrite(
        PWM_CH_IN1,
        PWM_MAX_DUTY
    );

    ledcWrite(
        PWM_CH_IN2,
        PWM_MAX_DUTY
    );

    currentState = BRAKE;
    currentSpeed = 0;
}


// ============================================================
// CHECK DRV8833 nFAULT
// ============================================================

void checkFault()
{
    int faultState = digitalRead(NFAULT_PIN);

    Serial.println();
    Serial.println("========================================");
    Serial.println("        DRV8833 FAULT CHECK");
    Serial.println("========================================");

    Serial.print("nFAULT GPIO7 = ");

    if (faultState == LOW)
    {
        Serial.println("LOW");

        Serial.println();
        Serial.println("!!! DRV8833 BAO LOI !!!");

        Serial.println();
        Serial.println("Co the kiem tra:");
        Serial.println("1. Qua dong motor");
        Serial.println("2. Chan output bi cham");
        Serial.println("3. Motor bi ket");
        Serial.println("4. VM khong dung");
        Serial.println("5. DRV8833 qua nhiet");
        Serial.println("6. DRV8833 bi hong");
    }
    else
    {
        Serial.println("HIGH");

        Serial.println();
        Serial.println("DRV8833 KHONG BAO LOI");
    }

    Serial.println("========================================");
    Serial.println();
}


// ============================================================
// PROCESS COMMAND
// ============================================================

void processCommand(String cmd)
{
    // --------------------------------------------------------
    // Xóa khoảng trắng / xuống dòng
    // --------------------------------------------------------

    cmd.trim();

    // --------------------------------------------------------
    // Chuyển toàn bộ thành chữ hoa
    // --------------------------------------------------------

    cmd.toUpperCase();

    // --------------------------------------------------------
    // DEBUG: hiển thị lệnh nhận được
    // --------------------------------------------------------

    Serial.print("[RX] ");
    Serial.println(cmd);


    // ========================================================
    // FAULT
    // ========================================================

    // Đặt FAULT TRƯỚC lệnh F
    // để không bị hiểu thành FORWARD

    if (cmd == "FAULT")
    {
        checkFault();

        return;
    }


    // ========================================================
    // STATUS
    // ========================================================

    if (cmd == "STATUS")
    {
        printStatus();

        return;
    }


    // ========================================================
    // HELP
    // ========================================================

    if (cmd == "HELP")
    {
        printHelp();
return;
    }


    // ========================================================
    // STOP
    // ========================================================

    if (cmd == "S")
    {
        motorStop();

        Serial.println("[CMD] STOP");

        return;
    }


    // ========================================================
    // BRAKE
    // ========================================================

    if (cmd == "BRAKE")
    {
        motorBrake();

        Serial.println("[CMD] BRAKE");

        return;
    }


    // ========================================================
    // FORWARD
    // F0 -> F255
    // ========================================================

    if (cmd.startsWith("F"))
    {
        // Kiểm tra phải có số phía sau F
        if (cmd.length() <= 1)
        {
            Serial.println("[ERROR] Vi du: F150");

            return;
        }

        int speed = cmd.substring(1).toInt();

        speed = constrain(
            speed,
            0,
            PWM_MAX_DUTY
        );

        motorForward(speed);

        Serial.println("[CMD] FORWARD");

        Serial.print("Speed: ");
        Serial.println(speed);

        Serial.println("IN1: PWM");
        Serial.println("IN2: LOW");

        return;
    }


    // ========================================================
    // BACKWARD
    // B0 -> B255
    // ========================================================

    if (cmd.startsWith("B"))
    {
        // Kiểm tra phải có số phía sau B
        if (cmd.length() <= 1)
        {
            Serial.println("[ERROR] Vi du: B150");

            return;
        }

        int speed = cmd.substring(1).toInt();

        speed = constrain(
            speed,
            0,
            PWM_MAX_DUTY
        );

        motorBackward(speed);

        Serial.println("[CMD] BACKWARD");

        Serial.print("Speed: ");
        Serial.println(speed);

        Serial.println("IN1: LOW");
        Serial.println("IN2: PWM");

        return;
    }


    // ========================================================
    // UNKNOWN COMMAND
    // ========================================================

    Serial.print("[ERROR] Unknown command: ");
    Serial.println(cmd);

    Serial.println("Nhap HELP de xem lenh.");
}


// ============================================================
// PRINT STATUS
// ============================================================

void printStatus()
{
    Serial.println();
    Serial.println("========================================");
    Serial.println("           MOTOR STATUS");
    Serial.println("========================================");

    Serial.print("State: ");

    switch (currentState)
    {
        case FORWARD:
            Serial.println("FORWARD");
            break;

        case BACKWARD:
            Serial.println("BACKWARD");
            break;

        case STOP:
            Serial.println("STOP");
            break;

        case BRAKE:
Serial.println("BRAKE");
            break;

        default:
            Serial.println("UNKNOWN");
            break;
    }

    Serial.print("Speed: ");
    Serial.println(currentSpeed);

    Serial.println();

    Serial.println("PIN:");
    Serial.println("IN1    = GPIO13");
    Serial.println("IN2    = GPIO14");
    Serial.println("nFAULT = GPIO7");
    Serial.println("nSLEEP = 3.3V");

    Serial.println();

    Serial.print("nFAULT state: ");

    if (digitalRead(NFAULT_PIN) == LOW)
    {
        Serial.println("LOW - FAULT");
    }
    else
    {
        Serial.println("HIGH - OK");
    }

    Serial.println("========================================");
    Serial.println();
}


// ============================================================
// PRINT HELP
// ============================================================

void printHelp()
{
    Serial.println();
    Serial.println("========================================");
    Serial.println("             COMMAND LIST");
    Serial.println("========================================");

    Serial.println("F0 - F255");
    Serial.println("  -> Forward");

    Serial.println();

    Serial.println("B0 - B255");
    Serial.println("  -> Backward");

    Serial.println();

    Serial.println("S");
    Serial.println("  -> Stop / Coast");

    Serial.println();

    Serial.println("BRAKE");
    Serial.println("  -> Brake motor");

    Serial.println();

    Serial.println("FAULT");
    Serial.println("  -> Check DRV8833 nFAULT");

    Serial.println();

    Serial.println("STATUS");
    Serial.println("  -> Motor + nFAULT status");

    Serial.println();

    Serial.println("HELP");
    Serial.println("  -> Show commands");

    Serial.println("========================================");
    Serial.println();
}