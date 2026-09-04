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
    // Level 1-5 interface for Jetson (PROPOSED, REQUIRES CONFIRMATION)
    // Maps Level -> PWM: 1:51,2:102,3:153,4:204,5:255 (linear, TBD)
    void setLevel(uint8_t level);
    uint8_t getLevel() const;
    void fanOff(); // save + OFF (Jetson 7)
    void fanOn();  // restore (Jetson 6)
    uint8_t getLastLevel() const { return last_level_; }

private:
    bool initialized_ = false;
    bool enabled_ = true;
    uint8_t current_speed_ = 0;
    uint8_t target_speed_ = 0;
    uint8_t ramp_rate_ = 10;
    drivers::PwmDriver* pwm_driver_ = nullptr;
    uint32_t last_update_ = 0;
    // For Jetson FAN ON/OFF save/restore (H02 §5, H03 §6)
    uint8_t last_level_ = 128; // saved level for 6/7
    bool is_on_ = true;
};

} // namespace services
