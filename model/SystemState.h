#pragma once

#include <cstdint>

namespace model {

enum class SystemMode : uint8_t {
    OFF = 0,
    INITIALIZING = 1,
    NORMAL = 2,
    CLIMATE_AUTO = 3,
    CLIMATE_MANUAL = 4,
    DEFROST = 5,
    ERROR = 0xFF
};

enum class ErrorCode : uint8_t {
    NONE = 0,
    SENSOR_FAILURE = 1,
    COMMUNICATION_ERROR = 2,
    ACTUATOR_FAILURE = 3,
    OVER_TEMPERATURE = 4,
    UNDER_VOLTAGE = 5,
    OVER_CURRENT = 6,
    INVALID_CONFIG = 7,
    WATCHDOG_TIMEOUT = 8
};

struct SystemState {
    SystemMode mode = SystemMode::INITIALIZING;
    ErrorCode error = ErrorCode::NONE;
    uint32_t uptime_ms = 0;
    uint32_t free_heap = 0;
    uint16_t cpu_usage = 0;
    bool watchdog_ok = true;
    uint8_t retry_count = 0;
};

} // namespace model