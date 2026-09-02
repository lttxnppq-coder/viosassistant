#include "EncoderDriver.h"
#include <Arduino.h>

namespace drivers {

EncoderDriver* EncoderDriver::instance_ = nullptr;

bool EncoderDriver::begin(uint8_t pin_a, uint8_t pin_b, uint8_t pin_btn) {
    pin_a_ = pin_a;
    pin_b_ = pin_b;
    pin_btn_ = pin_btn;

    pinMode(pin_a_, INPUT_PULLUP);
    pinMode(pin_b_, INPUT_PULLUP);
    pinMode(pin_btn_, INPUT_PULLUP);

    instance_ = this;
    attachInterrupt(digitalPinToInterrupt(pin_a_), isrA, CHANGE);
    attachInterrupt(digitalPinToInterrupt(pin_b_), isrB, CHANGE);

    initialized_ = true;
    return true;
}

void EncoderDriver::end() {
    detachInterrupt(digitalPinToInterrupt(pin_a_));
    detachInterrupt(digitalPinToInterrupt(pin_b_));
    instance_ = nullptr;
    initialized_ = false;
}

int32_t EncoderDriver::getPosition() const {
    return position_;
}

void EncoderDriver::setPosition(int32_t pos) {
    position_ = pos;
    last_position_ = pos;
}

int32_t EncoderDriver::getDelta() {
    int32_t delta = position_ - last_position_;
    last_position_ = position_;
    return delta;
}

bool EncoderDriver::getButton() const {
    return btn_state_;
}

bool EncoderDriver::isButtonPressed() const {
    return btn_state_ && !btn_last_;
}

void EncoderDriver::reset() {
    position_ = 0;
    last_position_ = 0;
}

void IRAM_ATTR EncoderDriver::isrA() {
    if (instance_) {
        bool a = digitalRead(instance_->pin_a_);
        bool b = digitalRead(instance_->pin_b_);
        instance_->position_ += (a == b) ? 1 : -1;
    }
}

void IRAM_ATTR EncoderDriver::isrB() {
    if (instance_) {
        bool a = digitalRead(instance_->pin_a_);
        bool b = digitalRead(instance_->pin_b_);
        instance_->position_ += (a != b) ? 1 : -1;
    }
}

} // namespace drivers
