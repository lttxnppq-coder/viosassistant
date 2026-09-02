#include "ResponseManager.h"
#include "Crc16.h"
#include <Arduino.h>

namespace application {

bool ResponseManager::begin(drivers::UartDriver* uart) {
    uart_driver_ = uart;
    initialized_ = true;
    return true;
}

void ResponseManager::sendResponse(const model::CommandResponse& response) {
    if (!initialized_ || !uart_driver_) return;
    uint8_t frame[32];
    frame[0] = 0xAA;
    frame[1] = (uint8_t)response.cmd_type;
    frame[2] = response.success ? 0x01 : 0x00;
    frame[3] = response.error_code;
    frame[4] = response.response_len;
    for (uint8_t i = 0; i < response.response_len && i < 8; i++) {
        frame[5 + i] = response.response_data[i];
    }
    uint16_t crc = crc16(frame, 5 + response.response_len);
    frame[5 + response.response_len] = crc & 0xFF;
    frame[6 + response.response_len] = (crc >> 8) & 0xFF;
    frame[7 + response.response_len] = 0x55;
    uart_driver_->write(frame, 8 + response.response_len);
}

void ResponseManager::sendStatus(const model::SystemState& state) {
    if (!initialized_ || !uart_driver_) return;
    uint8_t frame[20];
    frame[0] = 0xAA;
    frame[1] = 0x80;
    frame[2] = (uint8_t)state.mode;
    frame[3] = (uint8_t)state.error;
    frame[4] = state.uptime_ms & 0xFF;
    frame[5] = (state.uptime_ms >> 8) & 0xFF;
    frame[6] = (state.uptime_ms >> 16) & 0xFF;
    frame[7] = (state.uptime_ms >> 24) & 0xFF;
    frame[8] = state.free_heap & 0xFF;
    frame[9] = (state.free_heap >> 8) & 0xFF;
    frame[10] = (state.free_heap >> 16) & 0xFF;
    frame[11] = (state.free_heap >> 24) & 0xFF;
    frame[12] = state.cpu_usage & 0xFF;
    frame[13] = (state.cpu_usage >> 8) & 0xFF;
    frame[14] = state.watchdog_ok ? 1 : 0;
    frame[15] = state.retry_count;
    uint16_t crc = crc16(frame, 16);
    frame[16] = crc & 0xFF;
    frame[17] = (crc >> 8) & 0xFF;
    frame[18] = 0x55;
    uart_driver_->write(frame, 19);
}

void ResponseManager::sendTemperatureData(const model::TemperatureData& temps) {
    if (!initialized_ || !uart_driver_) return;
    uint8_t frame[24];
    frame[0] = 0xAA;
    frame[1] = 0x81;
    int16_t t = (int16_t)(temps.inside_temp_c * 10);
    frame[2] = t & 0xFF; frame[3] = (t >> 8) & 0xFF;
    t = (int16_t)(temps.outside_temp_c * 10);
    frame[4] = t & 0xFF; frame[5] = (t >> 8) & 0xFF;
    t = (int16_t)(temps.evaporator_temp_c * 10);
    frame[6] = t & 0xFF; frame[7] = (t >> 8) & 0xFF;
    t = (int16_t)(temps.ambient_temp_c * 10);
    frame[8] = t & 0xFF; frame[9] = (t >> 8) & 0xFF;
    t = (int16_t)(temps.setpoint_temp_c * 10);
    frame[10] = t & 0xFF; frame[11] = (t >> 8) & 0xFF;
    frame[12] = temps.inside_valid ? 1 : 0;
    frame[13] = temps.outside_valid ? 1 : 0;
    frame[14] = temps.evaporator_valid ? 1 : 0;
    frame[15] = temps.ambient_valid ? 1 : 0;
    uint16_t crc = crc16(frame, 16);
    frame[16] = crc & 0xFF;
    frame[17] = (crc >> 8) & 0xFF;
    frame[18] = 0x55;
    uart_driver_->write(frame, 19);
}

void ResponseManager::sendVehicleData(const model::VehicleData& vehicle) {
    if (!initialized_ || !uart_driver_) return;
    uint8_t frame[24];
    frame[0] = 0xAA;
    frame[1] = 0x82;
    int16_t v = (int16_t)(vehicle.vehicle_speed_kmh * 10);
    frame[2] = v & 0xFF; frame[3] = (v >> 8) & 0xFF;
    v = (int16_t)(vehicle.engine_rpm / 10);
    frame[4] = v & 0xFF; frame[5] = (v >> 8) & 0xFF;
    v = (int16_t)(vehicle.coolant_temp_c * 10);
    frame[6] = v & 0xFF; frame[7] = (v >> 8) & 0xFF;
    v = (int16_t)(vehicle.battery_voltage_v * 100);
    frame[8] = v & 0xFF; frame[9] = (v >> 8) & 0xFF;
    frame[10] = vehicle.ac_compressor_active ? 1 : 0;
    frame[11] = vehicle.blower_active ? 1 : 0;
    frame[12] = vehicle.gear_position;
    frame[13] = vehicle.data_valid ? 1 : 0;
    uint16_t crc = crc16(frame, 14);
    frame[14] = crc & 0xFF;
    frame[15] = (crc >> 8) & 0xFF;
    frame[16] = 0x55;
    uart_driver_->write(frame, 17);
}

void ResponseManager::sendAck(uint8_t cmd_id, bool success) {
    if (!initialized_ || !uart_driver_) return;
    uint8_t frame[8];
    frame[0] = 0xAA;
    frame[1] = 0xFF;
    frame[2] = cmd_id;
    frame[3] = success ? 0x01 : 0x00;
    frame[4] = 0x00;
    uint16_t crc = crc16(frame, 5);
    frame[5] = crc & 0xFF;
    frame[6] = (crc >> 8) & 0xFF;
    frame[7] = 0x55;
    uart_driver_->write(frame, 8);
}

void ResponseManager::sendError(uint8_t cmd_id, uint8_t error_code) {
    if (!initialized_ || !uart_driver_) return;
    uint8_t frame[8];
    frame[0] = 0xAA;
    frame[1] = 0xFE;
    frame[2] = cmd_id;
    frame[3] = error_code;
    frame[4] = 0x00;
    uint16_t crc = crc16(frame, 5);
    frame[5] = crc & 0xFF;
    frame[6] = (crc >> 8) & 0xFF;
    frame[7] = 0x55;
    uart_driver_->write(frame, 8);
}

uint16_t ResponseManager::crc16(const uint8_t* data, uint16_t len) {
    return utils::Crc16::calculate(data, len);
}

} // namespace application
