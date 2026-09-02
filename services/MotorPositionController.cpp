#include "MotorPositionController.h"
#include <Arduino.h>

namespace services {

bool MotorPositionController::begin(drivers::MotorDriver* motor, drivers::EncoderDriver* encoder, uint8_t enc_pin_a, uint8_t enc_pin_b, uint8_t enc_pin_btn) {
    motor_ = motor;
    encoder_ = encoder;
    if (motor_ && !motor_->isInitialized()) motor_->begin();
    if (encoder_ && !encoder_->isInitialized() && enc_pin_a && enc_pin_b) {
        encoder_->begin(enc_pin_a, enc_pin_b, enc_pin_btn);
    }
    initialized_ = true;
    return true;
}

void MotorPositionController::update() {
    if (!initialized_ || !enabled_) return;
    if (!encoder_ || !motor_) return;

    int32_t current = encoder_->getPosition();
    int32_t error = target_pos_ - current;

    if (abs(error) < 10) {
        motor_->stop();
        integral_ = 0;
        prev_error_ = 0;
        return;
    }

    float dt = (millis() - last_update_) / 1000.0f;
    if (dt <= 0) dt = 0.01f;

    integral_ += error * dt;
    float derivative = (error - prev_error_) / dt;

    float output = kp_ * error + ki_ * integral_ + kd_ * derivative;
    output = constrain(output, -100, 100);

    motor_->setSpeed((int8_t)output);

    prev_error_ = error;
    last_update_ = millis();
}

void MotorPositionController::setTargetPosition(int32_t position) {
    target_pos_ = position;
}

int32_t MotorPositionController::getCurrentPosition() const {
    return encoder_ ? encoder_->getPosition() : 0;
}

int32_t MotorPositionController::getTargetPosition() const {
    return target_pos_;
}

void MotorPositionController::setKp(float kp) { kp_ = kp; }
void MotorPositionController::setKi(float ki) { ki_ = ki; }
void MotorPositionController::setKd(float kd) { kd_ = kd; }

void MotorPositionController::enable(bool enable) {
    enabled_ = enable;
    if (!enable) {
        if (motor_) motor_->stop();
        integral_ = 0;
        prev_error_ = 0;
    }
}

bool MotorPositionController::isEnabled() const {
    return enabled_;
}

bool MotorPositionController::isAtTarget() const {
    if (!encoder_) return true;
    return abs(target_pos_ - encoder_->getPosition()) < 10;
}

} // namespace services