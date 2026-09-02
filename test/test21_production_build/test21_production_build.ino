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

#include "SystemManager.h"
#include "ResponseManager.h"
#include "CommunicationTask.h"
#include "ControlTask.h"
#include "OledTask.h"
#include "Logger.h"

// TEST 21 - FULL PRODUCTION BUILD (BUILD 02 pipeline reuse)
// Reuse: wrapper gwrap2 (compiler.path=gw) + temporary merged sketch + all 22 production .cpp + include paths
// Include paths: root, config, model, drivers, services, application, rtos, utils
// FQBN: ESP32:esp32:esp32s3:CDCOnBoot=default,FlashSize=16M,PartitionScheme=default_8MB
// HARDWARE: NOT EXECUTED

application::SystemManager system_manager;
application::ResponseManager response_manager;

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000) {}
    utils::Logger::begin(utils::LogLevel::INFO);
    LOG_INFO("MAIN", "ViosAssistant T21 full production build starting...");
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
    LOG_INFO("MAIN", "ViosAssistant T21 full production build OK");
}
void loop() {
    delay(1000);
    system_manager.triggerWatchdog();
    const auto& state = system_manager.getSystemState();
    LOG_DEBUG("MAIN", "Mode:%d Heap:%lu Err:%d", (int)state.mode, state.free_heap, (int)state.error);
}
