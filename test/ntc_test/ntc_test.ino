#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

#define NTC1_GPIO  1
#define NTC2_GPIO  2

#define OLED_SDA  8
#define OLED_SCL  9
#define OLED_ADDR 0x3C
#define OLED_W    128
#define OLED_H    64

#define STATUS_OK          0
#define STATUS_ADC_ERROR   1
#define STATUS_SATURATION  2
#define STATUS_V_INVALID   3
#define STATUS_R_INVALID   4

const float VCC       = 3.3f;
const float R_FIXED   = 10000.0f;
const float R25       = 10000.0f;
const float BETA      = 3950.0f;
const float ADC_MAX   = 4095.0f;
const float T0        = 298.15f;

Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);

bool oled_ok = false;
bool adc_ok = true;
unsigned long last_tick = 0;

struct NtcSample {
    int raw;
    float voltage;
    float resistance;
    float temp_c;
    int status;
};

NtcSample readNtc(uint8_t pin) {
    NtcSample s;
    s.raw = analogRead(pin);
    s.voltage = ((float)s.raw * VCC) / ADC_MAX;
    s.resistance = 0.0f;
    s.temp_c = 0.0f;

    if (s.raw == 0) {
        s.status = STATUS_ADC_ERROR;
        return s;
    }
    if (s.raw >= (int)ADC_MAX) {
        s.status = STATUS_SATURATION;
        return s;
    }
    if (s.voltage <= 0.0f) {
        s.status = STATUS_V_INVALID;
        return s;
    }
    s.resistance = (VCC * R_FIXED / s.voltage) - R_FIXED;
    if (s.resistance <= 0.0f) {
        s.status = STATUS_R_INVALID;
        return s;
    }
    s.temp_c = 1.0f / (1.0f / T0 + log(s.resistance / R25) / BETA) - 273.15f;
    s.status = STATUS_OK;
    return s;
}

const char* statusLabel(int status) {
    switch (status) {
        case STATUS_ADC_ERROR:  return "ADC ERROR";
        case STATUS_SATURATION: return "ADC SATURATION";
        case STATUS_V_INVALID:  return "VOLTAGE INVALID";
        case STATUS_R_INVALID:  return "RESISTANCE INVALID";
        default:                return "OK";
    }
}

void printSample(uint8_t pin, const char* name, const NtcSample& s) {
    Serial.printf("%s\r\n", name);
    Serial.printf("GPIO: %u\r\n", pin);
    Serial.printf("RAW: %u\r\n", s.raw);
    if (s.status == STATUS_OK) {
        Serial.printf("VOLTAGE: %.3f V\r\n", s.voltage);
        Serial.printf("RESISTANCE: %.2f ohm\r\n", s.resistance);
        Serial.printf("TEMP: %.2f C\r\n", s.temp_c);
    } else {
        Serial.printf("STATUS: %s\r\n", statusLabel(s.status));
    }
}

void showOled(const NtcSample& s1, const NtcSample& s2) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("NTC TEST");

    if (s1.status == STATUS_OK) {
        display.print("NTC1: "); display.println(s1.temp_c, 1);
    } else {
        display.println("NTC1: ERR");
    }
    display.print("RAW1: "); display.println(s1.raw);

    if (s2.status == STATUS_OK) {
        display.print("NTC2: "); display.println(s2.temp_c, 1);
    } else {
        display.println("NTC2: ERR");
    }
    display.print("RAW2: "); display.println(s2.raw);

    display.print(oled_ok ? "OLED: OK" : "OLED: INIT FAIL");
    display.setCursor(0, 56);
    display.print(adc_ok ? "ADC: OK" : "ADC: ERROR");
    display.display();
}

void setup() {
    Serial.begin(115200);
    delay(100);
    while (!Serial && millis() < 3000) {}

    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);

    Serial.println("=== NTC + OLED TEST ===");
    Serial.println("NTC1 GPIO1 ADC1_CH0");
    Serial.println("NTC2 GPIO2 ADC1_CH1");

    Wire.begin(OLED_SDA, OLED_SCL);

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        oled_ok = false;
        Serial.println("OLED: INIT FAIL");
    } else {
        oled_ok = true;
        Serial.println("OLED: INIT PASS (SSD1306 128x64 @0x3C)");
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("NTC TEST");
        display.display();
    }
}

void loop() {
    if (millis() - last_tick < 500) return;
    last_tick = millis();

    NtcSample s1 = readNtc(NTC1_GPIO);
    NtcSample s2 = readNtc(NTC2_GPIO);

    adc_ok = (s1.status == STATUS_OK) && (s2.status == STATUS_OK);

    printSample(NTC1_GPIO, "NTC1", s1);
    printSample(NTC2_GPIO, "NTC2", s2);
    Serial.printf("OLED: %s\r\n", oled_ok ? "OK" : "INIT FAIL");
    Serial.println("----");

    showOled(s1, s2);
}