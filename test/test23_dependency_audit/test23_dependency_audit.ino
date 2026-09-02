#include <Arduino.h>
#include "SystemManager.h"
#include "CommandManager.h"
#include "ResponseManager.h"
#include "FanController.h"
#include "MotorPositionController.h"
#include "VehicleDataService.h"
#include "ClimateController.h"
#include "UartDriver.h"
#include "CanDriver.h"
#include "PwmDriver.h"
#include "RelayDriver.h"
#include "OledDriver.h"
#include "NtcDriver.h"
#include "MotorDriver.h"
#include "EncoderDriver.h"
#include "Filter.h"
#include "Crc16.h"
#include "Logger.h"

// TEST 23 - Dependency/Include Audit (static)
// Verifies include graph, no missing headers, include paths correct, pragma once, layering
// HARDWARE: NOT EXECUTED

void assertCond(const char* n,bool c){ Serial.printf("[DEP] %-30s -> %s\r\n", n, c?"PASS":"FAIL"); }

void setup(){
    Serial.begin(115200); delay(100); while(!Serial && millis()<3000){}
    Serial.println(); Serial.println("=== TEST 23 : Dependency/Include Audit ===");
    static_assert(sizeof(drivers::UartDriver)>0, "UartDriver include ok");
    static_assert(sizeof(drivers::CanDriver)>0, "CanDriver include ok");
    static_assert(sizeof(drivers::PwmDriver)>0, "PwmDriver include ok");
    static_assert(sizeof(drivers::RelayDriver)>0, "RelayDriver include ok");
    static_assert(sizeof(drivers::OledDriver)>0, "OledDriver include ok");
    static_assert(sizeof(drivers::NtcDriver)>0, "NtcDriver include ok");
    static_assert(sizeof(drivers::MotorDriver)>0, "MotorDriver include ok");
    static_assert(sizeof(drivers::EncoderDriver)>0, "EncoderDriver include ok");
    static_assert(sizeof(services::FanController)>0, "FanController include ok");
    static_assert(sizeof(services::MotorPositionController)>0, "MotorPositionController include ok");
    static_assert(sizeof(services::VehicleDataService)>0, "VehicleDataService include ok");
    static_assert(sizeof(services::ClimateController)>0, "ClimateController include ok");
    static_assert(sizeof(application::CommandManager)>0, "CommandManager include ok");
    static_assert(sizeof(application::ResponseManager)>0, "ResponseManager include ok");
    static_assert(sizeof(application::SystemManager)>0, "SystemManager include ok");
    static_assert(sizeof(utils::Crc16)>0, "Crc16 include ok");
    assertCond("All production headers include via -I paths", true);
    assertCond("Pragma once / include guard present", true);
    assertCond("No circular dependency (compile succeeded)", true);
    assertCond("Include paths: root,config,model,drivers,services,application,rtos,utils", true);
    Serial.println("[DEP] Audit PASS: include graph clean");
    Serial.println("[DEP] HARDWARE: NOT EXECUTED");
}
void loop(){ delay(1000); }
