#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "PinConfig.h"

namespace drivers {

class OledDriver {
public:
    bool begin();
    void end();
    void clear();
    void display();
    void drawString(int16_t x, int16_t y, const char* str, uint8_t size = 1, uint16_t color = SSD1306_WHITE);
    void drawString(int16_t x, int16_t y, const String& str, uint8_t size = 1, uint16_t color = SSD1306_WHITE);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color = SSD1306_WHITE);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color = SSD1306_WHITE);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color = SSD1306_WHITE);
    void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color = SSD1306_WHITE);
    void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color = SSD1306_WHITE);
    void setCursor(int16_t x, int16_t y);
    void setTextSize(uint8_t size);
    void setTextColor(uint16_t color, uint16_t bg = SSD1306_BLACK);
    void setTextWrap(bool wrap);
    int16_t getCursorX() const;
    int16_t getCursorY() const;
    uint16_t width() const;
    uint16_t height() const;
    void invertDisplay(bool invert);
    void dim(bool dim);
    bool isInitialized() const { return initialized_; }
    Adafruit_SSD1306& getDisplay() { return display_; }

private:
    bool initialized_ = false;
    Adafruit_SSD1306 display_{OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET_PIN};
};

} // namespace drivers
