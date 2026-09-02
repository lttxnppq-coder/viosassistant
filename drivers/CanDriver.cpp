#include "CanDriver.h"

namespace drivers {

// STUB implementation - see header for rationale. No UART opened, no CAN bus access.

bool CanDriver::begin(uint32_t baud, uint8_t tx_pin, uint8_t rx_pin) {
    tx_pin_ = tx_pin;
    rx_pin_ = rx_pin;
    (void)baud;
    // Mark initialized for API compatibility, but do NOT open Serial2.
    // Caller must not treat initialized_ == true as "CAN operational".
    initialized_ = true;
    return true;
}

void CanDriver::end() {
    initialized_ = false;
}

bool CanDriver::write(uint32_t id, const uint8_t* data, uint8_t len, bool extended) {
    (void)id; (void)data; (void)len; (void)extended;
    // NOT_IMPLEMENTED: return false even if initialized_ to avoid fake success.
    // Application must handle failure (retry/log) rather than assume CAN TX succeeded.
    return false;
}

bool CanDriver::read(uint32_t& id, uint8_t* data, uint8_t& len, bool& extended) {
    (void)id; (void)data; (void)len; (void)extended;
    return false;
}

int CanDriver::available() const {
    return 0;
}

void CanDriver::setFilter(uint32_t id, uint32_t mask, bool extended) {
    (void)id; (void)mask; (void)extended;
}

} // namespace drivers
