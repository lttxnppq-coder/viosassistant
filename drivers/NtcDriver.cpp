#include "NtcDriver.h"
#include <Arduino.h>
#include <math.h>

namespace drivers {

bool NtcDriver::begin(const Config& config, uint8_t pin) {
    config_ = config;
    default_pin_ = pin;
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
    initialized_ = true;
    return true;
}

void NtcDriver::end() {
    initialized_ = false;
}

float NtcDriver::readTemperature(uint8_t pin) {
    if (!initialized_) return 0.0f;
    float voltage = readVoltage(pin);
    if (voltage <= 0.0f) return 0.0f;
    float resistance = (config_.vcc * config_.series_resistor / voltage) - config_.series_resistor;
    return resistanceToTemp(resistance);
}

float NtcDriver::readTemperatureC(uint8_t pin) {
    return readTemperature(pin);
}

void NtcDriver::setConfig(const Config& config) {
    config_ = config;
}

float NtcDriver::readVoltage(uint8_t pin) {
    int raw = analogRead(pin);
    if (raw < 0) return 0.0f;
    return (raw * config_.vcc) / config_.adc_max;
}

float NtcDriver::resistanceToTemp(float resistance) const {
    if (resistance <= 0.0f) return 0.0f;
    float temp_k = 1.0f / (1.0f / 298.15f + log(resistance / config_.r25) / config_.beta);
    return temp_k - 273.15f;
}

} // namespace drivers
