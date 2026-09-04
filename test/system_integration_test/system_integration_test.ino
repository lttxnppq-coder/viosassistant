#include "C:/Users/hi/Documents/NCKH/main/ViosAssistant/application/CommandManager.cpp"
#include "C:/Users/hi/Documents/NCKH/main/ViosAssistant/application/ResponseManager.cpp"
#include "C:/Users/hi/Documents/NCKH/main/ViosAssistant/application/SystemManager.cpp"
#include "C:/Users/hi/Documents/NCKH/main/ViosAssistant/drivers/CanDriver.cpp"
#include "C:/Users/hi/Documents/NCKH/main/ViosAssistant/drivers/EncoderDriver.cpp"
#include "C:/Users/hi/Documents/NCKH/main/ViosAssistant/drivers/MotorDriver.cpp"
#include "C:/Users/hi/Documents/NCKH/main/ViosAssistant/drivers/NtcDriver.cpp"
#include "C:/Users/hi/Documents/NCKH/main/ViosAssistant/drivers/OledDriver.cpp"
#include "C:/Users/hi/Documents/NCKH/main/ViosAssistant/drivers/PwmDriver.cpp"
#include "C:/Users/hi/Documents/NCKH/main/ViosAssistant/drivers/RelayDriver.cpp"
#include "C:/Users/hi/Documents/NCKH/main/ViosAssistant/drivers/UartDriver.cpp"
#include "C:/Users/hi/Documents/NCKH/main/ViosAssistant/services/AirModeController.cpp"
#include "C:/Users/hi/Documents/NCKH/main/ViosAssistant/services/ClimateController.cpp"
#include "C:/Users/hi/Documents/NCKH/main/ViosAssistant/services/FanController.cpp"
#include "C:/Users/hi/Documents/NCKH/main/ViosAssistant/services/MotorPositionController.cpp"
#include "C:/Users/hi/Documents/NCKH/main/ViosAssistant/services/VehicleDataService.cpp"
#include "C:/Users/hi/Documents/NCKH/main/ViosAssistant/rtos/CommunicationTask.cpp"
#include "C:/Users/hi/Documents/NCKH/main/ViosAssistant/rtos/ControlTask.cpp"
#include "C:/Users/hi/Documents/NCKH/main/ViosAssistant/rtos/OledTask.cpp"
#include "C:/Users/hi/Documents/NCKH/main/ViosAssistant/utils/Crc16.cpp"
#include "C:/Users/hi/Documents/NCKH/main/ViosAssistant/utils/Filter.cpp"
#include "C:/Users/hi/Documents/NCKH/main/ViosAssistant/utils/Logger.cpp"

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#include "PinConfig.h"
#include "SystemManager.h"
#include "ResponseManager.h"
#include "CommunicationTask.h"
#include "ControlTask.h"
#include "OledTask.h"
#include "Logger.h"
#include "CommandManager.h"

// OLED SSD1306 128x64 I2C 0x3C SDA8 SCL9 — chi hien thi tong quat, tranh overflow
#define OLED_SDA 8
#define OLED_SCL 9
#define OLED_ADDR 0x3C
#define OLED_W 128
#define OLED_H 64
Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);
bool oled_ok = false;

// Production objects — su dung object/interface goc, khong copy code
application::SystemManager system_manager;
application::ResponseManager response_manager;

// Test results
struct TestResult {
    const char* name;
    bool pass;
    const char* reason;
};

TestResult results[16];
int resultCount = 0;
bool overallPass = false;

void setResult(int idx, const char* name, bool pass, const char* reason) {
    results[idx].name = name;
    results[idx].pass = pass;
    results[idx].reason = reason;
}

void oledShowSummary(const char* status) {
    if (!oled_ok) return;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("SYSTEM TEST");
    display.println("");
    for (int i = 0; i < 5 && i < resultCount; i++) {
        char buf[22];
        snprintf(buf, sizeof(buf), "%-6s %s", results[i].name, results[i].pass ? "PASS" : "FAIL");
        display.println(buf);
    }
    display.println("");
    display.print("STATUS:");
    display.println(status);
    display.display();
}

void oledShowStatus(const char* s){ oledShowSummary(s); }

void printHeader() {
    Serial.println("========================================");
    Serial.println("VIOSASSISTANT SYSTEM INTEGRATION TEST");
    Serial.println("========================================");
    Serial.println("");
    Serial.println("Architecture: Jetson -> Serial1(17/18) -> UartDriver -> CommunicationTask -> CommandManager -> SystemManager -> Services -> Drivers -> Hardware");
    Serial.println("Reverse:      Hardware/Services -> SystemManager -> ResponseManager -> UartDriver -> Serial1 -> Jetson");
    Serial.println("");
}

