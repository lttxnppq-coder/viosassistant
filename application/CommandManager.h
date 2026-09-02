#pragma once

#include <cstdint>
#include "Command.h"
#include "SystemState.h"

namespace application {

// CommandManager - queue + dispatch for Pi UART commands
// STATUS: queueCommand implemented; processCommand validated but execution is
// delegated to SystemManager::handleCommand for actual actuation.
// Error codes for response.error_code:
//   0 = SUCCESS (VALID + IMPLEMENTED)
//   1 = INVALID (NONE or unknown CommandType)
//   2 = NOT_IMPLEMENTED (VALID but no handler yet)
//   3 = QUEUE_FULL (queueCommand failed)
//   4 = EXECUTION_ERROR (reserved)
// processCommand does NOT fake SUCCESS for NOT_IMPLEMENTED/INVALID.
// NOTE: Command support classification is derived from the currently implemented
// SystemManager::handleCommand() dispatch. This is an implementation assumption,
// not an authoritative protocol specification.
class CommandManager {
public:
    bool begin();
    void update();
    bool queueCommand(const model::Command& cmd);
    bool processCommand(const model::Command& cmd, model::CommandResponse& response);
    model::CommandType getLastCommand() const;
    uint32_t getLastCommandTime() const;
    void clearQueue();
    bool isInitialized() const { return initialized_; }

private:
    bool initialized_ = false;
    model::Command command_queue_[16];
    uint8_t queue_head_ = 0;
    uint8_t queue_tail_ = 0;
    uint8_t queue_count_ = 0;
    model::CommandType last_cmd_ = model::CommandType::NONE;
    uint32_t last_cmd_time_ = 0;
};

} // namespace application
