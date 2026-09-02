#pragma once

#include <cstdint>
#include "Command.h"
#include "SystemState.h"
#include "TemperatureData.h"
#include "VehicleData.h"
#include "UartDriver.h"

namespace application {

class ResponseManager {
public:
    bool begin(drivers::UartDriver* uart = nullptr);
    void sendResponse(const model::CommandResponse& response);
    void sendStatus(const model::SystemState& state);
    void sendTemperatureData(const model::TemperatureData& temps);
    void sendVehicleData(const model::VehicleData& vehicle);
    void sendAck(uint8_t cmd_id, bool success);
    void sendError(uint8_t cmd_id, uint8_t error_code);
    bool isInitialized() const { return initialized_; }

private:
    bool initialized_ = false;
    drivers::UartDriver* uart_driver_ = nullptr;
    uint8_t tx_buffer_[256];
    uint16_t crc16(const uint8_t* data, uint16_t len);
};

} // namespace application
