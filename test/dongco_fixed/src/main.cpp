/**
 * @file main.cpp
 * @brief ESP32-S3 DC Motor Control via DRV8833 Parallel Mode
 * 
 * Hardware: ESP32-S3 N8 + DRV8833 (Parallel Mode)
 * 
 * Wiring:
 *   ESP32-S3          DRV8833
 *   --------          --------
 *   GPIO5    ----->   AIN1 + BIN1 (IN1)
 *   GPIO6    ----->   AIN2 + BIN2 (IN2)
 *   GPIO48   ----->   nSLEEP
 *   GND      ----->   GND
 *   VM       ----->   Motor Supply (per datasheet)
 * 
 * DRV8833 Parallel Mode Truth Table:
 * | IN1  | IN2  | OUT1/OUT2 | Motor     |
 * |------|------|-----------|-----------|
 * | LOW  | LOW  | Hi-Z      | Coast     |
 * | PWM  | LOW  | PWM       | Forward   |
 * | LOW  | PWM  | PWM       | Backward  |
 * | HIGH | HIGH | LOW       | Brake     |
 */

#include <Arduino.h>

// ============================================================
// PIN DEFINITIONS
// ============================================================
#define IN1_PIN       5    // AIN1 + BIN1 (Parallel Mode)
#define IN2_PIN       6    // AIN2 + BIN2 (Parallel Mode)
#define NSLEEP_PIN    48   // nSLEEP (Active HIGH = RUN, LOW = SLEEP)

// ============================================================
// PWM CONFIGURATION (LEDC - compatible with Arduino-ESP32)
// ============================================================
constexpr uint32_t PWM_FREQ_HZ     = 20000;  // 20 kHz
constexpr uint8_t  PWM_RES_BITS    = 8;      // 8-bit resolution (0-255)
constexpr uint16_t PWM_MAX_DUTY    = (1 << PWM_RES_BITS) - 1;  // 255
constexpr uint8_t  PWM_CHANNEL_IN1 = 0;      // LEDC channel for IN1
constexpr uint8_t  PWM_CHANNEL_IN2 = 1;      // LEDC channel for IN2

// ============================================================
// SAFETY CONSTANTS
// ============================================================
constexpr uint16_t DIRECTION_CHANGE_DELAY_MS = 75;  // Delay when changing direction

// ============================================================
// MOTOR STATE ENUM
// ============================================================
enum class MotorState {
    STOP,       // Coast (IN1=LOW, IN2=LOW)
    FORWARD,    // Forward (IN1=PWM, IN2=LOW)
    BACKWARD,   // Backward (IN1=LOW, IN2=PWM)
    BRAKE,      // Brake (IN1=HIGH, IN2=HIGH)
    SLEEP       // Driver asleep (nSLEEP=LOW)
};

// ============================================================
// GLOBAL STATE
// ============================================================
MotorState currentState = MotorState::STOP;
uint8_t currentSpeed = 0;
bool driverAwake = false;

// ============================================================
// FORWARD DECLARATIONS
// ============================================================
void motorForward(int speed);
void motorBackward(int speed);
void motorStop();
void motorBrake();
void motorSleep();
void motorWake();
void printStatus();
void processCommand(const String& cmd);
void safeSetPinsLow();
void initPWM();

// ============================================================
// SETUP
// ============================================================
void setup() {
    // 1. Initialize Serial first for debugging
    Serial.begin(115200);
    while (!Serial && millis() < 2000) { delay(10); }
    
    Serial.println();
    Serial.println(F("========================================"));
    Serial.println(F("  ESP32-S3 DRV8833 Motor Controller"));
    Serial.println(F("  Parallel Mode - GPIO5/6/48"));
    Serial.println(F("========================================"));
    
    // 2. SAFETY: Set all control pins LOW immediately
    pinMode(IN1_PIN, OUTPUT);
    pinMode(IN2_PIN, OUTPUT);
    pinMode(NSLEEP_PIN, OUTPUT);
    
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, LOW);
    digitalWrite(NSLEEP_PIN, LOW);  // Driver asleep initially
    
    driverAwake = false;
    currentState = MotorState::STOP;
    currentSpeed = 0;
    
    Serial.println(F("[INIT] Control pins set LOW, nSLEEP=LOW"));
    
    // 3. Initialize PWM (LEDC) on IN1 and IN2
    initPWM();
    
    Serial.printf("[INIT] PWM initialized: %u Hz, %u-bit\n", PWM_FREQ_HZ, PWM_RES_BITS);
    Serial.println(F("[INIT] IN1=0, IN2=0"));
    
    // 4. Wake driver after PWM is ready
    motorWake();
    
    // 5. Print help
    printHelp();
}

