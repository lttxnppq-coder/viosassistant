#include "ResponseManager.h"
#include "Crc16.h"
#include <Arduino.h>

namespace application {

// Unified frame: AA | ID | LEN | PAYLOAD | CRC_L | CRC_H | 55
// LEN at byte 2, CRC over AA ID LEN PAYLOAD (3+LEN bytes), LE
bool ResponseManager::begin(drivers::UartDriver* uart) {
    uart_driver_ = uart;
    initialized_ = true;
    return true;
}

void ResponseManager::sendResponse(const model::CommandResponse& response) {
    if (!initialized_ || !uart_driver_) return;
    // PAYLOAD = success(1) + error(1) + response_data
    uint8_t payloadLen = 2 + response.response_len;
    uint8_t frame[32];
    frame[0] = 0xAA;
    frame[1] = (uint8_t)response.cmd_type;
    frame[2] = payloadLen;
    frame[3] = response.success ? 0x01 : 0x00;
    frame[4] = response.error_code;
    for (uint8_t i = 0; i < response.response_len && i < 8; i++) {
        frame[5 + i] = response.response_data[i];
    }
    uint16_t crc = crc16(frame, 3 + payloadLen);
    frame[3 + payloadLen] = crc & 0xFF;
    frame[4 + payloadLen] = (crc >> 8) & 0xFF;
    frame[5 + payloadLen] = 0x55;
    uart_driver_->write(frame, 6 + payloadLen);
}

void ResponseManager::sendStatus(const model::SystemState& state) {
    if (!initialized_ || !uart_driver_) return;
    // PAYLOAD 14 bytes: mode, error, uptime(4), heap(4), cpu(2), watchdog(1), retry(1)
    uint8_t payloadLen = 14;
    uint8_t frame[32];
    frame[0] = 0xAA;
    frame[1] = 0x80;
    frame[2] = payloadLen;
    frame[3] = (uint8_t)state.mode;
    frame[4] = (uint8_t)state.error;
    frame[5] = state.uptime_ms & 0xFF;
    frame[6] = (state.uptime_ms >> 8) & 0xFF;
    frame[7] = (state.uptime_ms >> 16) & 0xFF;
    frame[8] = (state.uptime_ms >> 24) & 0xFF;
    frame[9] = state.free_heap & 0xFF;
    frame[10] = (state.free_heap >> 8) & 0xFF;
    frame[11] = (state.free_heap >> 16) & 0xFF;
    frame[12] = (state.free_heap >> 24) & 0xFF;
    frame[13] = state.cpu_usage & 0xFF;
    frame[14] = (state.cpu_usage >> 8) & 0xFF;
    frame[15] = state.watchdog_ok ? 1 : 0;
    frame[16] = state.retry_count;
    uint16_t crc = crc16(frame, 3 + payloadLen);
    frame[3 + payloadLen] = crc & 0xFF;
    frame[4 + payloadLen] = (crc >> 8) & 0xFF;
    frame[5 + payloadLen] = 0x55;
    uart_driver_->write(frame, 6 + payloadLen);
}

void ResponseManager::sendTemperatureData(const model::TemperatureData& temps) {
    if (!initialized_ || !uart_driver_) return;
    // PAYLOAD 14 bytes: inside*10(2) outside*10(2) evap*10(2) ambient*10(2) setpoint*10(2) valid*4(4)
    uint8_t payloadLen = 14;
    uint8_t frame[32];
    frame[0] = 0xAA;
    frame[1] = 0x81;
    frame[2] = payloadLen;
    int16_t t = (int16_t)(temps.inside_temp_c * 10);
    frame[3] = t & 0xFF; frame[4] = (t >> 8) & 0xFF;
    t = (int16_t)(temps.outside_temp_c * 10);
    frame[5] = t & 0xFF; frame[6] = (t >> 8) & 0xFF;
    t = (int16_t)(temps.evaporator_temp_c * 10);
    frame[7] = t & 0xFF; frame[8] = (t >> 8) & 0xFF;
    t = (int16_t)(temps.ambient_temp_c * 10);
    frame[9] = t & 0xFF; frame[10] = (t >> 8) & 0xFF;
    t = (int16_t)(temps.setpoint_temp_c * 10);
    frame[11] = t & 0xFF; frame[12] = (t >> 8) & 0xFF;
    frame[13] = temps.inside_valid ? 1 : 0;
    frame[14] = temps.outside_valid ? 1 : 0;
    frame[15] = temps.evaporator_valid ? 1 : 0;
    frame[16] = temps.ambient_valid ? 1 : 0;
    uint16_t crc = crc16(frame, 3 + payloadLen);
    frame[3 + payloadLen] = crc & 0xFF;
    frame[4 + payloadLen] = (crc >> 8) & 0xFF;
    frame[5 + payloadLen] = 0x55;
    uart_driver_->write(frame, 6 + payloadLen);
}

void ResponseManager::sendVehicleData(const model::VehicleData& vehicle) {
    if (!initialized_ || !uart_driver_) return;
    // PAYLOAD 12 bytes: speed*10(2) rpm/10(2) coolant*10(2) battery*100(2) ac(1) blower(1) gear(1) valid(1)
    uint8_t payloadLen = 12;
    uint8_t frame[32];
    frame[0] = 0xAA;
    frame[1] = 0x82;
    frame[2] = payloadLen;
    int16_t v = (int16_t)(vehicle.vehicle_speed_kmh * 10);
    frame[3] = v & 0xFF; frame[4] = (v >> 8) & 0xFF;
    v = (int16_t)(vehicle.engine_rpm / 10);
    frame[5] = v & 0xFF; frame[6] = (v >> 8) & 0xFF;
    v = (int16_t)(vehicle.coolant_temp_c * 10);
    frame[7] = v & 0xFF; frame[8] = (v >> 8) & 0xFF;
    v = (int16_t)(vehicle.battery_voltage_v * 100);
    frame[9] = v & 0xFF; frame[10] = (v >> 8) & 0xFF;
    frame[11] = vehicle.ac_compressor_active ? 1 : 0;
    frame[12] = vehicle.blower_active ? 1 : 0;
    frame[13] = vehicle.gear_position;
    frame[14] = vehicle.data_valid ? 1 : 0;
    uint16_t crc = crc16(frame, 3 + payloadLen);
    frame[3 + payloadLen] = crc & 0xFF;
    frame[4 + payloadLen] = (crc >> 8) & 0xFF;
    frame[5 + payloadLen] = 0x55;
    uart_driver_->write(frame, 6 + payloadLen);
}

void ResponseManager::sendAck(uint8_t cmd_id, bool success) {
    if (!initialized_ || !uart_driver_) return;
    // PAYLOAD 2 bytes: cmd_id, success
    uint8_t payloadLen = 2;
    uint8_t frame[16];
    frame[0] = 0xAA;
    frame[1] = 0xFF;
    frame[2] = payloadLen;
    frame[3] = cmd_id;
    frame[4] = success ? 0x01 : 0x00;
    uint16_t crc = crc16(frame, 3 + payloadLen);
    frame[3 + payloadLen] = crc & 0xFF;
    frame[4 + payloadLen] = (crc >> 8) & 0xFF;
    frame[5 + payloadLen] = 0x55;
    uart_driver_->write(frame, 6 + payloadLen);
}

void ResponseManager::sendError(uint8_t cmd_id, uint8_t error_code) {
    if (!initialized_ || !uart_driver_) return;
    // PAYLOAD 2 bytes: cmd_id, error_code
    uint8_t payloadLen = 2;
    uint8_t frame[16];
    frame[0] = 0xAA;
    frame[1] = 0xFE;
    frame[2] = payloadLen;
    frame[3] = cmd_id;
    frame[4] = error_code;
    uint16_t crc = crc16(frame, 3 + payloadLen);
    frame[3 + payloadLen] = crc & 0xFF;
    frame[4 + payloadLen] = (crc >> 8) & 0xFF;
    frame[5 + payloadLen] = 0x55;
    uart_driver_->write(frame, 6 + payloadLen);
}

uint16_t ResponseManager::crc16(const uint8_t* data, uint16_t len) {
    return utils::Crc16::calculate(data, len);
}

} // namespace application
