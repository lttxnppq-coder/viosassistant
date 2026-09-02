#pragma once

#include <cstdint>
#include "MotorDriver.h"
#include "EncoderDriver.h"

namespace services {

class MotorPositionController {
public:
    // Encoder pins are optional - pass 0 if not assigned
    bool begin(drivers::MotorDriver* motor, drivers::EncoderDriver* encoder, uint8_t enc_pin_a = 0, uint8_t enc_pin_b = 0, uint8_t enc_pin_btn = 0);
    void update();
    void setTargetPosition(int32_t position);
    int32_t getCurrentPosition() const;
    int32_t getTargetPosition() const;
    void setKp(float kp);
    void setKi(float ki);
    void setKd(float kd);
    void enable(bool enable);
    bool isEnabled() const;
    bool isAtTarget() const;
    bool isInitialized() const { return initialized_; }

private:
    bool initialized_ = false;
    bool enabled_ = false;
    int32_t target_pos_ = 0;
    drivers::MotorDriver* motor_ = nullptr;
    drivers::EncoderDriver* encoder_ = nullptr;
    float kp_ = 1.0f, ki_ = 0.0f, kd_ = 0.0f;
    float integral_ = 0.0f;
    int32_t prev_error_ = 0;
    uint32_t last_update_ = 0;
};

} // namespace services