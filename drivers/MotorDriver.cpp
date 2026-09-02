#include "MotorDriver.h"
#include <Arduino.h>

namespace drivers {

bool MotorDriver::begin(const Config& config) {
    config_ = config;
    max_duty_ = (1 << config_.pwm_resolution) - 1;

    // Initialize IN1 PWM channel using ESP32 Arduino core 3.x API
    ledcAttach(config_.in1_pin, config_.pwm_freq, config_.pwm_resolution);
    // Note: In ESP32 Arduino core 3.x, ledcAttach assigns a channel internally
    // We'll use the pin directly for ledcWrite

    // Initialize IN2 PWM channel
    ledcAttach(config_.in2_pin, config_.pwm_freq, config_.pwm_resolution);

    // Start with motor stopped (coast)
    ledcWrite(config_.in1_pin, 0);
    ledcWrite(config_.in2_pin, 0);

    initialized_ = true;
    return true;
}

void MotorDriver::end() {
    coast();
    ledcDetach(config_.in1_pin);
    ledcDetach(config_.in2_pin);
    initialized_ = false;
}

void MotorDriver::setSpeed(int16_t speed) {
    if (!initialized_) return;

    current_speed_ = constrain(speed, -1000, 1000);

    if (current_speed_ > 0) {
        // Forward: IN1 = PWM, IN2 = LOW
        uint16_t duty = map(abs(current_speed_), 0, 1000, 0, max_duty_);
        ledcWrite(config_.in1_pin, duty);
        ledcWrite(config_.in2_pin, 0);
    } else if (current_speed_ < 0) {
        // Reverse: IN1 = LOW, IN2 = PWM
        uint16_t duty = map(abs(current_speed_), 0, 1000, 0, max_duty_);
        ledcWrite(config_.in1_pin, 0);
        ledcWrite(config_.in2_pin, duty);
    } else {
        // Coast/stop: both LOW
        ledcWrite(config_.in1_pin, 0);
        ledcWrite(config_.in2_pin, 0);
    }
}

void MotorDriver::brake() {
    if (!initialized_) return;
    ledcWrite(config_.in1_pin, max_duty_);
    ledcWrite(config_.in2_pin, max_duty_);
    current_speed_ = 0;
}

void MotorDriver::coast() {
    if (!initialized_) return;
    ledcWrite(config_.in1_pin, 0);
    ledcWrite(config_.in2_pin, 0);
    current_speed_ = 0;
}

int16_t MotorDriver::getSpeed() const {
    return current_speed_;
}

} // namespace drivers