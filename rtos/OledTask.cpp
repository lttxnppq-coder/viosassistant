#include "OledTask.h"
#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>

namespace rtos {

application::SystemManager* OledTask::sys_mgr_ = nullptr;
drivers::OledDriver* OledTask::oled_driver_ = nullptr;
TaskHandle_t OledTask::task_handle_ = nullptr;
bool OledTask::running_ = false;

bool OledTask::begin(application::SystemManager* sys_mgr, drivers::OledDriver* oled) {
    sys_mgr_ = sys_mgr;
    oled_driver_ = oled;
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

void OledTask::taskFunction(void* parameter) {
    (void)parameter;
    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(100);

    while (running_) {
        if (oled_driver_ && oled_driver_->isInitialized()) {
            oled_driver_->clear();
            drawMainScreen();
            oled_driver_->display();
        }
        vTaskDelayUntil(&last_wake, period);
    }
    vTaskDelete(nullptr);
}

void OledTask::drawMainScreen() {
    if (!sys_mgr_ || !oled_driver_) return;

    char buf[32];
    const auto& temps = sys_mgr_->getTemperatureData();
    const auto& vehicle = sys_mgr_->getVehicleData();
    const auto& state = sys_mgr_->getSystemState();

    oled_driver_->setTextSize(1);
    oled_driver_->setTextColor(SSD1306_WHITE);
    oled_driver_->setCursor(0, 0);

    snprintf(buf, sizeof(buf), "ViosAssistant v0.1");
    oled_driver_->drawString(0, 0, buf);

    snprintf(buf, sizeof(buf), "In:%.1f Out:%.1f", temps.inside_temp_c, temps.outside_temp_c);
    oled_driver_->drawString(0, 12, buf);

    snprintf(buf, sizeof(buf), "Eva:%.1f Amb:%.1f", temps.evaporator_temp_c, temps.ambient_temp_c);
    oled_driver_->drawString(0, 24, buf);

    snprintf(buf, sizeof(buf), "Spd:%.0f RPM:%.0f", vehicle.vehicle_speed_kmh, vehicle.engine_rpm);
    oled_driver_->drawString(0, 36, buf);

    snprintf(buf, sizeof(buf), "Bat:%.1fV Mode:%d", vehicle.battery_voltage_v, (int)state.mode);
    oled_driver_->drawString(0, 48, buf);

    snprintf(buf, sizeof(buf), "Heap:%lukB Err:%d", state.free_heap / 1024, (int)state.error);
    oled_driver_->drawString(0, 56, buf);
}

} // namespace rtos
