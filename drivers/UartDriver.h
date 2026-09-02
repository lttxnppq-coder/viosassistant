#pragma once

#include <cstdint>
#include <cstddef>
#include "PinConfig.h"

namespace drivers {

class UartDriver {
public:
    // Raspberry Pi UART pins (GPIO17/18)
    bool begin(uint32_t baud = 115200, uint8_t tx_pin = PIN_PI_UART_TX, uint8_t rx_pin = PIN_PI_UART_RX);
    void end();
    bool write(const uint8_t* data, size_t len);
    int read(uint8_t* buffer, size_t max_len);
    int available() const;
    void flush();
    void setBaudRate(uint32_t baud);
    bool isInitialized() const { return initialized_; }

private:
    bool initialized_ = false;
    uint8_t tx_pin_ = PIN_PI_UART_TX;
    uint8_t rx_pin_ = PIN_PI_UART_RX;
};

} // namespace drivers
