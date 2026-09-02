#include "AirModeController.h"
#include <Arduino.h>

namespace services {

bool AirModeController::begin(drivers::MotorDriver* motor_driver) {
    motor_driver_ = motor_driver;
    if (motor_driver_ && !motor_driver_->isInitialized()) {
        motor_driver_->begin();
    }
    initialized_ = true;
    return true;
}

void AirModeController::update(ClimateController::AirMode mode) {
    if (!initialized_) return;
    static const uint16_t mode_positions[] = {0, 15000, 30000, 45000, 60000, 65535};
    if ((uint8_t)mode < 6) {
        target_pos_ = mode_positions[(uint8_t)mode];
    }
    if (current_pos_ < target_pos_) {
        current_pos_ += 100;
        if (motor_driver_) motor_driver_->setSpeed(50);
    } else if (current_pos_ > target_pos_) {
        current_pos_ = (current_pos_ > 100) ? current_pos_ - 100 : 0;
        if (motor_driver_) motor_driver_->setSpeed(-50);
    } else {
        if (motor_driver_) motor_driver_->stop();
    }
}

void AirModeController::setPosition(uint16_t position) {
    target_pos_ = position;
}

uint16_t AirModeController::getPosition() const {
    return current_pos_;
}

void AirModeController::calibrate() {
    calibrated_ = true;
    current_pos_ = 0;
    target_pos_ = 0;
}

bool AirModeController::isCalibrated() const {
    return calibrated_;
}

} // namespace services
