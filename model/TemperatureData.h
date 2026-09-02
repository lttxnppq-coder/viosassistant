#pragma once

#include <cstdint>

namespace model {

struct TemperatureData {
    float inside_temp_c = 0.0f;
    float outside_temp_c = 0.0f;
    float evaporator_temp_c = 0.0f;
    float ambient_temp_c = 0.0f;
    float setpoint_temp_c = 22.0f;
    uint32_t last_update_ms = 0;
    bool inside_valid = false;
    bool outside_valid = false;
    bool evaporator_valid = false;
    bool ambient_valid = false;
};

struct TemperatureRaw {
    uint16_t inside_adc = 0;
    uint16_t outside_adc = 0;
    uint16_t evaporator_adc = 0;
    uint16_t ambient_adc = 0;
};

} // namespace model