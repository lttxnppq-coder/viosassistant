// I2C scanner doc lap — chi dung Wire core (KHONG dung Adafruit SSD1306).
//
// File nay nam trong src/ nhung duoc guard bang #ifdef I2C_SCANNER:
//   - Env esp32-s3 (main): khong define I2C_SCANNER -> file rong, khong
//     anh huong firmware.
//   - Env esp32-s3-scan: define I2C_SCANNER + build_src_filter chi lay
//     file nay -> build scanner doc lap, khong keo main.cpp/OLED code.
//
// Scan tat ca dia chi I2C tren bus SDA=GPIO41, SCL=GPIO42 va in ket qua
// ra Serial Monitor (115200). Khong ep dia chi: bao cao dia chi THUC te
// ma OLED/device tra ve.
//
// Build:  python -m platformio run -e esp32-s3-scan
// Upload: python -m platformio run -e esp32-s3-scan -t upload

#ifdef I2C_SCANNER

#include <Arduino.h>
#include <Wire.h>

#define OLED_SDA 41
#define OLED_SCL 42

void setup() {
    Serial.begin(115200);
    delay(200);

    Wire.begin(OLED_SDA, OLED_SCL);

    Serial.println("Scanning I2C...");

    byte error;
    int devices = 0;

    for (byte address = 0x03; address < 0x78; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
            Serial.print("Found device at 0x");
            if (address < 0x10) {
                Serial.print("0");
            }
            Serial.println(address, HEX);
            devices++;
        }
    }

    if (devices == 0) {
        Serial.println("No I2C devices found.");
    }

    Serial.println("Scan complete.");
}

void loop() {
    // Scan 1 lan trong setup(), loop rong.
}

#endif  // I2C_SCANNER
