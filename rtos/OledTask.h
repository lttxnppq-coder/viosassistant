#pragma once

#include <cstdint>
#include "SystemManager.h"
#include "OledDriver.h"

namespace rtos {

class OledTask {
public:
    static bool begin(application::SystemManager* sys_mgr, drivers::OledDriver* oled);
    static void end();
    static void taskFunction(void* parameter);
    static bool isRunning() { return running_; }

private:
    static application::SystemManager* sys_mgr_;
    static drivers::OledDriver* oled_driver_;
    static TaskHandle_t task_handle_;
    static bool running_;
    static void drawMainScreen();
    static void drawClimateScreen();
    static void drawVehicleScreen();
    static void drawStatusScreen();
};

} // namespace rtos
