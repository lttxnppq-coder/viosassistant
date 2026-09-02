#include "PwmDriver.h"
#include <Arduino.h>

namespace drivers {

bool PwmDriver::begin(const ChannelConfig& config) {
    config_ = config;
    // ESP32 Arduino core 3.x: ledcAttach(pin, freq, resolution)
    ledcAttach(config_.pin, config_.freq, config_.resolution);
    ledcWrite(config_.pin, 0);
    initialized_ = true;
    return true;
}

void PwmDriver::end() {
    ledcDetach(config_.pin);
    initialized_ = false;
}

void PwmDriver::setDuty(uint8_t duty) {
    if (initialized_) {
        ledcWrite(config_.pin, duty);
    }
}

void PwmDriver::setFrequency(uint32_t freq) {
    if (initialized_) {
        ledcChangeFrequency(config_.pin, freq, config_.resolution);
        config_.freq = freq;
    }
}

uint8_t PwmDriver::getDuty() const {
    if (initialized_) {
        return ledcRead(config_.pin);
    }
    return 0;
}

uint32_t PwmDriver::getFrequency() const {
    return config_.freq;
}

} // namespace drivers