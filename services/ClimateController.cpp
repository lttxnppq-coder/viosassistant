#include "ClimateController.h"
#include "SystemConfig.h"
#include <algorithm>

namespace services {

bool ClimateController::begin(const Config& config) {
    config_ = config;
    initialized_ = true;
    return true;
}

void ClimateController::update(const model::TemperatureData& temps, const model::SystemState& state) {
    (void)state;
    // Fan hysteresis decision - single source of truth for temperature -> fan ON/OFF
    // Uses SystemConfig thresholds: ON > 25.5, OFF < 24.5, HOLD 24.5..25.5
    // Input sensor: NTC1 inside_temp_c (user decision); hold if invalid
    if (!temps.inside_valid) {
        return;
    }
    float t = temps.inside_temp_c;
    if (!fan_on_) {
        if (t > SystemConfig::FAN_ON_THRESHOLD_C) {
            fan_on_ = true;
        }
    } else {
        if (t < SystemConfig::FAN_OFF_THRESHOLD_C) {
            fan_on_ = false;
        }
    }
}

void ClimateController::setTemperature(float temp_c) {
    target_temp_ = std::clamp(temp_c, config_.temp_min, config_.temp_max);
}

void ClimateController::setFanSpeed(uint8_t speed) {
    fan_speed_ = std::clamp(speed, config_.fan_min, config_.fan_max);
}

void ClimateController::setAirMode(AirMode mode) {
    air_mode_ = mode;
}

void ClimateController::setRecirculation(RecircMode mode) {
    recirc_mode_ = mode;
}

void ClimateController::setAC(bool enabled) {
    ac_enabled_ = enabled;
}

void ClimateController::setHeater(bool enabled) {
    heater_enabled_ = enabled;
}

float ClimateController::getTemperature() const {
    return target_temp_;
}

uint8_t ClimateController::getFanSpeed() const {
    return fan_speed_;
}

ClimateController::AirMode ClimateController::getAirMode() const {
    return air_mode_;
}

ClimateController::RecircMode ClimateController::getRecirculation() const {
    return recirc_mode_;
}

bool ClimateController::getAC() const {
    return ac_enabled_;
}

bool ClimateController::getHeater() const {
    return heater_enabled_;
}

} // namespace services
