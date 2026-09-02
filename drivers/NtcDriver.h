#pragma once

#include <cstdint>
#include "PinConfig.h"

namespace drivers {

class NtcDriver {
public:
    struct Config {
        uint16_t r25;
        uint16_t beta;
        uint16_t series_resistor;
        uint16_t adc_max;
        float vcc;
        Config() : r25(10000), beta(3950), series_resistor(10000), adc_max(4095), vcc(3.3f) {}
        Config(uint16_t r, uint16_t b, uint16_t sr, uint16_t adc, float v)
            : r25(r), beta(b), series_resistor(sr), adc_max(adc), vcc(v) {}
    };

    // NTC ADC pins: GPIO1 and GPIO2
    bool begin(const Config& config = Config{}, uint8_t pin = PIN_NTC1_ADC);
    void end();
    float readTemperature(uint8_t pin);
    float readTemperatureC(uint8_t pin);
    void setConfig(const Config& config);
    bool isInitialized() const { return initialized_; }

private:
    bool initialized_ = false;
    Config config_;
    uint8_t default_pin_ = PIN_NTC1_ADC;
    float readVoltage(uint8_t pin);
    float resistanceToTemp(float resistance) const;
};

} // namespace drivers
