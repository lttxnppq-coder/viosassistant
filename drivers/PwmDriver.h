#pragma once

#include <cstdint>
#include "PinConfig.h"

namespace drivers {

// PwmDriver - ESP32 LEDC wrapper. Currently active-HIGH: duty 0=low, max=high.
// Fan FET GPIO7 polarity TBD (see PinConfig.h). If active-LOW verified, invert here.
class PwmDriver {
public:
    struct ChannelConfig {
        uint8_t pin;
        uint32_t freq = 1000;
        uint8_t resolution = 8;
    };

    bool begin(const ChannelConfig& config);
    void end();
    void setDuty(uint8_t duty);
    void setFrequency(uint32_t freq);
    uint8_t getDuty() const;
    uint32_t getFrequency() const;
    bool isInitialized() const { return initialized_; }

private:
    bool initialized_ = false;
    ChannelConfig config_;
};

} // namespace drivers