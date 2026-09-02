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

void loop() {
    static unsigned long lastTx = 0;

    // ---- Gửi frame mỗi 1 giây (dùng millis, không dùng delay dài) ----
    unsigned long now = millis();
    if (now - lastTx >= TX_INTERVAL_MS) {
        lastTx = now;

        twai_message_t msg = {};
        msg.identifier = 0x036;   // CAN ID 11-bit (standard frame)
        msg.extd = 0;             // 0 = standard frame
        msg.data_length_code = 8; // DLC = 8

        // Dữ liệu test: counter -> counter+7
        for (uint8_t i = 0; i < 8; i++) {
            msg.data[i] = counter + i;
        }
        counter++;

        // LƯU Ý về twai_transmit():
        // - ESP_OK  : frame chỉ mới được ĐƯA VÀO TX QUEUE của driver.
        //             KHÔNG khẳng định frame đã truyền thành công trên bus.
        // - ESP_ERR_TIMEOUT: hết 100ms chờ mà TX queue vẫn đầy.
        // Trạng thái truyền thực tế (thành công/thất bại) được theo dõi
        // bằng TWAI alerts bên dưới.
        esp_err_t err = twai_transmit(&msg, pdMS_TO_TICKS(100));
        if (err == ESP_OK) {
            Serial.println("[TX] Queued to TX queue (chua xac nhan len bus)");
            printFrame(msg, "[TX] Frame details:");
        } else if (err == ESP_ERR_TIMEOUT) {
            Serial.println("[TX] Send failed: TX queue full (timeout 100ms)");
        } else {
            Serial.printf("[TX] Send failed: %s (0x%x)\n",
                          esp_err_to_name(err), err);
        }
    }

    // ---- Đọc TWAI alerts để theo dõi trạng thái transmission ----
    uint32_t alerts = 0;
    esp_err_t err = twai_read_alerts(&alerts, pdMS_TO_TICKS(50));
    if (err == ESP_OK) {
        if (alerts & TWAI_ALERT_TX_SUCCESS) {
            // TWAI báo transmission đã hoàn thành thành công.
            // KHÔNG khẳng định tuyệt đối rằng có node khác ACK -
            // API không cung cấp thông tin để chứng minh điều đó.
            Serial.println("[TX] TWAI_ALERT_TX_SUCCESS: transmission completed");
        }
        if (alerts & TWAI_ALERT_TX_FAILED) {
            // TWAI báo transmission thất bại.
            // KHÔNG mặc định nguyên nhân (có thể là arbitration loss,
            // ACK error, ...). Chỉ in các trạng thái/error counters
            // mà API thực sự cung cấp để tự đối chiếu:
            Serial.println("[TX] TWAI_ALERT_TX_FAILED: transmission failed");

            twai_status_info_t status;
            if (twai_get_status_info(&status) == ESP_OK) {
                Serial.printf("[TX] status: state=%d tx_err=%u rx_err=%u "
                              "arb_lost=%u bus_err=%u tx_failed=%u\n",
                              (int)status.state,
                              status.tx_error_counter,
                              status.rx_error_counter,
                              status.arb_lost_count,
                              status.bus_error_count,
                              status.tx_failed_count);
            }
        }
        if (alerts & TWAI_ALERT_BUS_ERROR) {
            Serial.println("[TX] TWAI_ALERT_BUS_ERROR: bus error occurred");
        }
        if (alerts & TWAI_ALERT_ERR_PASS) {
            Serial.println("[TX] TWAI_ALERT_ERR_PASS: controller is error-passive");
        }
        if (alerts & TWAI_ALERT_BUS_OFF) {
            Serial.println("[TX] TWAI_ALERT_BUS_OFF: controller is bus-off");
        }
    } else if (err != ESP_ERR_TIMEOUT) {
        // ESP_ERR_TIMEOUT là bình thường khi không có alert nào trong 50ms
        Serial.printf("[TX] twai_read_alerts error: %s (0x%x)\n",
                      esp_err_to_name(err), err);
    }
}