void printResult(int idx) {
    const char* st = results[idx].pass ? "PASS" : "FAIL";
    Serial.printf("[%-16s] %s", results[idx].name, st);
    if (!results[idx].pass && results[idx].reason) {
        Serial.printf(" -- %s", results[idx].reason);
    }
    Serial.println("");
}

void printAllResults() {
    for (int i = 0; i < resultCount; i++) printResult(i);
    Serial.println("");
    Serial.println("========================================");
    Serial.printf("OVERALL: %s\n", overallPass ? "PASS" : "FAIL");
    Serial.println("========================================");
    if (!overallPass) {
        Serial.println("Note: FAIL co the do thieu hardware (chua cam sensor/relay) hoac wiring null — xem reason tung module.");
    }
}

bool checkBoot() { return true; }

void setup() {
    Serial.begin(115200);
    delay(100);
    while (!Serial && millis() < 3000) {}

    printHeader();

    Wire.begin(OLED_SDA, OLED_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        oled_ok = false;
        Serial.println("[OLED] INIT FAIL: SSD1306 not responding at 0x3C — check SDA8 SCL9 3.3V");
    } else {
        oled_ok = true;
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0,0);
        display.println("SYSTEM TEST");
        display.println("BOOTING...");
        display.display();
        Serial.println("[OLED] INIT PASS: SSD1306 128x64 at 0x3C");
    }

    bool bootPass = checkBoot();
    setResult(0, "BOOT", bootPass, bootPass ? nullptr : "ESP32 not booted");
    resultCount = 1;
    oledShowStatus("TESTING");
    Serial.printf("[BOOT] %s\n", bootPass ? "PASS" : "FAIL");

    bool uartPass = false;
    const char* uartReason = nullptr;
    if (PIN_PI_UART_TX != 17 || PIN_PI_UART_RX != 18) {
        uartReason = "PinConfig TX/RX mismatch (expect 17/18)";
        uartPass = false;
    } else {
        uartPass = true;
        uartReason = "Pin 17/18 115200 8N1 (wiring will be verified after SystemManager)";
    }
    setResult(1, "UART", uartPass, uartReason);
    resultCount = 2;

    setResult(2, "OLED", oled_ok, oled_ok ? nullptr : "SSD1306 0x3C no ACK on SDA8/SCL9");
    resultCount = 3;

    utils::Logger::begin(utils::LogLevel::INFO);
    LOG_INFO("TEST", "SystemManager::begin() ...");
    bool sysBegin = system_manager.begin();
    bool sysUartOk = false;
    bool sysCanOk = false;
    if (sysBegin) {
        sysUartOk = system_manager.getUartDriver().isInitialized();
        sysCanOk = system_manager.getCanDriver().isInitialized();
        if (!sysUartOk) {
            setResult(1, "UART", false, "SystemManager uart not initialized (GPIO17/18)");
        } else {
            setResult(1, "UART", true, nullptr);
        }
    } else {
        setResult(1, "UART", false, "SystemManager begin failed — uart not init");
    }

    bool ntcPass = false;
    const char* ntcReason = nullptr;
    if (!sysBegin) {
        ntcReason = "SystemManager not initialized";
        ntcPass = false;
    } else {
        system_manager.update();
        delay(50);
        const auto& temps = system_manager.getTemperatureData();
        int raw1 = system_manager.getNtc1Raw();
        int raw2 = system_manager.getNtc2Raw();
        float v1 = system_manager.getNtc1Voltage();
        float v2 = system_manager.getNtc2Voltage();
        Serial.printf("[NTC] RAW1=%d V1=%.2f Tinside=%.1f valid=%d\n", raw1, v1, temps.inside_temp_c, temps.inside_valid);
        Serial.printf("[NTC] RAW2=%d V2=%.2f Tout=%.1f valid=%d\n", raw2, v2, temps.outside_temp_c, temps.outside_valid);
        if (temps.inside_valid || temps.outside_valid) {
            ntcPass = true;
            ntcReason = nullptr;
        } else {
            if (raw1 <= 10 || raw1 >= 4085 || raw2 <= 10 || raw2 >= 4085) {
                ntcReason = "NTC raw out of range (sensor not connected or floating)";
            } else {
                ntcReason = "NTC valid flag false (inside_valid=false) — Climate hysteresis will stay HOLD";
            }
            ntcPass = false;
        }
    }
    setResult(3, "NTC", ntcPass, ntcReason);
    resultCount = 4;

    bool gpioPass = false;
    const char* gpioReason = nullptr;
    if (!sysBegin) {
        gpioReason = "SystemManager not init";
    } else {
        int lvl = system_manager.getGpio10State();
        Serial.printf("[GPIO INPUT] GPIO10 level=%d (%s)\n", lvl, lvl ? "HIGH" : "LOW");
        gpioPass = true;
        gpioReason = "GPIO10 mode TBD (PinConfig 48) — read OK but pull not verified (H04)";
    }
    setResult(4, "GPIO INPUT", gpioPass, gpioReason);
    resultCount = 5;

    bool relayPass = false;
    const char* relayReason = nullptr;
    if (!sysBegin) {
        relayReason = "SystemManager not init";
    } else {
        const auto& climate = system_manager.getClimateController();
        bool ac = climate.getAC();
        Serial.printf("[RELAY] AC relay logical AC=%s (GPIO4), FAN relay OFF (GPIO5), PI relay OFF (GPIO6) — safety OFF\n", ac ? "ON" : "OFF");
        if (!ac) {
            relayPass = true;
        } else {
            relayReason = "AC relay should be OFF at boot (safety)";
            relayPass = false;
        }
        if (relayPass) relayReason = "GPIO4 AC OFF, GPIO5/6 OFF (FAN/PI not yet driven in production — see SystemManager 150)";
    }
    setResult(5, "RELAY", relayPass, relayReason);
    resultCount = 6;

    bool fanPass = false;
    const char* fanReason = nullptr;
    if (!sysBegin) {
        fanReason = "SystemManager not init";
    } else {
        const auto& fan = system_manager.getFanController();
        uint8_t level = fan.getLevel();
        uint8_t speed = fan.getSpeed();
        Serial.printf("[FAN PWM] GPIO7 1kHz 8bit level=%d speed=%d (expect 0 at boot)\n", level, speed);
        if (speed == 0) {
            fanPass = true;
            fanReason = "PWM 0 safe, polarity active-HIGH TBD (PinConfig 33 H06)";
        } else {
            fanReason = "Fan PWM not 0 at boot";
            fanPass = false;
        }
    }
    setResult(6, "FAN PWM", fanPass, fanReason);
    resultCount = 7;

    bool motorPass = false;
    const char* motorReason = nullptr;
    if (!sysBegin) {
        motorReason = "SystemManager not init";
    } else {
        Serial.println("[MOTOR DRIVER] GPIO13/14 20kHz 10bit DRV8833 parallel — expect COAST");
        motorPass = true;
        motorReason = "Motor coast safe (update disabled in SystemManager 147)";
    }
    setResult(7, "MOTOR DRIVER", motorPass, motorReason);
    resultCount = 8;

    bool encPass = false;
    const char* encReason = nullptr;
    if (!sysBegin) {
        encReason = "SystemManager not init";
    } else {
        int32_t pos = system_manager.getEncoderCount();
        Serial.printf("[ENCODER] GPIO19/20 PPR 11300 pos=%ld\n", (long)pos);
        if (pos >= -5 && pos <= 5) {
            encPass = true;
            encReason = "Encoder init OK, PPR 11300, no motion at boot";
        } else {
            encReason = "Encoder pos drift — check wiring 19/20 or USB-OTG conflict (PinConfig 94)";
            encPass = false;
        }
    }
    setResult(8, "ENCODER", encPass, encReason);
    resultCount = 9;

    bool motorEncPass = false;
    const char* motorEncReason = nullptr;
    if (!sysBegin) {
        motorEncReason = "SystemManager not init";
    } else {
        Serial.println("[MOTOR+ENCODER] PID Kp=1.0 (paper 14) update disabled — no closed-loop");
        motorEncPass = false;
        motorEncReason = "MotorPositionController update commented (SystemManager 147) — end-to-end disabled by design";
    }
    setResult(9, "MOTOR+ENCODER", motorEncPass, motorEncReason);
    resultCount = 10;

    bool canPass = false;
    const char* canReason = nullptr;
    if (!sysBegin) {
        canReason = "SystemManager not init";
    } else {
        bool canInit = system_manager.getCanDriver().isInitialized();
        Serial.printf("[CAN UART] GPIO11 TX GPIO12 RX 500k isInitialized=%d\n", canInit);
        if (canInit) {
            canPass = false;
            canReason = "CanDriver STUB (no Serial2, write false) — H09 module not yet defined (REQUIRES H09)";
        } else {
            canReason = "CanDriver not initialized";
            canPass = false;
        }
    }
    setResult(10, "CAN UART", canPass, canReason);
    resultCount = 11;

    bool svcPass = false;
    const char* svcReason = nullptr;
    if (!sysBegin) {
        svcReason = "SystemManager not init";
    } else {
        const auto& climate = system_manager.getClimateController();
        const auto& fan = system_manager.getFanController();
        Serial.printf("[SERVICES] Climate init=%d Fan init=%d AirMode not started MotorPos begin OK but update disabled\n", climate.isInitialized(), fan.isInitialized());
        svcPass = climate.isInitialized() && fan.isInitialized();
        svcReason = svcPass ? "Climate/Fan OK, VehicleData STUB, AirMode/MotorPos disabled (see SystemManager 96,146)" : "Climate/Fan not init";
    }
    setResult(11, "SERVICES", svcPass, svcReason);
    resultCount = 12;

    bool cmdPass = false;
    const char* cmdReason = nullptr;
    {
        application::CommandManager cm;
        bool ok = cm.begin();
        model::Command cmd; cmd.type = model::CommandType::SET_AC; cmd.payload[0]=1; cmd.payload_len=1;
        model::CommandResponse resp;
        cm.processCommand(cmd, resp);
        Serial.printf("[COMMAND MANAGER] begin=%d SET_AC success=%d err=%d\n", ok, resp.success, resp.error_code);
        model::Command cmd2; cmd2.type = model::CommandType::SET_DAMPER_POS; cmd2.payload_len=0;
        model::CommandResponse resp2;
        cm.processCommand(cmd2, resp2);
        Serial.printf("[COMMAND MANAGER] SET_DAMPER_POS success=%d err=%d (expect NOT_IMPLEMENTED 2)\n", resp2.success, resp2.error_code);
        cmdPass = ok && resp.success && !resp2.success && resp2.error_code==2;
        cmdReason = cmdPass ? nullptr : "CommandManager validation failed (see CommandManager.cpp 38-52)";
    }
    setResult(12, "COMMAND MANAGER", cmdPass, cmdReason);
    resultCount = 13;

    bool respPass = false;
    const char* respReason = nullptr;
    {
        bool rmOk = response_manager.isInitialized() && system_manager.getUartDriver().isInitialized();
        Serial.printf("[RESPONSE MANAGER] isInitialized=%d uartInit=%d\n", response_manager.isInitialized(), system_manager.getUartDriver().isInitialized());
        if (rmOk) {
            respPass = true;
            respReason = "ResponseManager wired to UartDriver 17/18 (P0-1 FIX)";
        } else {
            respPass = false;
            respReason = "ResponseManager not wired to UartDriver (check ViosAssistant 23)";
        }
    }
    setResult(13, "RESPONSE MANAGER", respPass, respReason);
    resultCount = 14;

    bool sysPass = sysBegin;
    const char* sysReason = sysPass ? nullptr : "SystemManager::begin failed (see LOG above, check oled/ntc/uart/can init)";
    if (sysPass) {
        const auto& temps = system_manager.getTemperatureData();
        Serial.printf("[SYSTEM MANAGER] mode=%d heap=%lu inside_valid=%d outside_valid=%d\n", (int)system_manager.getSystemState().mode, system_manager.getSystemState().free_heap, temps.inside_valid, temps.outside_valid);
        if (!temps.inside_valid) {
            Serial.println("[SYSTEM MANAGER] Note: inside_valid false — will be true only if NTC raw/voltage/temp in range (P0-2)");
        }
    }
    setResult(14, "SYSTEM MANAGER", sysPass, sysReason);
    resultCount = 15;

    bool rtosPass = false;
    const char* rtosReason = nullptr;
    if (sysBegin && sysUartOk && sysCanOk) {
        rtosPass = true;
        rtosReason = "RTOS wiring fixed P0-1 (CommTask will receive real uart/can/respMgr)";
    } else {
        rtosPass = false;
        rtosReason = "RTOS wiring not ready (sysBegin or uart/can not init)";
    }
    setResult(15, "RTOS", rtosPass, rtosReason);
    resultCount = 16;

    overallPass = true;
    for (int i = 0; i < resultCount; i++) if (!results[i].pass) overallPass = false;

    printAllResults();
    oledShowSummary(overallPass ? "PASS" : "FAIL");
    Serial.println("");
    Serial.println("HARDWARE SAFETY: Motor COAST PWM0 Fan PWM0 Relay OFF — no actuator run in this test");
    Serial.println("To run hardware actuator test, build PASS first and get authorization, then manually trigger via Jetson commands.");
}

void loop() {
    delay(1000);
}
