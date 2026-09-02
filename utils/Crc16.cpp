#include "Crc16.h"

namespace utils {

uint16_t Crc16::table[256];
bool Crc16::table_generated_ = false;

void Crc16::generateTable() {
    for (uint16_t i = 0; i < 256; i++) {
        uint16_t crc = i;
        if (REF_IN) {
            crc = (crc >> 8) | (crc << 8);
        }
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ POLY;
            } else {
                crc >>= 1;
            }
        }
        if (REF_OUT) {
            crc = (crc >> 8) | (crc << 8);
        }
        table[i] = crc;
    }
    table_generated_ = true;
}

uint16_t Crc16::calculateByte(uint16_t crc, uint8_t data) {
    if (!table_generated_) generateTable();
    uint8_t index = (crc ^ data) & 0xFF;
    return (crc >> 8) ^ table[index];
}

uint16_t Crc16::calculate(const uint8_t* data, uint16_t len, uint16_t init) {
    if (!table_generated_) generateTable();
    uint16_t crc = init;
    for (uint16_t i = 0; i < len; i++) {
        crc = calculateByte(crc, data[i]);
    }
    return crc ^ XOR_OUT;
}

} // namespace utils
