#pragma once

#include <cstdint>

namespace ProtocolConfig {

constexpr uint32_t CAN_BAUD_RATE = 500000;
constexpr uint8_t CAN_TX_QUEUE_SIZE = 16;
constexpr uint8_t CAN_RX_QUEUE_SIZE = 16;

constexpr uint32_t UART_BAUD_RATE = 115200;
constexpr uint16_t UART_TX_BUFFER_SIZE = 256;
constexpr uint16_t UART_RX_BUFFER_SIZE = 256;

constexpr uint8_t CMD_HEADER = 0xAA;
constexpr uint8_t CMD_FOOTER = 0x55;
constexpr uint16_t CMD_MAX_PAYLOAD = 64;
constexpr uint32_t CMD_TIMEOUT_MS = 1000;

constexpr uint16_t VEHICLE_DATA_ID_BASE = 0x100;
constexpr uint16_t CLIMATE_CMD_ID_BASE = 0x200;
constexpr uint16_t SYSTEM_CMD_ID_BASE = 0x300;

constexpr uint8_t CRC16_POLY = 0x8005;
constexpr uint16_t CRC16_INIT = 0xFFFF;

} // namespace ProtocolConfig