#pragma once

#include <cstdint>
#include <Arduino.h>
// PinConfig.h now defines PIN_ENC_A=19, PIN_ENC_B=20 (GA25 quadrature, user decision 2026-08)
// #include "PinConfig.h"  // included via SystemManager; encoder pins provided via begin()

namespace drivers {

class EncoderDriver {
public:
    // Encoder GA25 quadrature: pins provided explicitly via begin(pin_a, pin_b, pin_btn)
    // PinConfig.h is single source of truth (PIN_ENC_A=19, PIN_ENC_B=20); driver does not hardcode
    bool begin(uint8_t pin_a, uint8_t pin_b, uint8_t pin_btn);
    void end();
    int32_t getPosition() const;
    void setPosition(int32_t pos);
    int32_t getDelta();
    bool getButton() const;
    bool isButtonPressed() const;
    void reset();
    bool isInitialized() const { return initialized_; }

private:
    bool initialized_ = false;
    uint8_t pin_a_ = 0;
    uint8_t pin_b_ = 0;
    uint8_t pin_btn_ = 0;
    volatile int32_t position_ = 0;
    volatile int32_t last_position_ = 0;
    volatile bool btn_state_ = false;
    volatile bool btn_last_ = false;
    static void IRAM_ATTR isrA();
    static void IRAM_ATTR isrB();
    static EncoderDriver* instance_;
};

} // namespace drivers