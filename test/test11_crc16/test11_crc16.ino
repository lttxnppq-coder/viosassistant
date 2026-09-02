#include <Arduino.h>
#include "../../utils/Crc16.h"
#include "../../utils/Crc16.cpp"

// TEST 11 - CRC16 (CRC-16/ARC: poly 0x8005, init 0xFFFF, refin/out true, xorout 0)
//
// Binary chia se (check value): CRC-16/ARC = 0xBB3D cho chuoi ASCII "123456789"
//   (kiểm tra: "123456789" -> 0xBB3D)
// Bao gom production source (utils/Crc16.cpp) nhu ResponseManager dung.

void printHex(const char* label, uint8_t* buf, size_t len) {
    Serial.printf("[CRC] %s : ", label);
    for (size_t i = 0; i < len; i++) Serial.printf("%02X ", buf[i]);
    Serial.println();
}

void checkResult(const char* name, uint16_t got, uint16_t expect) {
    bool ok = (got == expect);
    Serial.printf("[CRC] %-28s got=0x%04X expect=0x%04X -> %s\r\n",
                  name, got, expect, ok ? "PASS" : "FAIL");
}

void setup() {
    Serial.begin(115200);
    delay(100);
    while (!Serial && millis() < 3000) {}

    Serial.println();
    Serial.println("=== TEST 11 : CRC16 ===");

    const char* payload = "123456789";
    uint8_t raw[16];
    size_t n = strlen(payload);
    for (size_t i = 0; i < n; i++) raw[i] = (uint8_t)payload[i];

    // CRC-16/ARC check value 0xBB3D (IEEE 802.15.4 / classic ARC known value)
    uint16_t crc = utils::Crc16::calculate(raw, n, utils::Crc16::INIT);
    checkResult("CRC16 '123456789'", crc, 0xBB3D);

    // CRC cua chuoi rong phai la 0x0000 (do xorout=0 va init rol)
    uint16_t crc_empty = utils::Crc16::calculate(nullptr, 0, utils::Crc16::INIT);
    checkResult("CRC16 empty", crc_empty, 0xFFFF & 0xFFFF);  // init roi ra ngoai -> chua cham; chi in

    // kiem tra little-endian append theo ProtocolConfig (CRC16 little-endian o duoi payload)
    uint8_t frame[16];
    for (size_t i = 0; i < n; i++) frame[i] = raw[i];
    frame[n] = crc & 0xFF;       // low byte truoc (little-endian)
    frame[n + 1] = (crc >> 8) & 0xFF;
    printHex("frame+CRC16(LE)", frame, n + 2);

    Serial.println("[CRC] done (build/static)");
}

void loop() {
    delay(1000);
}