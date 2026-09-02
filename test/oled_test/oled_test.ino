#include <Wire.h>
#include <Adafruit_SSD1306.h>
#define OLED_SDA  8
#define OLED_SCL  9
#define OLED_ADDR 0x3C
#define OLED_W    128
#define OLED_H    64

Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);

bool oled_ok = false;
uint32_t count = 0;
unsigned long last_tick = 0;

void scanI2C() {
    Serial.println("I2C SCAN 0x03..0x77");
    uint8_t found[128];
    uint8_t n = 0;
    for (uint8_t addr = 0x03; addr <= 0x77; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) found[n++] = addr;
    }
    if (n == 0) {
        Serial.println("[I2C] NO DEVICE FOUND");
    } else {
        Serial.printf("[I2C] %u device(s):\r\n", n);
        for (uint8_t i = 0; i < n; i++) Serial.printf("  FOUND 0x%02X\r\n", found[i]);
    }
    bool hit = false;
    for (uint8_t i = 0; i < n; i++) if (found[i] == OLED_ADDR) hit = true;
    Serial.println(hit ? "[I2C] OLED 0x3C DETECTED" : "[I2C] OLED 0x3C NOT DETECTED");
}

void showHeader() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("ViosAssistant");
    display.println("OLED HARDWARE TEST");
    display.println();
    display.print("SDA: GPIO"); display.println(OLED_SDA);
    display.print("SCL: GPIO"); display.println(OLED_SCL);
    display.print("I2C: 0x");   display.println(OLED_ADDR, HEX);
    display.display();
}

void setup() {
    Serial.begin(115200);
    delay(100);
    while (!Serial && millis() < 3000) {}

    Serial.println("=== OLED HARDWARE TEST ===");
    Wire.begin(OLED_SDA, OLED_SCL);

    scanI2C();

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        oled_ok = false;
        Serial.println("[OLED] INIT FAIL: SSD1306 not responding at 0x3C");
        return;
    }
    oled_ok = true;
    Serial.println("[OLED] INIT PASS: SSD1306 128x64");

    display.clearDisplay();
    display.setTextSize(1);
    showHeader();
    delay(1500);
}

void loop() {
    if (!oled_ok) {
        if (millis() - last_tick >= 1000) {
            last_tick = millis();
            Serial.println("[OLED] INIT FAIL - check SDA/SCL/3.3V wiring");
        }
        return;
    }
    if (millis() - last_tick >= 1000) {
        last_tick = millis();
        count++;
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("TEST RUNNING");
        display.print("COUNT: ");
        display.println(count);
        display.display();
        Serial.printf("[OLED] COUNT: %lu\r\n", count);
    }
}