#include "SystemManager.h"
#include "ResponseManager.h"
#include "CommunicationTask.h"
#include "ControlTask.h"
#include "OledTask.h"
#include "Logger.h"

application::SystemManager system_manager;
application::ResponseManager response_manager;

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000) {}

    utils::Logger::begin(utils::LogLevel::INFO);
    LOG_INFO("MAIN", "ViosAssistant starting...");

    if (!system_manager.begin()) {
        LOG_ERROR("MAIN", "SystemManager init failed!");
        while (1) { delay(1000); }
    }

    if (!response_manager.begin()) {
        LOG_ERROR("MAIN", "ResponseManager init failed!");
    }

    if (!rtos::CommunicationTask::begin(&system_manager, nullptr, nullptr)) {
        LOG_WARN("MAIN", "CommunicationTask init failed!");
    }

    if (!rtos::ControlTask::begin(&system_manager)) {
        LOG_WARN("MAIN", "ControlTask init failed!");
    }

    if (!rtos::OledTask::begin(&system_manager, &system_manager.getOledDriver())) {
        LOG_WARN("MAIN", "OledTask init failed!");
    }

    LOG_INFO("MAIN", "ViosAssistant started successfully");
}

void loop() {
    delay(1000);
    system_manager.triggerWatchdog();

    const auto& state = system_manager.getSystemState();
    LOG_DEBUG("MAIN", "Mode:%d Heap:%lu Err:%d", (int)state.mode, state.free_heap, (int)state.error);
}