#include <Arduino.h>
// Include production source (duoc phep qua -I root)
#include "../../drivers/MotorDriver.h"
#include "../../drivers/MotorDriver.cpp"
#include "../../drivers/EncoderDriver.h"
#include "../../drivers/EncoderDriver.cpp"
#include "../../services/MotorPositionController.h"
#include "../../services/MotorPositionController.cpp"
#include "../../model/SystemState.h"

// TEST 13 - MotorPositionController (logic + contract)
// Bao cao (khong sua):
//   - Encoder pins TBD trong PinConfig (0,0,0) -> encoder->begin khong goi, getPosition=0
//   - MotorDriver 20kHz/10-bit, speed signed -1000..+1000 (thi kien 8-bit cu)
//   - Trong update(): khi abs(error)<10 se motor stop.
// Test nay lap 1 EncoderDriver gia (dung GPIO that khong) - chi verify contract:
//   begin(0,0,0) -> encoder chua init; motor.begin() se goi ledcAttach(13/14).
//   Luu y: neu chay that len bo mang thi motor se hoat dong! vi vay chi build/static.

void setup() {
    Serial.begin(115200);
    delay(100);
    while (!Serial && millis() < 3000) {}

    Serial.println();
    Serial.println("=== TEST 13 : MotorPositionController (compile/contract) ===");

    drivers::MotorDriver motor;
    drivers::EncoderDriver enc;
    services::MotorPositionController ctrl;

    // Encoder pins = 0 (TBD) -> encoder khong begin
    bool ok = ctrl.begin(&motor, &enc, 0, 0, 0);
    Serial.printf("[MPC] begin() = %s\r\n", ok ? "true" : "false");
    Serial.printf("[MPC] eff init encoder = %s\r\n", enc.isInitialized() ? "true" : "false");

    ctrl.setKp(1.0f);
    ctrl.setTargetPosition(500);
    ctrl.enable(true);
    Serial.printf("[MPC] target=500 enabled=%s\r\n",
                  ctrl.isEnabled() ? "true" : "false");

    // update() -> encoder null -> return (khong goi motor)
    ctrl.update();
    Serial.printf("[MPC] current=0 atTarget=%s (encoder khong init - TBD)\r\n",
                  ctrl.isAtTarget() ? "true" : "false");

    Serial.println("[PROD-NOTE] Encoder pins TBD -> chua the test dong that");
    Serial.println("[MPC] done (build/static - HARDWARE NOT TESTED)");
}

void loop() {
    delay(1000);
}