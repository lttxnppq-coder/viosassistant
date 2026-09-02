#pragma once

#include <cstdint>
#include "PwmDriver.h"
#include "PinConfig.h"

namespace services {

// Fan FET polarity note: current implementation is active-HIGH (duty 0=OFF, 255=MAX).
// Polarity TBD - hardware verification required (see PinConfig.h). If FET is active-LOW,
// invert duty in a single place (e.g., PwmDriver or here) after measurement.
class FanController {
public:
    bool begin(drivers::PwmDriver* pwm_driver = nullptr, uint8_t pwm_pin = PIN_FAN_FET_PWM);
    void update();
    void setSpeed(uint8_t speed);
    uint8_t getSpeed() const;
    void setRampRate(uint8_t rate);
    void enable(bool enable);
    bool isEnabled() const;
    bool isInitialized() const { return initialized_; }

private:
    bool initialized_ = false;
    bool enabled_ = true;
    uint8_t current_speed_ = 0;
    uint8_t target_speed_ = 0;
    uint8_t ramp_rate_ = 10;
    drivers::PwmDriver* pwm_driver_ = nullptr;
    uint32_t last_update_ = 0;
};

} // namespace services
