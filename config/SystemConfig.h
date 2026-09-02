#pragma once

#include <cstdint>

namespace SystemConfig {

constexpr uint32_t SERIAL_BAUD_RATE = 115200;
constexpr uint32_t MAIN_LOOP_INTERVAL_MS = 10;
constexpr uint32_t OLED_UPDATE_INTERVAL_MS = 100;
constexpr uint32_t CLIMATE_UPDATE_INTERVAL_MS = 500;
constexpr uint32_t VEHICLE_DATA_UPDATE_INTERVAL_MS = 200;
constexpr uint32_t WATCHDOG_TIMEOUT_MS = 5000;

constexpr float TEMP_MIN_C = -40.0f;
constexpr float TEMP_MAX_C = 125.0f;
constexpr float TEMP_DEFAULT_C = 22.0f;

constexpr uint8_t FAN_SPEED_MIN = 0;
constexpr uint8_t FAN_SPEED_MAX = 255;
constexpr uint8_t FAN_SPEED_DEFAULT = 128;

// Fan hysteresis thresholds (inside NTC1) - user decision 2026-08
// ON  when T > 25.5, OFF when T < 24.5, HOLD 24.5..25.5
constexpr float FAN_ON_THRESHOLD_C  = 25.5f;
constexpr float FAN_OFF_THRESHOLD_C = 24.5f;

constexpr uint8_t DAMPER_POS_MIN = 0;
constexpr uint8_t DAMPER_POS_MAX = 100;
constexpr uint8_t DAMPER_POS_DEFAULT = 50;

constexpr uint16_t MOTOR_POS_MIN = 0;
constexpr uint16_t MOTOR_POS_MAX = 65535;
constexpr uint16_t MOTOR_POS_DEFAULT = 0;

constexpr uint8_t MAX_RETRIES = 3;
constexpr uint32_t RETRY_DELAY_MS = 100;

} // namespace SystemConfig