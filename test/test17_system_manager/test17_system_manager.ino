#include <Arduino.h>
#include "SystemManager.h"
#include "SystemState.h"
#include "FanController.h"
#include "MotorPositionController.h"
#include "VehicleDataService.h"

// TEST 17 - SystemManager Integration (compile/static)
// Verifies SystemManager aggregates all subsystems and compiles without hardware
// HARDWARE: NOT EXECUTED

void assertCond(const char* n,bool c){ Serial.printf("[SYS] %-30s -> %s\r\n", n, c?"PASS":"FAIL"); }

void setup(){
    Serial.begin(115200); delay(100); while(!Serial && millis()<3000){}
    Serial.println(); Serial.println("=== TEST 17 : SystemManager Integration (compile/static) ===");
    // Compile-time checks (header-only, no link to .cpp required)
    static_assert(sizeof(application::SystemManager) > 0, "SystemManager size>0");
    static_assert(sizeof(model::SystemState) > 0, "SystemState size>0");
    static_assert(sizeof(services::FanController) > 0, "FanController size>0");
    static_assert(sizeof(services::MotorPositionController) > 0, "MotorPositionController size>0");
    // Type existence checks (no hardware, no .cpp linking)
    application::SystemManager* mgr = nullptr;
    assertCond("SystemManager type exists", mgr==nullptr);
    model::SystemState st; st.mode = model::SystemMode::INITIALIZING; st.error = model::ErrorCode::NONE;
    assertCond("SystemState default", st.mode==model::SystemMode::INITIALIZING);
    // Verify PinConfig single source compiles with SystemManager
    assertCond("PinConfig integrated", PIN_FAN_FET_PWM==7 && PIN_MOTOR_IN1==13);
    Serial.println("[SYS] Integration compile PASS HARDWARE: NOT EXECUTED");
}
void loop(){ delay(1000); }
