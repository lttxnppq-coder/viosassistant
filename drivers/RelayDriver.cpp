#include "RelayDriver.h"
#include <Arduino.h>

namespace drivers {

bool RelayDriver::begin(uint8_t pin, bool active_high) {
    pin_ = pin;
    active_high_ = active_high;
    pinMode(pin_, OUTPUT);
    off();
    initialized_ = true;
    return true;
}

void RelayDriver::end() {
    pinMode(pin_, INPUT);
    initialized_ = false;
}

void RelayDriver::set(bool state) {
    state_ = state;
    if (initialized_) {
        digitalWrite(pin_, (state_ == active_high_) ? HIGH : LOW);
    }
}

void RelayDriver::on() {
    set(true);
}

void RelayDriver::off() {
    set(false);
}

void RelayDriver::toggle() {
    set(!state_);
}

bool RelayDriver::getState() const {
    return state_;
}

} // namespace drivers
