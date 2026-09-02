#include <Arduino.h>
#include "driver/twai.h"

// ==================================================================
// NODE 2 - CAN RECEIVER (ESP32 + TWAI Driver)
// ------------------------------------------------------------------
// Liên tục nhận CAN frame từ bus và in ra Serial Monitor.
// Bộ lọc Accept-All: nhận MỌI CAN ID, không chỉ riêng 0x036.
// ==================================================================

#define CAN_TX_PIN (gpio_num_t)5
#define CAN_RX_PIN (gpio_num_t)4

#define RX_TIMEOUT_MS 1000  // thời gian chờ frame trong mỗi lần gọi twai_receive

// In thông tin 1 frame nhận được ra Serial Monitor (dạng HEX)
static void printFrame(const twai_message_t &msg) {
    Serial.println("[RX] CAN Frame Received");
    Serial.printf("ID: 0x%03X\n", msg.identifier);
    Serial.printf("DLC: %d\n", msg.data_length_code);
    Serial.printf("DATA: ");
    for (uint8_t i = 0; i < msg.data_length_code; i++) {
        Serial.printf("%02X ", msg.data[i]);
    }
    Serial.println();
}

// Bật các alert cần theo dõi (dùng để biết bus có hoạt động hay không)
#define CAN_ALERTS_ENABLED (TWAI_ALERT_RX_DATA   | \
                            TWAI_ALERT_BUS_ERROR | \
                            TWAI_ALERT_ERR_PASS  | \
                            TWAI_ALERT_BUS_OFF)

void setup() {
    Serial.begin(115200);
    delay(200);

    // 1. Cấu hình tổng quát: chân TX/RX, chế độ NORMAL
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
    g_config.alerts_enabled = CAN_ALERTS_ENABLED;
    g_config.tx_queue_len = 5;
    g_config.rx_queue_len = 10;  // đệm 10 frame cho RX

    // 2. Cấu hình timing: 500 kbps (PHẢI khớp với Node 1)
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();

    // 3. Bộ lọc: chỉ nhận ID 0x036 (LAB 4)
    //    (ESP-IDF v4.4 không có TWAI_FILTER_CONFIG_SINGLE -> dùng ACCEPT_ALL rồi ghi đè)
    twai_filter_config_t f_config =
    TWAI_FILTER_CONFIG_ACCEPT_ALL();
f_config.acceptance_code = 0x036 << 21;
f_config.acceptance_mask = ~(0x7FF << 21);
    // 4. Cài đặt driver
    esp_err_t err = twai_driver_install(&g_config, &t_config, &f_config);
    if (err != ESP_OK) {
        Serial.printf("[INIT] twai_driver_install FAILED: %s (0x%x)\n",
                      esp_err_to_name(err), err);
        return;  // dừng setup nếu lỗi
    }
    Serial.println("[INIT] twai_driver_install OK");

    // 5. Khởi động TWAI controller
    err = twai_start();
    if (err != ESP_OK) {
        Serial.printf("[INIT] twai_start FAILED: %s (0x%x)\n",
                      esp_err_to_name(err), err);
        return;
    }
    Serial.println("[INIT] twai_start OK");
    Serial.printf("[INIT] TX=GPIO%d  RX=GPIO%d  500kbps  Accept-All\n",
                  CAN_TX_PIN, CAN_RX_PIN);
    Serial.println("[RX] Waiting for CAN frames...");
}

void loop() {
    // Chờ tối đa 1 giây để nhận 1 frame.
    // ESP_OK           -> đã nhận frame, in ra màn hình
    // ESP_ERR_TIMEOUT  -> không có frame nào trong thời gian chờ (bus idle)
    twai_message_t msg;
    esp_err_t err = twai_receive(&msg, pdMS_TO_TICKS(RX_TIMEOUT_MS));

    if (err == ESP_OK) {
        printFrame(msg);
    } else if (err == ESP_ERR_TIMEOUT) {
        Serial.println("[RX] No frame within 1s (bus idle or not connected)");
    } else {
        Serial.printf("[RX] twai_receive error: %s (0x%x)\n",
                      esp_err_to_name(err), err);
    }
}
