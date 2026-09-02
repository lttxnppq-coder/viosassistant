#pragma once

#include <cstdint>

namespace model {

enum class CommandType : uint8_t {
    NONE = 0,
    SET_TEMPERATURE = 0x01,
    SET_FAN_SPEED = 0x02,
    SET_AIR_MODE = 0x03,
    SET_RECIRCULATION = 0x04,
    SET_AC = 0x05,
    SET_HEATER = 0x06,
    SET_DAMPER_POS = 0x07,
    REQUEST_STATUS = 0x10,
    REQUEST_VEHICLE_DATA = 0x11,
    SYSTEM_RESET = 0xF0,
    FACTORY_RESET = 0xF1
};

struct Command {
    CommandType type = CommandType::NONE;
    uint8_t payload[8] = {0};
    uint8_t payload_len = 0;
    uint32_t timestamp = 0;
    bool requires_ack = false;
};

struct CommandResponse {
    CommandType cmd_type = CommandType::NONE;
    bool success = false;
    uint8_t error_code = 0;
    uint8_t response_data[8] = {0};
    uint8_t response_len = 0;
};

} // namespace model