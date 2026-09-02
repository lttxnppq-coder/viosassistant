#pragma once

#include <cstdint>

namespace utils {

class Crc16 {
public:
    static constexpr uint16_t POLY = 0x8005;
    static constexpr uint16_t INIT = 0xFFFF;
    static constexpr uint16_t XOR_OUT = 0x0000;
    static constexpr bool REF_IN = true;
    static constexpr bool REF_OUT = true;

    static uint16_t calculate(const uint8_t* data, uint16_t len, uint16_t init = INIT);
    static uint16_t calculateByte(uint16_t crc, uint8_t data);
    static uint16_t table[256];
    static void generateTable();

private:
    static bool table_generated_;
};

} // namespace utils
