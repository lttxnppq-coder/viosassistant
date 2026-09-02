#pragma once

#include <cstdint>
#include "SystemManager.h"
#include "ClimateController.h"
#include "FanController.h"
#include "AirModeController.h"
#include "MotorPositionController.h"

namespace rtos {

class ControlTask {
public:
    static bool begin(application::SystemManager* sys_mgr);
    static void end();
    static void taskFunction(void* parameter);
    static bool isRunning() { return running_; }

private:
    static application::SystemManager* sys_mgr_;
    static TaskHandle_t task_handle_;
    static bool running_;
};

} // namespace rtos
