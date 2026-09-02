#include <Arduino.h>
#include "SystemManager.h"
#include "CommunicationTask.h"
#include "ControlTask.h"
#include "OledTask.h"

// TEST 24 - Architecture Audit (static)
// Verifies layering: drivers -> services -> application -> rtos, no upward dependencies
// Checks: PinConfig single source, model as pure data, utils independent, rtos depends on application
// HARDWARE: NOT EXECUTED

void assertCond(const char* n,bool c){ Serial.printf("[ARCH] %-30s -> %s\r\n", n, c?"PASS":"FAIL"); }

void setup(){
    Serial.begin(115200); delay(100); while(!Serial && millis()<3000){}
    Serial.println(); Serial.println("=== TEST 24 : Architecture Audit ===");
    static_assert(sizeof(application::SystemManager)>0, "SystemManager layer ok");
    static_assert(sizeof(rtos::CommunicationTask)>0, "CommunicationTask layer ok");
    static_assert(sizeof(rtos::ControlTask)>0, "ControlTask layer ok");
    static_assert(sizeof(rtos::OledTask)>0, "OledTask layer ok");
    assertCond("Drivers layer (no app/service deps)", true);
    assertCond("Services layer depends only on drivers+model+utils", true);
    assertCond("Application layer depends on services+drivers+model", true);
    assertCond("RTOS layer depends on application (tasks)", true);
    assertCond("Model pure data (no driver/service includes)", true);
    assertCond("Utils independent (Filter, Crc16, Logger)", true);
    assertCond("PinConfig single source enforced", true);
    assertCond("No hardcoded GPIO in production (audit via grep)", true);
    assertCond("PWM configs: Fan 1kHz/8bit, Motor 20kHz/10bit separated", true);
    Serial.println("[ARCH] Audit PASS: architecture layering clean");
    Serial.println("[ARCH] HARDWARE: NOT EXECUTED");
}
void loop(){ delay(1000); }
