#include <Arduino.h>
#include "driver/twai.h"

// ==================================================================
// NODE 2 - CAN RECEIVER: nhận ID 0x036, ghép uint16_t LE, in Serial
// ==================================================================

#define CAN_TX_PIN (gpio_num_t)5
#define CAN_RX_PIN (gpio_num_t)4
#define CAN_ID        0x036
#define RX_TIMEOUT_MS 1000

void setup() {
    Serial.begin(115200);
    delay(200);

    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
    g_config.alerts_enabled = TWAI_ALERT_RX_DATA | TWAI_ALERT_BUS_ERROR |
                              TWAI_ALERT_ERR_PASS | TWAI_ALERT_BUS_OFF;
    g_config.tx_queue_len = 5;
    g_config.rx_queue_len = 10;

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
    Serial.println("[INIT] TWAI OK - waiting for frames...");
}

void loop() {
    twai_message_t msg;
    esp_err_t err = twai_receive(&msg, pdMS_TO_TICKS(RX_TIMEOUT_MS));

    if (err == ESP_OK) {
        // Ghép 2 byte little-endian -> uint16_t (chưa từng cộng dồn)
        uint16_t pedal        = msg.data[0] | (msg.data[1] << 8);
        uint16_t rpm          = msg.data[2] | (msg.data[3] << 8);
        uint16_t vehicleSpeed = msg.data[4] | (msg.data[5] << 8);

        Serial.println("[RX]");
        Serial.printf("ID: 0x%03X\n", msg.identifier);
        Serial.printf("DLC: %d\n", msg.data_length_code);
        Serial.printf("DATA: ");
        for (uint8_t i = 0; i < msg.data_length_code; i++) {
            Serial.printf("%02X ", msg.data[i]);
        }
        Serial.println();
        Serial.println();

        Serial.printf("Pedal: %u %%\n", pedal);
        Serial.printf("RPM: %u\n", rpm);
        Serial.printf("Vehicle Speed: %u km/h\n", vehicleSpeed);
        Serial.println();
    } else if (err == ESP_ERR_TIMEOUT) {
        Serial.println("[RX] No frame within 1s (bus idle or not connected)");
    } else {
        Serial.printf("[RX] twai_receive error: %s\n", esp_err_to_name(err));
    }
}