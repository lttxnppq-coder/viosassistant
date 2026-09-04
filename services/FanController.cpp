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

void FanController::setLevel(uint8_t level) {
    // PROPOSED mapping Level 1-5 -> PWM, REQUIRES CONFIRMATION
    // 0 -> 0, 1->51,2->102,3->153,4->204,5->255
    if (level == 0) {
        setSpeed(0);
        is_on_ = false;
        return;
    }
    if (level > 5) level = 5;
    uint8_t pwm = level * 51; // 51,102,153,204,255
    if (level == 5) pwm = 255;
    last_level_ = pwm;
    is_on_ = true;
    setSpeed(pwm);
}

uint8_t FanController::getLevel() const {
    // Inverse map PWM -> Level 1-5 (approx)
    if (current_speed_ == 0) return 0;
    uint8_t lvl = (current_speed_ + 25) / 51;
    if (lvl < 1) lvl = 1;
    if (lvl > 5) lvl = 5;
    return lvl;
}

void FanController::fanOff() {
    if (current_speed_ > 0 || target_speed_ > 0) {
        last_level_ = (target_speed_ > 0) ? target_speed_ : current_speed_;
        if (last_level_ == 0) last_level_ = 128;
    }
    is_on_ = false;
    setSpeed(0);
}

void FanController::fanOn() {
    is_on_ = true;
    if (last_level_ == 0) last_level_ = 128;
    setSpeed(last_level_);
}

} // namespace services