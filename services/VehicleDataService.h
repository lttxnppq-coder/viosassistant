#pragma once

#include <cstdint>
#include "VehicleData.h"
#include "CanDriver.h"
#include "UartDriver.h"

namespace services {

// VehicleDataService - ingestion from CAN UART (GPIO11/12) + Pi UART (GPIO17/18)
// STATUS: PARTIAL STUB - parseCanFrame/parseUartFrame return false (NOT_IMPLEMENTED).
// No CAN/UART frame format spec has been finalized, so no fake parsing or fake
// valid data is produced. update() polls drivers but data_valid stays false until
// parsers are implemented. Do not treat as production-ready vehicle data source.
class VehicleDataService {
public:
    bool begin(drivers::CanDriver* can = nullptr, drivers::UartDriver* uart = nullptr);
    void update();
    const model::VehicleData& getData() const;
    void requestData();
    // Parsers: STUB - always return false (NOT_IMPLEMENTED), no data mutation.
    bool parseCanFrame(uint32_t id, const uint8_t* data, uint8_t len);
    bool parseUartFrame(const uint8_t* data, uint8_t len);
    void setCallback(void (*callback)(const model::VehicleData&));
    bool isInitialized() const { return initialized_; }

private:
    bool initialized_ = false;
    model::VehicleData vehicle_data_;
    drivers::CanDriver* can_driver_ = nullptr;
    drivers::UartDriver* uart_driver_ = nullptr;
    void (*data_callback_)(const model::VehicleData&) = nullptr;
    uint32_t last_request_ = 0;
    uint32_t last_update_ = 0;
};

} // namespace services
