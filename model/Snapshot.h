#pragma once

#include <cstdint>
#include "TemperatureData.h"
#include "VehicleData.h"
#include "SystemState.h"

namespace model {

struct Snapshot {
    TemperatureData temps{};
    VehicleData vehicle{};
    SystemState system{};
    uint32_t timestamp_ms = 0;
    uint32_t sequence = 0;
    bool valid = false;
};

} // namespace model