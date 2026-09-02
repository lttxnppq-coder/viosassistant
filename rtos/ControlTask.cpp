#include "ControlTask.h"
#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>

namespace rtos {

application::SystemManager* ControlTask::sys_mgr_ = nullptr;
TaskHandle_t ControlTask::task_handle_ = nullptr;
bool ControlTask::running_ = false;

bool ControlTask::begin(application::SystemManager* sys_mgr) {
    sys_mgr_ = sys_mgr;
    running_ = true;
    BaseType_t result = xTaskCreate(
        taskFunction,
        "CtrlTask",
        4096,
        nullptr,
        4,
        &task_handle_
    );
    return result == pdPASS;
}

void ControlTask::end() {
    running_ = false;
    if (task_handle_) {
        vTaskDelete(task_handle_);
        task_handle_ = nullptr;
    }
}

void ControlTask::taskFunction(void* parameter) {
    (void)parameter;
    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(10);

    while (running_) {
        if (sys_mgr_) {
            sys_mgr_->update();
        }
        vTaskDelayUntil(&last_wake, period);
    }
    vTaskDelete(nullptr);
}

} // namespace rtos
