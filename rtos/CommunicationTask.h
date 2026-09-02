#pragma once

#include <cstdint>
#include "SystemManager.h"
#include "CanDriver.h"
#include "UartDriver.h"

namespace rtos {

class CommunicationTask {
public:
    static bool begin(application::SystemManager* sys_mgr, drivers::CanDriver* can, drivers::UartDriver* uart);
    static void end();
    static void taskFunction(void* parameter);
    static bool isRunning() { return running_; }

private:
    static application::SystemManager* sys_mgr_;
    static drivers::CanDriver* can_driver_;
    static drivers::UartDriver* uart_driver_;
    static TaskHandle_t task_handle_;
    static bool running_;
    static void processCanMessages();
    static void processUartMessages();
    static void sendPeriodicUpdates();
};

} // namespace rtos