// ============================================================
// PWM INITIALIZATION (LEDC API compatible)
// ============================================================
void initPWM() {
    // Configure LEDC timers and channels
    // Channel 0 -> IN1, Channel 1 -> IN2
    ledcSetup(PWM_CHANNEL_IN1, PWM_FREQ_HZ, PWM_RES_BITS);
    ledcSetup(PWM_CHANNEL_IN2, PWM_FREQ_HZ, PWM_RES_BITS);
    
    // Attach pins to channels
    ledcAttachPin(IN1_PIN, PWM_CHANNEL_IN1);
    ledcAttachPin(IN2_PIN, PWM_CHANNEL_IN2);
    
    // Start with 0 duty
    ledcWrite(PWM_CHANNEL_IN1, 0);
    ledcWrite(PWM_CHANNEL_IN2, 0);
}

// ============================================================
// MAIN LOOP
// ============================================================
void loop() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        cmd.toUpperCase();
        
        if (cmd.length() > 0) {
            processCommand(cmd);
        }
    }
    
    // Small delay to prevent watchdog issues
    delay(10);
}

// ============================================================
// MOTOR CONTROL FUNCTIONS
// ============================================================

/**
 * @brief Set motor FORWARD with PWM speed
 * IN1 = PWM, IN2 = LOW
 */
void motorForward(int speed) {
    speed = constrain(speed, 0, PWM_MAX_DUTY);
    
    // Safety: if currently backward, stop first with delay
    if (currentState == MotorState::BACKWARD) {
        motorStop();
        delay(DIRECTION_CHANGE_DELAY_MS);
    }
    
    if (!driverAwake) motorWake();
    
    ledcWrite(PWM_CHANNEL_IN1, speed);
    ledcWrite(PWM_CHANNEL_IN2, 0);
    // Ensure IN2 is driven LOW explicitly
    pinMode(IN2_PIN, OUTPUT);
    digitalWrite(IN2_PIN, LOW);
    // Restore PWM on IN1
    ledcAttachPin(IN1_PIN, PWM_CHANNEL_IN1);
    
    currentState = MotorState::FORWARD;
    currentSpeed = speed;
}

/**
 * @brief Set motor BACKWARD with PWM speed
 * IN1 = LOW, IN2 = PWM
 */
void motorBackward(int speed) {
    speed = constrain(speed, 0, PWM_MAX_DUTY);
    
    // Safety: if currently forward, stop first with delay
    if (currentState == MotorState::FORWARD) {
        motorStop();
        delay(DIRECTION_CHANGE_DELAY_MS);
    }
    
    if (!driverAwake) motorWake();
    
    ledcWrite(PWM_CHANNEL_IN1, 0);
    ledcWrite(PWM_CHANNEL_IN2, speed);
    // Ensure IN1 is driven LOW
    pinMode(IN1_PIN, OUTPUT);
    digitalWrite(IN1_PIN, LOW);
    // Restore PWM on IN2
    ledcAttachPin(IN2_PIN, PWM_CHANNEL_IN2);
    
    currentState = MotorState::BACKWARD;
    currentSpeed = speed;
}

/**
 * @brief COAST/STOP - Motor free wheels
 * IN1 = LOW, IN2 = LOW
 */
void motorStop() {
    if (!driverAwake) motorWake();
    
    ledcWrite(PWM_CHANNEL_IN1, 0);
    ledcWrite(PWM_CHANNEL_IN2, 0);
    
    // Explicitly drive both LOW
    pinMode(IN1_PIN, OUTPUT);
    pinMode(IN2_PIN, OUTPUT);
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, LOW);
    
    currentState = MotorState::STOP;
    currentSpeed = 0;
}

/**
 * @brief BRAKE - Motor shorted (both outputs LOW)
 * IN1 = HIGH, IN2 = HIGH
 */
void motorBrake() {
    if (!driverAwake) motorWake();
    
    // Stop PWM first
    ledcWrite(PWM_CHANNEL_IN1, 0);
    ledcWrite(PWM_CHANNEL_IN2, 0);
    
    // Drive both HIGH for brake
    pinMode(IN1_PIN, OUTPUT);
    pinMode(IN2_PIN, OUTPUT);
    digitalWrite(IN1_PIN, HIGH);
    digitalWrite(IN2_PIN, HIGH);
    
    currentState = MotorState::BRAKE;
    currentSpeed = 0;
}

/**
 * @brief Put DRV8833 to SLEEP (low power)
 * nSLEEP = LOW
 * Also ensures IN1/IN2 are LOW for safety
 */
void motorSleep() {
    safeSetPinsLow();
    digitalWrite(NSLEEP_PIN, LOW);
    driverAwake = false;
    currentState = MotorState::SLEEP;
    currentSpeed = 0;
}

/**
 * @brief Wake DRV8833 from SLEEP
 * nSLEEP = HIGH
 * Motor remains in STOP state
 */
void motorWake() {
    digitalWrite(NSLEEP_PIN, HIGH);
    driverAwake = true;
    // Motor stays in current state (default STOP)
    if (currentState == MotorState::SLEEP) {
        currentState = MotorState::STOP;
        safeSetPinsLow();
    }
}

/**
 * @brief Helper: Set IN1 and IN2 to LOW (GPIO mode)
 */
void safeSetPinsLow() {
    ledcWrite(PWM_CHANNEL_IN1, 0);
    ledcWrite(PWM_CHANNEL_IN2, 0);
    pinMode(IN1_PIN, OUTPUT);
    pinMode(IN2_PIN, OUTPUT);
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, LOW);
}

