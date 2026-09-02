#pragma once

#include <cstdint>
#include "PinConfig.h"

namespace drivers {

// CanDriver - UART-to-CAN external module via GPIO11/12 (NOT ESP32 native TWAI)
// STATUS: STUB - NOT_IMPLEMENTED. Production code must treat as non-functional.
// - begin() sets initialized_ but does NOT open UART (no Serial2.begin) to avoid
//   fake "success" that would make application think CAN is operational.
// - write() returns false (FAIL/NOT_IMPLEMENTED) even when initialized_ is true
//   to satisfy safety rule: FAIL > fake success.
// - read() always false, available() always 0.
// Hardware decision: keep GPIO11/12, do not switch to TWAI. Protocol spec
// (frame format, baud, CRC) not yet defined - do not invent. Hardware test
// H09 must verify module model, wiring, termination before implementing.
// See FINALIZE_PRODUCTION_BEFORE_HARDWARE.md ISSUE C.
class CanDriver {
public:
    // CAN Module UART pins (GPIO11/12)
    bool begin(uint32_t baud = 500000, uint8_t tx_pin = PIN_CAN_UART_TX, uint8_t rx_pin = PIN_CAN_UART_RX);
    void end();
    // Write returns false (NOT_IMPLEMENTED) to avoid fake success.
    bool write(uint32_t id, const uint8_t* data, uint8_t len, bool extended = false);
    bool read(uint32_t& id, uint8_t* data, uint8_t& len, bool& extended);
    int available() const;
    void setFilter(uint32_t id, uint32_t mask, bool extended = false);
    bool isInitialized() const { return initialized_; }

private:
    bool initialized_ = false;
    uint8_t tx_pin_ = PIN_CAN_UART_TX;
    uint8_t rx_pin_ = PIN_CAN_UART_RX;
};

} // namespace drivers
