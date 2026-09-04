#include "OledTask.h"
#include "PinConfig.h"
#include "CommunicationTask.h"
#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>

namespace rtos {

application::SystemManager* OledTask::sys_mgr_ = nullptr;
drivers::OledDriver* OledTask::oled_driver_ = nullptr;
TaskHandle_t OledTask::task_handle_ = nullptr;
bool OledTask::running_ = false;
uint8_t OledTask::currentPage_ = 0;
uint8_t OledTask::lastButtonState_ = HIGH;
uint32_t OledTask::lastDebounceMs_ = 0;
uint32_t OledTask::lastButtonPressMs_ = 0;

bool OledTask::begin(application::SystemManager* sys_mgr, drivers::OledDriver* oled) {
    sys_mgr_ = sys_mgr;
    oled_driver_ = oled;
    // Button GPIO 35 active LOW with INPUT_PULLUP, debounce 30-50ms
    pinMode(PIN_OLED_BUTTON, INPUT_PULLUP);
    lastButtonState_ = digitalRead(PIN_OLED_BUTTON);
    currentPage_ = 0;
    lastDebounceMs_ = millis();
    running_ = true;
    BaseType_t result = xTaskCreate(
        taskFunction,
        "OledTask",
        4096,
        nullptr,
        2,
        &task_handle_
    );
    return result == pdPASS;
}

void OledTask::end() {
    running_ = false;
    if (task_handle_) {
        vTaskDelete(task_handle_);
        task_handle_ = nullptr;
    }
}

static const char* airModeStr(services::ClimateController::AirMode m) {
    switch (m) {
        case services::ClimateController::AirMode::VENT: return "FACE";
        case services::ClimateController::AirMode::FLOOR: return "FOOT";
        case services::ClimateController::AirMode::DEFROST: return "DEFROST";
        case services::ClimateController::AirMode::BI_LEVEL: return "BI_LVL";
        case services::ClimateController::AirMode::MIX: return "MIX";
        case services::ClimateController::AirMode::FLOOR_DEFROST: return "F/D";
        default: return "UNKNOWN";
    }
}

static const char* errorStr(model::ErrorCode e) {
    switch (e) {
        case model::ErrorCode::NONE: return "NONE";
        case model::ErrorCode::SENSOR_FAILURE: return "SENSOR";
        case model::ErrorCode::COMMUNICATION_ERROR: return "COMM";
        case model::ErrorCode::ACTUATOR_FAILURE: return "ACTUATOR";
        case model::ErrorCode::OVER_TEMPERATURE: return "OVER TEMP";
        case model::ErrorCode::UNDER_VOLTAGE: return "UNDER VOLT";
        case model::ErrorCode::OVER_CURRENT: return "OVER CURR";
        case model::ErrorCode::INVALID_CONFIG: return "CONFIG";
        case model::ErrorCode::WATCHDOG_TIMEOUT: return "WDOG";
        default: return "UNKNOWN";
    }
}

void OledTask::checkButton() {
    int reading = digitalRead(PIN_OLED_BUTTON);
    uint32_t now = millis();
    // Debounce 40ms
    if (reading != lastButtonState_) {
        lastDebounceMs_ = now;
    }
    if ((now - lastDebounceMs_) > 40) {
        // Stable state
        static int lastStable = HIGH;
        if (reading != lastStable) {
            lastStable = reading;
            if (reading == LOW) { // pressed active LOW
                // One press -> one page
                uint8_t oldPage = currentPage_;
                currentPage_ = (currentPage_ + 1) % 5;
                lastButtonPressMs_ = now;
                Serial.printf("[OLED BUTTON] PAGE: %u -> %u\n", oldPage, currentPage_);
            }
        }
    }
    lastButtonState_ = reading;
}

void OledTask::taskFunction(void* parameter) {
    (void)parameter;
    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(100);
    uint8_t tick = 0;

    while (running_) {
        checkButton(); // non-blocking, 40ms debounce

        if (oled_driver_ && oled_driver_->isInitialized() && sys_mgr_) {
            const auto& state = sys_mgr_->getSystemState();
            const auto& temps = sys_mgr_->getTemperatureData();
            bool hasError = (state.error != model::ErrorCode::NONE) || (!temps.inside_valid && state.mode != model::SystemMode::INITIALIZING);

            // Error has priority: show error page for 2s then resume (keep original behavior)
            if (hasError && (tick % 30 < 20)) {
                oled_driver_->clear();
                char buf[32];
                oled_driver_->setTextSize(1);
                oled_driver_->setCursor(0, 0);
                oled_driver_->drawString(0, 0, "!!! ERROR !!!");
                oled_driver_->drawString(0, 10, errorStr(state.error));
                if (!temps.inside_valid) {
                    snprintf(buf, sizeof(buf), "NTC1 INVALID");
                    oled_driver_->drawString(0, 20, buf);
                    snprintf(buf, sizeof(buf), "RAW:%d", sys_mgr_->getNtc1Raw());
                    oled_driver_->drawString(0, 30, buf);
                } else {
                    snprintf(buf, sizeof(buf), "CODE:%d", (int)state.error);
                    oled_driver_->drawString(0, 20, buf);
                }
                snprintf(buf, sizeof(buf), "Heap:%lu Err:%d", state.free_heap/1024, (int)state.error);
                oled_driver_->drawString(0, 50, buf);
                oled_driver_->display();
            } else {
                oled_driver_->clear();
                switch (currentPage_) {
                    case 0: drawMainScreen(); break;
                    case 1: drawClimateScreen(); break;
                    case 2: drawVehicleScreen(); break;
                    case 3: drawCommCheckScreen(); break;
                    case 4: drawButtonDebugScreen(); break;
                    default: drawMainScreen(); break;
                }
                oled_driver_->display();
            }
            tick++;
        }
        vTaskDelayUntil(&last_wake, period);
    }
    vTaskDelete(nullptr);
}

void OledTask::drawMainScreen() {
    if (!sys_mgr_ || !oled_driver_) return;
    char buf[32];
    const auto& temps = sys_mgr_->getTemperatureData();
    const auto& state = sys_mgr_->getSystemState();
    const auto& climate = sys_mgr_->getClimateController();
    const auto& fan = sys_mgr_->getFanController();

    oled_driver_->setTextSize(1);
    // SYSTEM — 128x64, 21 chars/line, 8 lines max, use y 0,9,18,28,38,48,56
    oled_driver_->drawString(0, 0, "SYSTEM");
    oled_driver_->drawString(0, 9, "----------------");
    if (temps.inside_valid) snprintf(buf, sizeof(buf), "TEMP:%.1fC", temps.inside_temp_c);
    else snprintf(buf, sizeof(buf), "TEMP:N/A");
    oled_driver_->drawString(0, 18, buf);
    snprintf(buf, sizeof(buf), "AC:%s", climate.getAC() ? "ON" : "OFF");
    oled_driver_->drawString(64, 18, buf);
    uint8_t lvl = fan.getLevel();
    if (lvl==0) snprintf(buf, sizeof(buf), "FAN:OFF");
    else snprintf(buf, sizeof(buf), "FAN:L%d", lvl);
    oled_driver_->drawString(0, 28, buf);
    snprintf(buf, sizeof(buf), "MODE:%s", airModeStr(climate.getAirMode()));
    oled_driver_->drawString(64, 28, buf);
    int32_t enc = sys_mgr_->getEncoderCount();
    float deg = (enc / 11300.0f) * 360.0f;
    snprintf(buf, sizeof(buf), "MOTOR:%.1f", deg);
    oled_driver_->drawString(0, 38, buf);
    // STATUS instead of ST/Heap to match spec, keep short
    const char* status = (state.error==model::ErrorCode::NONE) ? "OK" : "ERR";
    snprintf(buf, sizeof(buf), "STATUS:%s", status);
    oled_driver_->drawString(0, 48, buf);
    snprintf(buf, sizeof(buf), "PAGE:%d/5", currentPage_+1);
    oled_driver_->drawString(64, 48, buf);
    // Keep heap on last line but short
    snprintf(buf, sizeof(buf), "Heap:%lu", state.free_heap/1024);
    oled_driver_->drawString(0, 56, buf);
}

void OledTask::drawClimateScreen() {
    if (!sys_mgr_ || !oled_driver_) return;
    char buf[32];
    const auto& temps = sys_mgr_->getTemperatureData();

    oled_driver_->setTextSize(1);
    oled_driver_->drawString(0, 0, "SENSORS");
    oled_driver_->drawString(0, 9, "----------------");
    if (temps.inside_valid) snprintf(buf, sizeof(buf), "NTC1:%.1fC", temps.inside_temp_c);
    else snprintf(buf, sizeof(buf), "NTC1:N/A");
    oled_driver_->drawString(0, 18, buf);
    snprintf(buf, sizeof(buf), "RAW1:%d", sys_mgr_->getNtc1Raw());
    oled_driver_->drawString(64, 18, buf);
    snprintf(buf, sizeof(buf), "V1:%.2fV", sys_mgr_->getNtc1Voltage());
    oled_driver_->drawString(0, 28, buf);
    if (temps.outside_valid) snprintf(buf, sizeof(buf), "NTC2:%.1fC", temps.outside_temp_c);
    else snprintf(buf, sizeof(buf), "NTC2:N/A");
    oled_driver_->drawString(0, 38, buf);
    snprintf(buf, sizeof(buf), "RAW2:%d", sys_mgr_->getNtc2Raw());
    oled_driver_->drawString(64, 38, buf);
    snprintf(buf, sizeof(buf), "V2:%.2fV", sys_mgr_->getNtc2Voltage());
    oled_driver_->drawString(0, 48, buf);
    int inp = sys_mgr_->getGpio10State();
    snprintf(buf, sizeof(buf), "INPUT:%s", inp ? "ON" : "OFF");
    oled_driver_->drawString(0, 56, buf);
}

void OledTask::drawVehicleScreen() {
    if (!sys_mgr_ || !oled_driver_) return;
    char buf[32];
    const auto& fan = sys_mgr_->getFanController();
    int32_t enc = sys_mgr_->getEncoderCount();
    float pos = (enc / 11300.0f) * 360.0f;
    oled_driver_->setTextSize(1);
    oled_driver_->drawString(0, 0, "MOTOR");
    oled_driver_->drawString(0, 9, "----------------");
    snprintf(buf, sizeof(buf), "POS:%.1f", pos);
    oled_driver_->drawString(0, 18, buf);
    // TARGET - from MotorPositionController if available, else N/A (keep short)
    oled_driver_->drawString(0, 28, "TARGET:N/A");
    snprintf(buf, sizeof(buf), "PWM:%d", fan.getSpeed());
    oled_driver_->drawString(0, 38, buf);
    snprintf(buf, sizeof(buf), "DIR:%s", (enc>=0)?"CW":"CCW");
    oled_driver_->drawString(64, 38, buf);
    snprintf(buf, sizeof(buf), "ENC:%ld", (long)enc);
    oled_driver_->drawString(0, 48, buf);
    snprintf(buf, sizeof(buf), "PPR:11300");
    oled_driver_->drawString(64, 48, buf);
    snprintf(buf, sizeof(buf), "FAN L:%d", fan.getLevel());
    oled_driver_->drawString(0, 56, buf);
}

void OledTask::drawCommCheckScreen() {
    if (!sys_mgr_ || !oled_driver_) return;
    char buf[32];
    auto diag = CommunicationTask::getDiag();
    oled_driver_->setTextSize(1);
    oled_driver_->drawString(0, 0, "COMM CHECK");
    oled_driver_->drawString(0, 9, "----------------");
    // RX/TX counters — real, not fake; show N/A if never
    if (diag.hasRx) snprintf(buf, sizeof(buf), "RX:%lu", (unsigned long)diag.rxCount);
    else snprintf(buf, sizeof(buf), "RX:N/A");
    oled_driver_->drawString(0, 18, buf);
    if (diag.hasTx) snprintf(buf, sizeof(buf), "TX:%lu", (unsigned long)diag.txCount);
    else snprintf(buf, sizeof(buf), "TX:N/A");
    oled_driver_->drawString(64, 18, buf);
    if (diag.hasRx) snprintf(buf, sizeof(buf), "CMD:0x%02X", diag.lastRxId);
    else snprintf(buf, sizeof(buf), "CMD:N/A");
    oled_driver_->drawString(0, 28, buf);
    if (diag.hasRx) snprintf(buf, sizeof(buf), "LEN:%u", diag.lastRxLen);
    else snprintf(buf, sizeof(buf), "LEN:N/A");
    oled_driver_->drawString(64, 28, buf);
    if (!diag.hasRx) snprintf(buf, sizeof(buf), "CRC:N/A");
    else snprintf(buf, sizeof(buf), "CRC:%s", diag.lastRxCrcOk ? "OK" : "ERR");
    oled_driver_->drawString(0, 38, buf);
    if (!diag.hasRx) snprintf(buf, sizeof(buf), "LAST:N/A");
    else snprintf(buf, sizeof(buf), "LAST:%s", diag.lastRxOk ? "OK" : "ERR");
    oled_driver_->drawString(64, 38, buf);
    // Extra: show last TX id
    if (diag.hasTx) snprintf(buf, sizeof(buf), "TXID:0x%02X", diag.lastTxId);
    else snprintf(buf, sizeof(buf), "TXID:N/A");
    oled_driver_->drawString(0, 48, buf);
    // Page indicator
    snprintf(buf, sizeof(buf), "PAGE:%d/5", currentPage_+1);
    oled_driver_->drawString(0, 56, buf);
}

void OledTask::drawButtonDebugScreen() {
    if (!sys_mgr_ || !oled_driver_) return;
    char buf[32];
    auto diag = CommunicationTask::getDiag();
    const auto& state = sys_mgr_->getSystemState();
    oled_driver_->setTextSize(1);
    oled_driver_->drawString(0, 0, "BUTTON / DEBUG");
    oled_driver_->drawString(0, 9, "----------------");
    snprintf(buf, sizeof(buf), "PAGE:%d/5", currentPage_+1);
    oled_driver_->drawString(0, 18, buf);
    int btn = digitalRead(PIN_OLED_BUTTON);
    snprintf(buf, sizeof(buf), "BUTTON:%s", btn==LOW ? "PRESSED" : "READY");
    oled_driver_->drawString(0, 28, buf);
    if (!diag.hasRx) snprintf(buf, sizeof(buf), "LAST RX:N/A");
    else snprintf(buf, sizeof(buf), "LAST RX:%s", diag.lastRxOk ? "OK" : "ERR");
    oled_driver_->drawString(0, 38, buf);
    if (!diag.hasTx) snprintf(buf, sizeof(buf), "LAST TX:N/A");
    else snprintf(buf, sizeof(buf), "LAST TX:%s", diag.lastTxOk ? "OK" : "ERR");
    oled_driver_->drawString(0, 48, buf);
    snprintf(buf, sizeof(buf), "ERROR:%d", (int)diag.lastError);
    oled_driver_->drawString(0, 56, buf);
    (void)state;
}

} // namespace rtos
