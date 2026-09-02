#pragma once

#include <cstdint>
#include "TemperatureData.h"
#include "SystemState.h"

namespace services {

class ClimateController {
public:
    enum class AirMode : uint8_t {
        VENT = 0,
        BI_LEVEL = 1,
        FLOOR = 2,
        MIX = 3,
        DEFROST = 4,
        FLOOR_DEFROST = 5
    };

    enum class RecircMode : uint8_t {
        FRESH = 0,
        RECIRC = 1,
        AUTO = 2
    };

    struct Config {
        float temp_min;
        float temp_max;
        uint8_t fan_min;
        uint8_t fan_max;
        Config() : temp_min(16.0f), temp_max(30.0f), fan_min(0), fan_max(255) {}
        Config(float tmin, float tmax, uint8_t fmin, uint8_t fmax)
            : temp_min(tmin), temp_max(tmax), fan_min(fmin), fan_max(fmax) {}
    };

    bool begin(const Config& config = Config{});
    void update(const model::TemperatureData& temps, const model::SystemState& state);
    void setTemperature(float temp_c);
    void setFanSpeed(uint8_t speed);
    void setAirMode(AirMode mode);
    void setRecirculation(RecircMode mode);
    void setAC(bool enabled);
    void setHeater(bool enabled);
    float getTemperature() const;
    uint8_t getFanSpeed() const;
    AirMode getAirMode() const;
    RecircMode getRecirculation() const;
    bool getAC() const;
    bool getHeater() const;
    bool isInitialized() const { return initialized_; }
    // Fan hysteresis decision (temperature -> fan ON/OFF)
    // Fan ON when T > FAN_ON_THRESHOLD_C (25.5), OFF when T < FAN_OFF_THRESHOLD_C (24.5), HOLD in between
    // Source: SystemConfig::FAN_ON/OFF_THRESHOLD_C, sensor: NTC1 inside_temp_c
    bool getFanOn() const { return fan_on_; }

private:
    bool initialized_ = false;
    Config config_;
    float target_temp_ = 22.0f;
    uint8_t fan_speed_ = 128;
    AirMode air_mode_ = AirMode::VENT;
    RecircMode recirc_mode_ = RecircMode::FRESH;
    bool ac_enabled_ = false;
    bool heater_enabled_ = false;
    // Fan hysteresis state: tracks last decision, updated in update()
    bool fan_on_ = false;
};

} // namespace services
