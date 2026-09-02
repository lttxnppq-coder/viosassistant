#include <Arduino.h>
#include "CommandManager.h"
#include "Command.h"
#include "../../application/CommandManager.cpp"

// TEST 15 - CommandManager (software module)
// Covers: begin, queueCommand (16 depth), processCommand, lastCommand, clearQueue, overflow
// HARDWARE: NOT EXECUTED

void assertCond(const char* name, bool cond) {
    Serial.printf("[CMD] %-26s -> %s\r\n", name, cond ? "PASS" : "FAIL");
}

void testCommandManager() {
    application::CommandManager cm;
    assertCond("begin", cm.begin() && cm.isInitialized());
    // queue one
    model::Command c1; c1.type = model::CommandType::SET_FAN_SPEED; c1.payload[0]=42; c1.payload_len=1;
    assertCond("queue 1", cm.queueCommand(c1));
    // fill to 16
    for (int i=1;i<16;i++) {
        model::Command c; c.type=model::CommandType::SET_TEMPERATURE; c.payload[0]=i;
        bool ok = cm.queueCommand(c);
        if (!ok) Serial.printf("[CMD] queue %d failed unexpectedly\r\n", i);
    }
    // 17th should fail (overflow)
    model::Command cx; cx.type=model::CommandType::SET_AC;
    bool overflow = !cm.queueCommand(cx);
    assertCond("queue overflow 17th should fail", overflow);
    // process
    model::CommandResponse rsp;
    bool pr = cm.processCommand(c1, rsp);
    assertCond("processCommand returns true", pr);
    assertCond("response success", rsp.success);
    assertCond("lastCommand == SET_FAN_SPEED", cm.getLastCommand()==model::CommandType::SET_FAN_SPEED);
    // last command time should be >0
    assertCond("lastCommandTime !=0", cm.getLastCommandTime()!=0);
    cm.clearQueue();
    // after clear, queue should accept again
    model::Command c2; c2.type=model::CommandType::REQUEST_STATUS;
    assertCond("queue after clear", cm.queueCommand(c2));
}

void setup() {
    Serial.begin(115200);
    delay(100);
    while (!Serial && millis() < 3000) {}
    Serial.println();
    Serial.println("=== TEST 15 : CommandManager (software) ===");
    testCommandManager();
    Serial.println("[CMD] done (build/static) HARDWARE: NOT EXECUTED");
}
void loop(){ delay(1000); }
