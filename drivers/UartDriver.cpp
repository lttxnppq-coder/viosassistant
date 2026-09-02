#include "UartDriver.h"
#include <Arduino.h>

namespace drivers {

bool UartDriver::begin(uint32_t baud, uint8_t tx_pin, uint8_t rx_pin) {
    tx_pin_ = tx_pin;
    rx_pin_ = rx_pin;
    // Use Serial1 (UART1) for Pi UART, keeping Serial (UART0/CH343P 43-44) for debug/USB
    Serial1.begin(baud, SERIAL_8N1, rx_pin_, tx_pin_);
    initialized_ = true;
    return true;
}

void UartDriver::end() {
    Serial1.end();
    initialized_ = false;
}

bool UartDriver::write(const uint8_t* data, size_t len) {
    if (!initialized_) return false;
    return Serial1.write(data, len) == len;
}

int UartDriver::read(uint8_t* buffer, size_t max_len) {
    if (!initialized_) return -1;
    int count = 0;
    while (Serial1.available() > 0 && count < (int)max_len) {
        buffer[count++] = Serial1.read();
    }
    return count;
}

int UartDriver::available() const {
    if (!initialized_) return 0;
    return Serial1.available();
}

void UartDriver::flush() {
    if (initialized_) Serial1.flush();
}

void UartDriver::setBaudRate(uint32_t baud) {
    if (initialized_) {
        Serial1.end();
        Serial1.begin(baud, SERIAL_8N1, rx_pin_, tx_pin_);
    }
}

} // namespace drivers
