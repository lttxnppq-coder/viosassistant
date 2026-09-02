#pragma once

#include <cstdint>
#include "MotorDriver.h"
#include "ClimateController.h"

namespace services {

class AirModeController {
public:
    bool begin(drivers::MotorDriver* motor_driver = nullptr);
    void update(ClimateController::AirMode mode);
    void setPosition(uint16_t position);
    uint16_t getPosition() const;
    void calibrate();
    bool isCalibrated() const;
    bool isInitialized() const { return initialized_; }

private:
    bool initialized_ = false;
    bool calibrated_ = false;
    uint16_t current_pos_ = 0;
    uint16_t target_pos_ = 0;
    drivers::MotorDriver* motor_driver_ = nullptr;
    uint32_t last_update_ = 0;
};

} // namespace services
