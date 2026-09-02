#include "CommunicationTask.h"
#include "ResponseManager.h"
#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>

namespace rtos {

application::SystemManager* CommunicationTask::sys_mgr_ = nullptr;
drivers::CanDriver* CommunicationTask::can_driver_ = nullptr;
drivers::UartDriver* CommunicationTask::uart_driver_ = nullptr;
TaskHandle_t CommunicationTask::task_handle_ = nullptr;
bool CommunicationTask::running_ = false;

bool CommunicationTask::begin(application::SystemManager* sys_mgr, drivers::CanDriver* can, drivers::UartDriver* uart) {
    sys_mgr_ = sys_mgr;
    can_driver_ = can;
    uart_driver_ = uart;
    running_ = true;
    BaseType_t result = xTaskCreate(
        taskFunction,
        "CommTask",
        4096,
        nullptr,
        3,
        &task_handle_
    );
    return result == pdPASS;
}

void CommunicationTask::end() {
    running_ = false;
    if (task_handle_) {
        vTaskDelete(task_handle_);
        task_handle_ = nullptr;
    }
}

void CommunicationTask::taskFunction(void* parameter) {
    (void)parameter;
    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(50);

    while (running_) {
        processCanMessages();
        processUartMessages();
        sendPeriodicUpdates();
        vTaskDelayUntil(&last_wake, period);
    }
    vTaskDelete(nullptr);
}

void CommunicationTask::processCanMessages() {
    if (!can_driver_) return;
    while (can_driver_->available() > 0) {
        uint32_t id; uint8_t data[8]; uint8_t len; bool ext;
        if (can_driver_->read(id, data, len, ext)) {
            // Process CAN frame
        }
    }
}

void CommunicationTask::processUartMessages() {
    if (!uart_driver_) return;
    while (uart_driver_->available() > 0) {
        uint8_t buffer[64];
        int len = uart_driver_->read(buffer, sizeof(buffer));
        if (len > 0) {
            // Process UART frame
        }
    }
}

void CommunicationTask::sendPeriodicUpdates() {
    static uint32_t last_status = 0;
    static uint32_t last_temps = 0;
    static uint32_t last_vehicle = 0;
    uint32_t now = millis();

    if (now - last_status > 1000 && sys_mgr_) {
        last_status = now;
    }
    if (now - last_temps > 500 && sys_mgr_) {
        last_temps = now;
    }
    if (now - last_vehicle > 1000 && sys_mgr_) {
        last_vehicle = now;
    }
}

} // namespace rtos
