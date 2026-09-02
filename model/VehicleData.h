#pragma once

#include <cstdint>

namespace model {

struct VehicleData {
    float vehicle_speed_kmh = 0.0f;
    float engine_rpm = 0.0f;
    float coolant_temp_c = 0.0f;
    float battery_voltage_v = 0.0f;
    bool ac_compressor_active = false;
    bool blower_active = false;
    uint8_t gear_position = 0;
    uint32_t last_update_ms = 0;
    bool data_valid = false;
};

struct VehicleStatus {
    bool ignition_on = false;
    bool engine_running = false;
    bool ac_request = false;
    bool defrost_request = false;
    bool recirc_request = false;
    uint8_t fan_speed_cmd = 0;
    uint8_t temp_setpoint_cmd = 22;
    uint8_t air_mode_cmd = 0;
};

} // namespace model