// ============================================================
// SERIAL COMMAND PROCESSING
// ============================================================

/**
 * @brief Parse and execute serial command
 * Commands: Fxxx, Bxxx, S, BRAKE, SLEEP, WAKE, STATUS, HELP
 */
void processCommand(const String& cmd) {
    if (cmd.length() == 0) return;
    
    char type = cmd.charAt(0);
    
    // FORWARD commands: F0 - F255
    if (type == 'F' && cmd.length() > 1) {
        int speed = cmd.substring(1).toInt();
        speed = constrain(speed, 0, PWM_MAX_DUTY);
        motorForward(speed);
        Serial.printf("[CMD] FORWARD\nSpeed: %d\nIN1: PWM\nIN2: LOW\nnSLEEP: %s\n", 
                      speed, driverAwake ? "HIGH" : "LOW");
        return;
    }
    
    // BACKWARD commands: B0 - B255
    if (type == 'B' && cmd.length() > 1) {
        int speed = cmd.substring(1).toInt();
        speed = constrain(speed, 0, PWM_MAX_DUTY);
        motorBackward(speed);
        Serial.printf("[CMD] BACKWARD\nSpeed: %d\nIN1: LOW\nIN2: PWM\nnSLEEP: %s\n", 
                      speed, driverAwake ? "HIGH" : "LOW");
        return;
    }
    
    // Single-letter / word commands
    if (cmd == "S") {
        motorStop();
        Serial.printf("[CMD] STOP (COAST)\nIN1: LOW\nIN2: LOW\nnSLEEP: %s\n", 
                      driverAwake ? "HIGH" : "LOW");
        return;
    }
    
    if (cmd == "BRAKE") {
        motorBrake();
        Serial.printf("[CMD] BRAKE\nIN1: HIGH\nIN2: HIGH\nnSLEEP: %s\n", 
                      driverAwake ? "HIGH" : "LOW");
        return;
    }
    
    if (cmd == "SLEEP") {
        motorSleep();
        Serial.println(F("[CMD] SLEEP\nDriver: ASLEEP\nnSLEEP: LOW"));
        return;
    }
    
    if (cmd == "WAKE") {
        motorWake();
        Serial.println(F("[CMD] WAKE\nDriver: AWAKE\nnSLEEP: HIGH"));
        return;
    }
    
    if (cmd == "STATUS") {
        printStatus();
        return;
    }
    
    if (cmd == "HELP") {
        printHelp();
        return;
    }
    
    // Unknown command
    Serial.println(F("[ERROR] Unknown command. Type HELP for list."));
}

/**
 * @brief Print detailed motor status
 */
void printStatus() {
    const char* stateStr;
    const char* in1Str;
    const char* in2Str;
    
    switch (currentState) {
        case MotorState::FORWARD:
            stateStr = "FORWARD";
            in1Str = "PWM";
            in2Str = "LOW";
            break;
        case MotorState::BACKWARD:
            stateStr = "BACKWARD";
            in1Str = "LOW";
            in2Str = "PWM";
            break;
        case MotorState::STOP:
            stateStr = "STOP (COAST)";
            in1Str = "LOW";
            in2Str = "LOW";
            break;
        case MotorState::BRAKE:
            stateStr = "BRAKE";
            in1Str = "HIGH";
            in2Str = "HIGH";
            break;
        case MotorState::SLEEP:
            stateStr = "SLEEP";
            in1Str = "LOW";
            in2Str = "LOW";
            break;
        default:
            stateStr = "UNKNOWN";
            in1Str = "?";
            in2Str = "?";
    }
    
    Serial.println(F("========== MOTOR STATUS =========="));
    Serial.printf("Driver      : %s\n", driverAwake ? "AWAKE" : "ASLEEP");
    Serial.printf("Direction   : %s\n", stateStr);
    Serial.printf("Speed       : %d\n", currentSpeed);
    Serial.printf("IN1         : %s\n", in1Str);
    Serial.printf("IN2         : %s\n", in2Str);
    Serial.printf("nSLEEP      : %s\n", driverAwake ? "HIGH" : "LOW");
    Serial.printf("PWM         : %u Hz\n", PWM_FREQ_HZ);
    Serial.printf("Resolution  : %u bit\n", PWM_RES_BITS);
    Serial.println(F("==================================="));
}

/**
 * @brief Print help menu
 */
void printHelp() {
    Serial.println(F("\n========== COMMANDS =========="));
    Serial.println(F("  F0 - F255    : Forward speed 0-255"));
    Serial.println(F("  B0 - B255    : Backward speed 0-255"));
    Serial.println(F("  S            : Stop (Coast)"));
    Serial.println(F("  BRAKE        : Brake (Short)"));
    Serial.println(F("  SLEEP        : Driver sleep (nSLEEP=LOW)"));
    Serial.println(F("  WAKE         : Driver wake (nSLEEP=HIGH)"));
    Serial.println(F("  STATUS       : Show current status"));
    Serial.println(F("  HELP         : Show this help"));
    Serial.println(F("===============================\n"));
}