#include "FanController.h"
#include <Arduino.h>

namespace services {

bool FanController::begin(drivers::PwmDriver* pwm_driver, uint8_t pwm_pin) {
    pwm_driver_ = pwm_driver;
    if (pwm_driver_ && !pwm_driver_->isInitialized()) {
        drivers::PwmDriver::ChannelConfig config;
        config.pin = pwm_pin;
        config.freq = 1000;
        config.resolution = 8;
        pwm_driver_->begin(config);
    }
    initialized_ = true;
    return true;
}

void FanController::update() {
    if (!initialized_ || !enabled_) return;
    if (current_speed_ < target_speed_) {
        current_speed_ = std::min<uint8_t>(current_speed_ + ramp_rate_, target_speed_);
    } else if (current_speed_ > target_speed_) {
        current_speed_ = (current_speed_ > ramp_rate_) ? current_speed_ - ramp_rate_ : 0;
    }
    if (pwm_driver_) {
        pwm_driver_->setDuty(map(current_speed_, 0, 255, 0, 255));
    }
}

void FanController::setSpeed(uint8_t speed) {
    target_speed_ = speed;
}

uint8_t FanController::getSpeed() const {
    return current_speed_;
}

void FanController::setRampRate(uint8_t rate) {
    ramp_rate_ = rate;
}

void FanController::enable(bool enable) {
    enabled_ = enable;
    if (!enable) {
        target_speed_ = 0;
    }
}

bool FanController::isEnabled() const {
    return enabled_;
}

} // namespace services