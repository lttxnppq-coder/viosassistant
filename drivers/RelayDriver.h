#pragma once

#include <cstdint>
#include "PinConfig.h"

namespace drivers {

class RelayDriver {
public:
    bool begin(uint8_t pin, bool active_high = true);
    void end();
    void set(bool state);
    void on();
    void off();
    void toggle();
    bool getState() const;
    bool isInitialized() const { return initialized_; }

private:
    bool initialized_ = false;
    uint8_t pin_ = 0;
    bool active_high_ = true;
    bool state_ = false;
};

} // namespace drivers
