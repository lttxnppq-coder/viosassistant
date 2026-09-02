#include "CommandManager.h"
#include <Arduino.h>

namespace application {

bool CommandManager::begin() {
    initialized_ = true;
    return true;
}

void CommandManager::update() {
    // Process queued commands
}

bool CommandManager::queueCommand(const model::Command& cmd) {
    // Reject NONE as invalid command (do not queue)
    if (cmd.type == model::CommandType::NONE) return false;
    if (queue_count_ >= 16) return false;
    command_queue_[queue_tail_] = cmd;
    queue_tail_ = (queue_tail_ + 1) % 16;
    queue_count_++;
    return true;
}

bool CommandManager::processCommand(const model::Command& cmd, model::CommandResponse& response) {
    last_cmd_ = cmd.type;
    last_cmd_time_ = millis();
    response.cmd_type = cmd.type;
    response.success = false;
    response.error_code = 1; // default INVALID
    response.response_len = 0;

    // Validate CommandType against known enum range
    // Implemented set: temperature, fan, air_mode, recirc, ac, heater, system_reset
    // Valid but NOT_IMPLEMENTED: damper_pos, request_status, request_vehicle_data, factory_reset
    // ASSUMPTION: This classification is derived from SystemManager::handleCommand()
    // dispatch and is not an authoritative protocol specification.
    switch (cmd.type) {
        case model::CommandType::SET_TEMPERATURE:
        case model::CommandType::SET_FAN_SPEED:
        case model::CommandType::SET_AIR_MODE:
        case model::CommandType::SET_RECIRCULATION:
        case model::CommandType::SET_AC:
        case model::CommandType::SET_HEATER:
        case model::CommandType::SYSTEM_RESET:
            response.success = true;
            response.error_code = 0;
            return true;
        case model::CommandType::SET_DAMPER_POS:
        case model::CommandType::REQUEST_STATUS:
        case model::CommandType::REQUEST_VEHICLE_DATA:
        case model::CommandType::FACTORY_RESET:
            response.success = false;
            response.error_code = 2; // NOT_IMPLEMENTED
            return false;
        case model::CommandType::NONE:
        default:
            response.success = false;
            response.error_code = 1; // INVALID
            return false;
    }
}

model::CommandType CommandManager::getLastCommand() const {
    return last_cmd_;
}

uint32_t CommandManager::getLastCommandTime() const {
    return last_cmd_time_;
}

void CommandManager::clearQueue() {
    queue_head_ = 0;
    queue_tail_ = 0;
    queue_count_ = 0;
}

} // namespace application
