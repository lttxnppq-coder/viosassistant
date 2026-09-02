#pragma once

#include <cstdint>
#include "PinConfig.h"

namespace drivers {

class MotorDriver {
public:
    struct Config {
        uint8_t in1_pin = PIN_MOTOR_IN1;
        uint8_t in2_pin = PIN_MOTOR_IN2;
        uint32_t pwm_freq = 20000;
        uint8_t pwm_resolution = 10;
        uint8_t pwm_channel_in1 = 0;
        uint8_t pwm_channel_in2 = 1;
        Config() : in1_pin(PIN_MOTOR_IN1), in2_pin(PIN_MOTOR_IN2),
                   pwm_freq(20000), pwm_resolution(10), pwm_channel_in1(0), pwm_channel_in2(1) {}
        Config(uint8_t in1, uint8_t in2, uint32_t freq, uint8_t res, uint8_t ch1, uint8_t ch2)
            : in1_pin(in1), in2_pin(in2), pwm_freq(freq), pwm_resolution(res), pwm_channel_in1(ch1), pwm_channel_in2(ch2) {}
    };

    // DRV8833 Parallel Mode: IN1 = AIN1+BIN1, IN2 = AIN2+BIN2
    // No ENA pin in parallel mode
    // nSLEEP = hardware-controlled / pending electrical verification

    bool begin(const Config& config = Config{});
    void end();

    // Signed speed control: -1000 .. +1000
    // >0: IN1 = PWM(abs), IN2 = LOW (forward)
    // <0: IN1 = LOW, IN2 = PWM(abs) (reverse)
    // =0: IN1 = LOW, IN2 = LOW (coast/stop)
    void setSpeed(int16_t speed);

    // Brake: IN1 = HIGH, IN2 = HIGH
    void brake();

    // Coast/stop: IN1 = LOW, IN2 = LOW
    void coast();

    // Alias for coast() - compatibility
    void stop() { coast(); }

    int16_t getSpeed() const;
    bool isInitialized() const { return initialized_; }

private:
    bool initialized_ = false;
    Config config_;
    int16_t current_speed_ = 0;
    uint16_t max_duty_;
};

} // namespace drivers
