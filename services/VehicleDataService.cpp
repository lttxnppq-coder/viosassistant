#include "VehicleDataService.h"
#include <Arduino.h>
#include <string.h>

namespace services {

bool VehicleDataService::begin(drivers::CanDriver* can, drivers::UartDriver* uart) {
    can_driver_ = can;
    uart_driver_ = uart;
    if (can_driver_ && !can_driver_->isInitialized()) {
        can_driver_->begin();
    }
    if (uart_driver_ && !uart_driver_->isInitialized()) {
        uart_driver_->begin();
    }
    initialized_ = true;
    return true;
}

void VehicleDataService::update() {
    if (!initialized_) return;

    if (can_driver_ && can_driver_->available() > 0) {
        uint32_t id; uint8_t data[8]; uint8_t len; bool ext;
        while (can_driver_->read(id, data, len, ext)) {
            parseCanFrame(id, data, len);
        }
    }

    if (uart_driver_ && uart_driver_->available() > 0) {
        uint8_t buffer[64];
        int len = uart_driver_->read(buffer, sizeof(buffer));
        if (len > 0) {
            parseUartFrame(buffer, len);
        }
    }

    if (millis() - last_request_ > 1000) {
        requestData();
    }
}

const model::VehicleData& VehicleDataService::getData() const {
    return vehicle_data_;
}

void VehicleDataService::requestData() {
    last_request_ = millis();
    if (can_driver_) {
        uint8_t req[] = {0x02, 0x01, 0x00};
        can_driver_->write(0x7DF, req, 3, false);
    }
}

bool VehicleDataService::parseCanFrame(uint32_t id, const uint8_t* data, uint8_t len) {
    (void)id; (void)data; (void)len;
    // STUB: No CAN frame spec finalized. Return false (NOT_IMPLEMENTED) - do not
    // mutate vehicle_data_ or set data_valid. Caller must treat as no new data.
    return false;
}

bool VehicleDataService::parseUartFrame(const uint8_t* data, uint8_t len) {
    (void)data; (void)len;
    // STUB: No UART frame spec finalized. Return false (NOT_IMPLEMENTED).
    return false;
}

void VehicleDataService::setCallback(void (*callback)(const model::VehicleData&)) {
    data_callback_ = callback;
}

} // namespace services
