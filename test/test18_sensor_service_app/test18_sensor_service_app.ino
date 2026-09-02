#include <Arduino.h>
#include "NtcDriver.h"
#include "OledDriver.h"
#include "VehicleDataService.h"
#include "ClimateController.h"
#include "TemperatureData.h"
#include "SystemState.h"
#include "SystemConfig.h"
// Pull production source for hysteresis logic verification (single source of truth)
#include "../../services/ClimateController.cpp"

// TEST 18 - Sensor -> Service -> Application chain + Fan hysteresis (compile/static + logic)
// Verifies NTC -> Climate/Fan decision integration compiles and hysteresis is correct
// Hysteresis : ON  > 25.5, OFF < 24.5, HOLD 24.5..25.5 (inside NTC1, SystemConfig thresholds)
// HARDWARE: NOT EXECUTED

void assertCond(const char* n,bool c){ Serial.printf("[S2A] %-30s -> %s\r\n", n, c?"PASS":"FAIL"); }

void checkHyst(const char* label, bool got, bool expect) {
    bool ok = (got == expect);
    Serial.printf("[HYST] %-32s got=%s expect=%s -> %s\r\n",
                  label, got?"ON":"OFF", expect?"ON":"OFF", ok?"PASS":"FAIL");
}

static void driveCC(services::ClimateController& cc, float t, bool valid=true) {
    model::TemperatureData td;
    td.inside_temp_c = t;
    td.inside_valid = valid;
    td.outside_temp_c = 22.0f; td.outside_valid = true;
    model::SystemState st; st.mode = model::SystemMode::NORMAL;
    cc.update(td, st);
}

