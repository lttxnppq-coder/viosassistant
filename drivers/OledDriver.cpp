#include "OledDriver.h"

namespace drivers {

bool OledDriver::begin() {
    Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
    if (!display_.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
        return false;
    }
    display_.clearDisplay();
    display_.display();
    display_.setTextSize(1);
    display_.setTextColor(SSD1306_WHITE);
    display_.setTextWrap(false);
    initialized_ = true;
    return true;
}

void OledDriver::end() {
    initialized_ = false;
}

void OledDriver::clear() {
    if (initialized_) display_.clearDisplay();
}

void OledDriver::display() {
    if (initialized_) display_.display();
}

void OledDriver::drawString(int16_t x, int16_t y, const char* str, uint8_t size, uint16_t color) {
    if (!initialized_ || !str) return;
    display_.setCursor(x, y);
    display_.setTextSize(size);
    display_.setTextColor(color);
    display_.print(str);
}

void OledDriver::drawString(int16_t x, int16_t y, const String& str, uint8_t size, uint16_t color) {
    drawString(x, y, str.c_str(), size, color);
}

void OledDriver::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    if (initialized_) display_.drawLine(x0, y0, x1, y1, color);
}

void OledDriver::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (initialized_) display_.drawRect(x, y, w, h, color);
}

void OledDriver::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (initialized_) display_.fillRect(x, y, w, h, color);
}

void OledDriver::drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    if (initialized_) display_.drawCircle(x0, y0, r, color);
}

void OledDriver::fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    if (initialized_) display_.fillCircle(x0, y0, r, color);
}

void OledDriver::setCursor(int16_t x, int16_t y) {
    if (initialized_) display_.setCursor(x, y);
}

void OledDriver::setTextSize(uint8_t size) {
    if (initialized_) display_.setTextSize(size);
}

void OledDriver::setTextColor(uint16_t color, uint16_t bg) {
    if (initialized_) display_.setTextColor(color, bg);
}

void OledDriver::setTextWrap(bool wrap) {
    if (initialized_) display_.setTextWrap(wrap);
}

int16_t OledDriver::getCursorX() const {
    return initialized_ ? display_.getCursorX() : 0;
}

int16_t OledDriver::getCursorY() const {
    return initialized_ ? display_.getCursorY() : 0;
}

uint16_t OledDriver::width() const {
    return initialized_ ? display_.width() : OLED_WIDTH;
}

uint16_t OledDriver::height() const {
    return initialized_ ? display_.height() : OLED_HEIGHT;
}

void OledDriver::invertDisplay(bool invert) {
    if (initialized_) display_.invertDisplay(invert);
}

void OledDriver::dim(bool dim) {
    if (initialized_) display_.dim(dim);
}

} // namespace drivers
