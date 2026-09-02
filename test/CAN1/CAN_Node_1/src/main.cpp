#include <Arduino.h>
#include "driver/twai.h"

// ==================================================================
// NODE 1 - CAN TRANSMITTER: đọc 3 biến trở -> CAN ID 0x036 mỗi 100ms
// data[0..1]=pedal, data[2..3]=rpm, data[4..5]=vehicleSpeed,
// data[6..7]=reserved=0  (uint16_t, little-endian)
// ==================================================================

#define CAN_TX_PIN (gpio_num_t)5
#define CAN_RX_PIN (gpio_num_t)4

#define POT_PEDAL_PIN 32   // ADC1 -> pedal 0..100 %
#define POT_RPM_PIN   33   // ADC1 -> rpm   0..4000 RPM
#define POT_SPEED_PIN 34   // ADC1 -> speed 0..200 km/h

#define CAN_ID         0x036
#define TX_INTERVAL_MS 100   // gửi mỗi 100ms (dùng millis)

#define CAN_ALERTS_ENABLED (TWAI_ALERT_TX_SUCCESS | \
                            TWAI_ALERT_TX_FAILED  | \
                            TWAI_ALERT_BUS_ERROR  | \
                            TWAI_ALERT_ERR_PASS   | \
                            TWAI_ALERT_BUS_OFF)

static void pack16(uint8_t *out, uint16_t value) {
    out[0] = value & 0xFF;
    out[1] = (value >> 8) & 0xFF;
}

void setup() {
    Serial.begin(115200);
    delay(200);

    analogSetPinAttenuation(POT_PEDAL_PIN, ADC_11db);
    analogSetPinAttenuation(POT_RPM_PIN,   ADC_11db);
    analogSetPinAttenuation(POT_SPEED_PIN, ADC_11db);

    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
    g_config.alerts_enabled = CAN_ALERTS_ENABLED;
    g_config.tx_queue_len = 5;
    g_config.rx_queue_len = 5;

    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    esp_err_t err = twai_driver_install(&g_config, &t_config, &f_config);
    if (err != ESP_OK) {
        Serial.printf("[INIT] twai_driver_install FAILED: %s\n", esp_err_to_name(err));
        return;
    }
    err = twai_start();
    if (err != ESP_OK) {
        Serial.printf("[INIT] twai_start FAILED: %s\n", esp_err_to_name(err));
        return;
    }
    Serial.println("[INIT] TWAI OK - 500kbps, TX=GPIO5 RX=GPIO4");
}

void loop() {
    static unsigned long lastTx = 0;
    unsigned long now = millis();

    if (now - lastTx >= TX_INTERVAL_MS) {
        lastTx = now;

        // Đọc ADC TẠI THỜI ĐIỂM GỬI và map ngay -> không cộng dồn
        uint16_t pedal        = map(analogRead(POT_PEDAL_PIN), 0, 4095, 0, 100);
        uint16_t rpm          = map(analogRead(POT_RPM_PIN),   0, 4095, 0, 4000);
        uint16_t vehicleSpeed = map(analogRead(POT_SPEED_PIN), 0, 4095, 0, 200);

        Serial.println("[TX]");
        Serial.printf("Pedal: %u %%\n", pedal);
        Serial.printf("RPM: %u\n", rpm);
        Serial.printf("Vehicle Speed: %u km/h\n", vehicleSpeed);
        Serial.println();

        twai_message_t msg = {};
        msg.identifier = CAN_ID;
        msg.extd = 0;
        msg.data_length_code = 8;
        pack16(&msg.data[0], pedal);
        pack16(&msg.data[2], rpm);
        pack16(&msg.data[4], vehicleSpeed);
        msg.data[6] = 0;
        msg.data[7] = 0;

        Serial.printf("DATA: ");
        for (uint8_t i = 0; i < 8; i++) Serial.printf("%02X ", msg.data[i]);
        Serial.println();

        esp_err_t err = twai_transmit(&msg, pdMS_TO_TICKS(100));
        if (err == ESP_ERR_TIMEOUT) {
            Serial.println("[TX] TX queue full");
        } else if (err != ESP_OK) {
            Serial.printf("[TX] Send failed: %s\n", esp_err_to_name(err));
        }
    }

    uint32_t alerts = 0;
    if (twai_read_alerts(&alerts, pdMS_TO_TICKS(50)) == ESP_OK) {
        if (alerts & TWAI_ALERT_TX_SUCCESS) Serial.println("[TX] Transmission success");
        if (alerts & TWAI_ALERT_TX_FAILED) {
            Serial.println("[TX] Transmission failed (no ACK? check bus)");
            twai_status_info_t status;
            if (twai_get_status_info(&status) == ESP_OK) {
                Serial.printf("[TX] state=%d tx_err=%u rx_err=%u\n",
                              (int)status.state, status.tx_error_counter, status.rx_error_counter);
            }
        }
        if (alerts & TWAI_ALERT_BUS_ERROR) Serial.println("[TX] BUS_ERROR");
        if (alerts & TWAI_ALERT_ERR_PASS)  Serial.println("[TX] ERROR_PASSIVE");
        if (alerts & TWAI_ALERT_BUS_OFF)   Serial.println("[TX] BUS_OFF");
    }
}