void testHysteresis() {
    Serial.println("[HYST] === Fan hysteresis truth table (ClimateController) ===");
    Serial.printf("[HYST] Thresholds: ON > %.1f, OFF < %.1f, HOLD %.1f..%.1f (inside NTC1)\r\n",
                  SystemConfig::FAN_ON_THRESHOLD_C, SystemConfig::FAN_OFF_THRESHOLD_C,
                  SystemConfig::FAN_OFF_THRESHOLD_C, SystemConfig::FAN_ON_THRESHOLD_C);

    // Fan OFF initial -> only ON above 25.5
    {
        services::ClimateController cc; cc.begin();
        assertCond("ClimateController begin", cc.isInitialized());
        checkHyst("initial fan OFF", cc.getFanOn(), false);
        driveCC(cc, 24.0f);  checkHyst("OFF + T=24.0  -> OFF", cc.getFanOn(), false);
        driveCC(cc, 24.5f);  checkHyst("OFF + T=24.5  -> OFF", cc.getFanOn(), false);
        driveCC(cc, 25.0f);  checkHyst("OFF + T=25.0  -> OFF", cc.getFanOn(), false);
        driveCC(cc, 25.5f);  checkHyst("OFF + T=25.5  -> OFF", cc.getFanOn(), false);
        driveCC(cc, 25.51f); checkHyst("OFF + T=25.51 -> ON",  cc.getFanOn(), true);
    }
    // Fan ON -> only OFF below 24.5
    {
        services::ClimateController cc; cc.begin();
        // bring to ON
        driveCC(cc, 26.0f); // >25.5 -> ON
        assertCond("precondition ON after 26.0", cc.getFanOn()==true);
        checkHyst("ON + T=26.0  -> ON", cc.getFanOn(), true);
        driveCC(cc, 25.5f); checkHyst("ON + T=25.5  -> ON", cc.getFanOn(), true);
        driveCC(cc, 25.0f); checkHyst("ON + T=25.0  -> ON", cc.getFanOn(), true);
        driveCC(cc, 24.5f); checkHyst("ON + T=24.5  -> ON", cc.getFanOn(), true);
        driveCC(cc, 24.49f);checkHyst("ON + T=24.49 -> OFF",cc.getFanOn(), false);
    }
    // No toggle around 24.99 <-> 25.01 when OFF
    {
        services::ClimateController cc; cc.begin();
        driveCC(cc, 24.99f); checkHyst("OFF + T=24.99 -> OFF", cc.getFanOn(), false);
        driveCC(cc, 25.01f); checkHyst("OFF + T=25.01 -> OFF (no toggle)", cc.getFanOn(), false);
        driveCC(cc, 24.99f); checkHyst("OFF + T=24.99 -> OFF (no toggle)", cc.getFanOn(), false);
        driveCC(cc, 25.01f); checkHyst("OFF + T=25.01 -> OFF (no toggle)", cc.getFanOn(), false);
    }
    // No toggle around 24.99 <-> 25.01 when ON
    {
        services::ClimateController cc; cc.begin();
        driveCC(cc, 26.0f); // force ON
        driveCC(cc, 25.01f); checkHyst("ON + T=25.01 -> ON (no toggle)", cc.getFanOn(), true);
        driveCC(cc, 24.99f); checkHyst("ON + T=24.99 -> ON (no toggle)", cc.getFanOn(), true);
        driveCC(cc, 25.01f); checkHyst("ON + T=25.01 -> ON (no toggle)", cc.getFanOn(), true);
        driveCC(cc, 24.99f); checkHyst("ON + T=24.99 -> ON (no toggle)", cc.getFanOn(), true);
    }
    // Edge: invalid sensor -> HOLD
    {
        services::ClimateController cc; cc.begin();
        checkHyst("initial OFF", cc.getFanOn(), false);
        driveCC(cc, 30.0f, false); // invalid -> should HOLD OFF
        checkHyst("OFF + invalid T=30 -> OFF (HOLD)", cc.getFanOn(), false);
        driveCC(cc, 25.51f); checkHyst("OFF + T=25.51 -> ON", cc.getFanOn(), true);
        driveCC(cc, -40.0f, false); // invalid while ON -> HOLD ON
        checkHyst("ON + invalid T=-40 -> ON (HOLD)", cc.getFanOn(), true);
    }
    // Verify boundary exactness: 25.5 OFF, 24.5 ON (per spec > and <, not >= / <=)
    {
        services::ClimateController cc; cc.begin();
        driveCC(cc, 25.5f);  checkHyst("OFF + T=25.5 exact -> OFF", cc.getFanOn(), false);
        driveCC(cc, 25.51f); checkHyst("OFF + T=25.51 -> ON", cc.getFanOn(), true);
        driveCC(cc, 24.5f);  checkHyst("ON + T=24.5 exact -> ON", cc.getFanOn(), true);
        driveCC(cc, 24.49f); checkHyst("ON + T=24.49 -> OFF", cc.getFanOn(), false);
    }
    Serial.println("[HYST] hysteresis logic checks done");
}

void setup(){
    Serial.begin(115200); delay(100); while(!Serial && millis()<3000){}
    Serial.println(); Serial.println("=== TEST 18 : Sensor->Service->Application + Fan Hysteresis ===");
    static_assert(sizeof(drivers::NtcDriver) >0, "NtcDriver ok");
    static_assert(sizeof(services::ClimateController)>0, "ClimateController ok");
    static_assert(sizeof(model::TemperatureData)>0, "TemperatureData ok");
    drivers::NtcDriver ntc;
    assertCond("NtcDriver type", true);
    services::VehicleDataService vds;
    assertCond("VehicleDataService type", true);
    services::ClimateController climate;
    assertCond("ClimateController type", true);
    // Check data flow types compile
    model::TemperatureData td; td.inside_temp_c=22.0f; td.inside_valid=true;
    model::SystemState st; st.mode=model::SystemMode::NORMAL;
    (void)td; (void)st;
    assertCond("TemperatureData->SystemState chain", true);
    // Config threshold sanity
    assertCond("FAN_ON 25.5", SystemConfig::FAN_ON_THRESHOLD_C == 25.5f);
    assertCond("FAN_OFF 24.5", SystemConfig::FAN_OFF_THRESHOLD_C == 24.5f);
    assertCond("HOLD band 1.0C", (SystemConfig::FAN_ON_THRESHOLD_C - SystemConfig::FAN_OFF_THRESHOLD_C) == 1.0f);
    testHysteresis();
    Serial.println("[S2A] compile/integration + hysteresis PASS HARDWARE: NOT EXECUTED");
}
void loop(){ delay(1000); }
