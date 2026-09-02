#include <Arduino.h>
// Include production source (duoc phep qua -I root)
#include "../../drivers/CanDriver.h"
#include "../../drivers/CanDriver.cpp"
#include "../../drivers/UartDriver.h"
#include "../../drivers/UartDriver.cpp"
#include "../../services/VehicleDataService.h"
#include "../../services/VehicleDataService.cpp"
#include "../../model/VehicleData.h"

// TEST 12 - VehicleDataService (logic + contract)
// Ghi nhan production issue (bao cao, khong sua):
//   - CanDriver la STUB (begin chi set initialized_; write true; read false; available 0)
//   - VehicleDataService::parseCanFrame()/parseUartFrame() return false ngay (stub)
//   => update() khong co du lieu that, data_valid= false. Song loi to trong build.

// Vì CanDriver/UartDriver deu la stub, ta dung mot ban ghi diban cho UartDriver
// de kiem tra T: service nhan du lieu qua UartDriver.write? (khong duoc - stub)
// Test nay chi kiem tra contract/compile.

void setup() {
    Serial.begin(115200);
    delay(100);
    while (!Serial && millis() < 3000) {}

    Serial.println();
    Serial.println("=== TEST 12 : VehicleDataService (compile/contract) ===");

    drivers::CanDriver can;
    drivers::UartDriver uart;
    services::VehicleDataService svc;

    bool ok = svc.begin(&can, &uart);
    Serial.printf("[VDS] begin() = %s\r\n", ok ? "true" : "false");
    Serial.printf("[VDS] isInitialized = %s\r\n", svc.isInitialized() ? "true" : "false");

    // update() -> goi can.read(0) / uart.read(-1) stub; con can.write(0x7DF)
    svc.update();
    svc.update();

    const model::VehicleData& d = svc.getData();
    Serial.printf("[VDS] data_valid = %s\r\n", d.data_valid ? "true" : "false");
    Serial.printf("[VDS] speed=%.1f rpm=%.0f coolant=%.1f battery=%.1f\r\n",
                  d.vehicle_speed_kmh, d.engine_rpm, d.coolant_temp_c, d.battery_voltage_v);

    // parse Can/Uart frame truc tiep (stub -> false)
    uint8_t data8[8] = {0};
    bool pcan = svc.parseCanFrame(0x100, data8, 8);
    bool puart = svc.parseUartFrame(data8, 8);
    Serial.printf("[VDS] parseCanFrame = %s | parseUartFrame = %s\r\n",
                  pcan ? "true" : "false", puart ? "true" : "false");

    Serial.println("[PROD-NOTE] CanDriver STUB + parse stubs -> update khong co data that");
    Serial.println("[VDS] done (build/static - HARDWARE NOT TESTED)");
}

void loop() {
    delay(1000);
}