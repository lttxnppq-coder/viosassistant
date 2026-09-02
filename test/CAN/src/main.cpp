#include <Arduino.h>
#include "driver/twai.h"

// ==================================================================
// NODE 1 - CAN TRANSMITTER (ESP32 + TWAI Driver)
// ------------------------------------------------------------------
// Gửi 1 CAN frame mỗi 1 giây (Standard frame, ID = 0x036, DLC = 8)
// ==================================================================

#define CAN_TX_PIN (gpio_num_t)5
#define CAN_RX_PIN (gpio_num_t)4

#define TX_INTERVAL_MS 1000  // chu kỳ gửi: 1 giây

static uint8_t counter = 0;  // bộ đếm tạo dữ liệu test

// In thông tin 1 frame ra Serial Monitor (dạng HEX)
static void printFrame(const twai_message_t &msg, const char *prefix) {
    Serial.printf("%s\n", prefix);
    Serial.printf("ID: 0x%03X\n", msg.identifier);
    Serial.printf("DLC: %d\n", msg.data_length_code);
    Serial.printf("DATA: ");
    for (uint8_t i = 0; i < msg.data_length_code; i++) {
        Serial.printf("%02X ", msg.data[i]);
    }
    Serial.println();
}

// Bật các alert cần theo dõi (được đọc bằng twai_read_alerts)
#define CAN_ALERTS_ENABLED (TWAI_ALERT_TX_SUCCESS | \
                            TWAI_ALERT_TX_FAILED  | \
                            TWAI_ALERT_BUS_ERROR  | \
                            TWAI_ALERT_ERR_PASS   | \
                            TWAI_ALERT_BUS_OFF)

void setup() {
    Serial.begin(115200);
    delay(200);

    // 1. Cấu hình tổng quát: chân TX/RX, chế độ NORMAL
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
    g_config.alerts_enabled = CAN_ALERTS_ENABLED;  // alert để theo dõi transmission
    g_config.tx_queue_len = 5;
    g_config.rx_queue_len = 5;

    // 2. Cấu hình timing: 500 kbps
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();

    // 3. Bộ lọc: chấp nhận tất cả CAN ID
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

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
}
void loop()
{
    static unsigned long lastTx = 0;
    static uint8_t idIndex = 0;

    unsigned long now = millis();

    // Gửi 1 frame mỗi 1 giây
    if (now - lastTx >= TX_INTERVAL_MS)
    {
        lastTx = now;

        twai_message_t msg = {};

        // ==========================================
        // LAB 4: Gửi xen kẽ 3 CAN ID
        // ==========================================
        if (idIndex == 0)
        {
            msg.identifier = 0x036;
        }
        else if (idIndex == 1)
        {
            msg.identifier = 0x100;
        }
        else
        {
            msg.identifier = 0x200;
        }

        // Chuyển sang ID tiếp theo
        idIndex++;

        if (idIndex >= 3)
        {
            idIndex = 0;
        }

        // Standard CAN frame
        msg.extd = 0;
        msg.data_length_code = 8;

        // Data test
        for (uint8_t i = 0; i < 8; i++)
        {
            msg.data[i] = counter + i;
        }

        counter++;

        // Gửi frame
        esp_err_t err = twai_transmit(
            &msg,
            pdMS_TO_TICKS(100)
        );

        if (err == ESP_OK)
        {
            Serial.println(
                "[TX] Queued to TX queue"
            );

            printFrame(
                msg,
                "[TX] Frame details:"
            );
        }
        else if (err == ESP_ERR_TIMEOUT)
        {
            Serial.println(
                "[TX] TX queue full"
            );
        }
        else
        {
            Serial.printf(
                "[TX] Send failed: %s\n",
                esp_err_to_name(err)
            );
        }
    }

    // ==========================================
    // Đọc TWAI alerts
    // ==========================================
    uint32_t alerts = 0;

    esp_err_t err = twai_read_alerts(
        &alerts,
        pdMS_TO_TICKS(50)
    );

    if (err == ESP_OK)
    {
        if (alerts & TWAI_ALERT_TX_SUCCESS)
        {
            Serial.println(
                "[TX] TX_SUCCESS: transmission completed"
            );
        }

        if (alerts & TWAI_ALERT_TX_FAILED)
        {
            Serial.println(
                "[TX] TX_FAILED: transmission failed"
            );

            twai_status_info_t status;

            if (twai_get_status_info(&status) == ESP_OK)
            {
                Serial.printf(
                    "[TX] state=%d tx_err=%u rx_err=%u "
                    "arb_lost=%u bus_err=%u tx_failed=%u\n",
                    (int)status.state,
                    status.tx_error_counter,
                    status.rx_error_counter,
                    status.arb_lost_count,
                    status.bus_error_count,
                    status.tx_failed_count
                );
            }
        }

        if (alerts & TWAI_ALERT_BUS_ERROR)
        {
            Serial.println(
                "[TX] BUS_ERROR"
            );
        }

        if (alerts & TWAI_ALERT_ERR_PASS)
        {
            Serial.println(
                "[TX] ERROR_PASSIVE"
            );
        }

        if (alerts & TWAI_ALERT_BUS_OFF)
        {
            Serial.println(
                "[TX] BUS_OFF"
            );
        }
    }
}