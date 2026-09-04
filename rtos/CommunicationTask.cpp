#include "CommunicationTask.h"
#include "ResponseManager.h"
#include "Crc16.h"
#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>

namespace rtos {

application::SystemManager* CommunicationTask::sys_mgr_ = nullptr;
drivers::CanDriver* CommunicationTask::can_driver_ = nullptr;
drivers::UartDriver* CommunicationTask::uart_driver_ = nullptr;
application::ResponseManager* CommunicationTask::resp_mgr_ = nullptr;
TaskHandle_t CommunicationTask::task_handle_ = nullptr;
bool CommunicationTask::running_ = false;
// Parser state for Jetson binary protocol (H03)
CommunicationTask::ParserState CommunicationTask::parser_state_ = ParserState::WAIT_HEADER;
uint8_t CommunicationTask::parser_id_ = 0;
uint8_t CommunicationTask::parser_len_ = 0;
uint8_t CommunicationTask::parser_payload_[64] = {0};
uint8_t CommunicationTask::parser_payload_idx_ = 0;
uint8_t CommunicationTask::parser_crc_l_ = 0;
uint8_t CommunicationTask::parser_crc_h_ = 0;
// Diagnostic counters — real, no fake
volatile uint32_t CommunicationTask::rxCount_ = 0;
volatile uint32_t CommunicationTask::txCount_ = 0;
volatile uint8_t CommunicationTask::lastRxId_ = 0;
volatile uint8_t CommunicationTask::lastRxLen_ = 0;
volatile bool CommunicationTask::lastRxCrcOk_ = false;
volatile bool CommunicationTask::lastRxOk_ = false;
volatile uint8_t CommunicationTask::lastTxId_ = 0;
volatile uint8_t CommunicationTask::lastTxLen_ = 0;
volatile bool CommunicationTask::lastTxOk_ = false;
volatile uint8_t CommunicationTask::lastErrorCode_ = 0;
volatile bool CommunicationTask::hasRx_ = false;
volatile bool CommunicationTask::hasTx_ = false;

bool CommunicationTask::begin(application::SystemManager* sys_mgr, drivers::CanDriver* can, drivers::UartDriver* uart) {
    return begin(sys_mgr, can, uart, nullptr);
}

bool CommunicationTask::begin(application::SystemManager* sys_mgr, drivers::CanDriver* can, drivers::UartDriver* uart, application::ResponseManager* resp_mgr) {
    sys_mgr_ = sys_mgr;
    can_driver_ = can;
    uart_driver_ = uart;
    resp_mgr_ = resp_mgr;
    parser_state_ = ParserState::WAIT_HEADER;
    running_ = true;
    BaseType_t result = xTaskCreate(
        taskFunction,
        "CommTask",
        4096,
        nullptr,
        3,
        &task_handle_
    );
    return result == pdPASS;
}

void CommunicationTask::end() {
    running_ = false;
    if (task_handle_) {
        vTaskDelete(task_handle_);
        task_handle_ = nullptr;
    }
}

void CommunicationTask::taskFunction(void* parameter) {
    (void)parameter;
    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(50);

    while (running_) {
        processCanMessages();
        processUartMessages();
        sendPeriodicUpdates();
        vTaskDelayUntil(&last_wake, period);
    }
    vTaskDelete(nullptr);
}

void CommunicationTask::processCanMessages() {
    if (!can_driver_) return;
    while (can_driver_->available() > 0) {
        uint32_t id; uint8_t data[8]; uint8_t len; bool ext;
        if (can_driver_->read(id, data, len, ext)) {
            // Process CAN frame
        }
    }
}

uint16_t CommunicationTask::crc16(const uint8_t* data, uint16_t len) {
    return utils::Crc16::calculate(data, len);
}

CommunicationTask::CommDiag CommunicationTask::getDiag() {
    CommDiag d;
    d.rxCount = rxCount_;
    d.txCount = txCount_;
    d.lastRxId = lastRxId_;
    d.lastRxLen = lastRxLen_;
    d.lastRxCrcOk = lastRxCrcOk_;
    d.lastRxOk = lastRxOk_;
    d.lastTxId = lastTxId_;
    d.lastTxLen = lastTxLen_;
    d.lastTxOk = lastTxOk_;
    d.lastError = lastErrorCode_;
    d.hasRx = hasRx_;
    d.hasTx = hasTx_;
    return d;
}

void CommunicationTask::incTx(uint8_t id, uint8_t len) {
    txCount_++;
    lastTxId_ = id;
    lastTxLen_ = len;
    lastTxOk_ = true;
    hasTx_ = true;
}

bool CommunicationTask::handleFrame(uint8_t id, uint8_t len, const uint8_t* payload) {
    if (!sys_mgr_) return false;
    model::Command cmd;
    model::CommandResponse resp;
    bool mapped = true;

    // PRODUCTION CONTRACT ONLY — model/Command.h
    // Valid CommandType IDs: 0x01 SET_TEMPERATURE (payload 23-30), 0x02 SET_FAN_SPEED (payload 0..5 or 6..255), 0x03 SET_AIR_MODE (payload 8/9/10 -> 0/2/4 or 0/2/4), 0x05 SET_AC (payload 0/1)
    // No demo/raw IDs (1,2,4,5,6,7,8,9,10,101..105) in production. No 0x04/0x05 delta markers.
    switch (id) {
        case (uint8_t)model::CommandType::SET_AC: // 0x05
            if (len >= 1) {
                cmd.type = model::CommandType::SET_AC;
                cmd.payload[0] = payload[0] ? 1 : 0;
                cmd.payload_len = 1;
            } else {
                mapped = false;
            }
            break;
        case (uint8_t)model::CommandType::SET_TEMPERATURE: // 0x01 — FROZEN 23-30
            if (len == 1) {
                // Single-byte absolute 23-30
                if (payload[0] < 23 || payload[0] > 30) {
                    mapped = false;
                } else {
                    cmd.type = model::CommandType::SET_TEMPERATURE;
                    cmd.payload[0] = payload[0];
                    cmd.payload[1] = 0;
                    cmd.payload_len = 2;
                }
            } else if (len >= 2) {
                // Two-byte: payload[0] + payload[1]*0.1 — validate 23-30 at boundary
                float t = (float)payload[0] + (float)payload[1] * 0.1f;
                if (t < 23.0f || t > 30.0f) {
                    mapped = false;
                } else {
                    cmd.type = model::CommandType::SET_TEMPERATURE;
                    cmd.payload[0] = payload[0];
                    cmd.payload[1] = payload[1];
                    cmd.payload_len = 2;
                }
            } else {
                mapped = false;
            }
            break;
        case (uint8_t)model::CommandType::SET_FAN_SPEED: // 0x02
            if (len == 0) {
                // FAN ON (restore) — no payload
                cmd.type = model::CommandType::SET_FAN_SPEED;
                cmd.payload_len = 0;
            } else {
                // len >= 1 — single contract: payload 0=OFF, 1-5=Level, 6-255=raw PWM
                uint8_t v = payload[0];
                cmd.type = model::CommandType::SET_FAN_SPEED;
                cmd.payload[0] = v;
                cmd.payload_len = 1;
                // Note: Level 1-5 mapping to PWM is hardware calibration (FanController), not changed here
            }
            break;
        case (uint8_t)model::CommandType::SET_AIR_MODE: // 0x03
            if (len >= 1) {
                uint8_t mode = payload[0];
                // Payload contract: 8=FACE, 9=FOOT, 10=DEFROST (also accept ESP enum 0/2/4)
                if (mode == 8) mode = 0; // FACE -> VENT
                else if (mode == 9) mode = 2; // FOOT -> FLOOR
                else if (mode == 10) mode = 4; // DEFROST -> DEFROST
                // Modes 0,2,4 are mutually exclusive FACE/FOOT/DEFROST
                if (mode != 0 && mode != 2 && mode != 4) {
                    mapped = false;
                    break;
                }
                cmd.type = model::CommandType::SET_AIR_MODE;
                cmd.payload[0] = mode;
                cmd.payload_len = 1;
            } else {
                mapped = false;
            }
            break;
        default:
            // Unknown ID — no demo fallback in production
            mapped = false;
            break;
    }

    // Diagnostic — real RX frame (CRC already OK here)
    lastRxId_ = id;
    lastRxLen_ = len;
    lastRxCrcOk_ = true;
    hasRx_ = true;
    // Serial debug RX
    Serial.printf("[COMM RX] ID: 0x%02X LEN: %u PAYLOAD:", id, len);
    for (uint8_t i = 0; i < len; i++) Serial.printf(" %02X", payload[i]);
    Serial.printf(" CRC: OK\n");

    if (!mapped) {
        lastRxOk_ = false;
        lastErrorCode_ = 1;
        rxCount_++; // valid frame but invalid mapping still counts as RX
        hasRx_ = true;
        Serial.printf("[COMM RX] -> INVALID (mapped=false) ID:0x%02X\n", id);
        if (resp_mgr_) {
            resp_mgr_->sendError(id, 1); // INVALID
            incTx(0xFE, 2);
            Serial.printf("[COMM TX] ID: 0xFE LEN: 2 PAYLOAD: %02X 01 CRC: ...\n", id);
        }
        return false;
    }

    cmd.timestamp = millis();
    // Dispatch via SystemManager
    sys_mgr_->handleCommand(cmd, resp);
    // Update RX diagnostic after dispatch
    lastRxOk_ = resp.success;
    lastErrorCode_ = resp.error_code;
    if (resp.success) rxCount_++;
    else {
        // Even if CommandManager says NOT_IMPLEMENTED, it is still a valid RX frame, count it
        rxCount_++;
    }
    // Send ACK/ERROR via ResponseManager
    if (resp_mgr_) {
        if (resp.success) {
            resp_mgr_->sendResponse(resp);
            // TX diagnostic — response frame ID = cmd_type, LEN = 2+resp_len
            uint8_t txLen = 2 + resp.response_len;
            incTx((uint8_t)resp.cmd_type, txLen);
            Serial.printf("[COMM TX] ID: 0x%02X LEN: %u PAYLOAD: %02X %02X", (uint8_t)resp.cmd_type, txLen, resp.success?1:0, resp.error_code);
            for (uint8_t i=0;i<resp.response_len;i++) Serial.printf(" %02X", resp.response_data[i]);
            Serial.println(" CRC: ...");
        } else {
            resp_mgr_->sendError((uint8_t)id, resp.error_code);
            incTx(0xFE, 2);
            Serial.printf("[COMM TX] ID: 0xFE LEN: 2 PAYLOAD: %02X %02X CRC: ...\n", id, resp.error_code);
        }
    }
    return resp.success;
}

void CommunicationTask::processUartMessages() {
    if (!uart_driver_ || !sys_mgr_) return;
    while (uart_driver_->available() > 0) {
        uint8_t byte;
        // Read one byte at a time for state machine
        uint8_t buf[1];
        int n = uart_driver_->read(buf, 1);
        if (n <= 0) break;
        byte = buf[0];

        switch (parser_state_) {
            case ParserState::WAIT_HEADER:
                if (byte == 0xAA) {
                    parser_state_ = ParserState::WAIT_ID;
                }
                break;
            case ParserState::WAIT_ID:
                parser_id_ = byte;
                parser_state_ = ParserState::WAIT_LEN;
                break;
            case ParserState::WAIT_LEN:
                if (byte > 64) {
                    // Payload too large — reset
                    parser_state_ = ParserState::WAIT_HEADER;
                } else {
                    parser_len_ = byte;
                    parser_payload_idx_ = 0;
                    if (parser_len_ == 0) {
                        parser_state_ = ParserState::WAIT_CRC_L;
                    } else {
                        parser_state_ = ParserState::WAIT_PAYLOAD;
                    }
                }
                break;
            case ParserState::WAIT_PAYLOAD:
                parser_payload_[parser_payload_idx_++] = byte;
                if (parser_payload_idx_ >= parser_len_) {
                    parser_state_ = ParserState::WAIT_CRC_L;
                }
                break;
            case ParserState::WAIT_CRC_L:
                parser_crc_l_ = byte;
                parser_state_ = ParserState::WAIT_CRC_H;
                break;
            case ParserState::WAIT_CRC_H:
                parser_crc_h_ = byte;
                parser_state_ = ParserState::WAIT_FOOTER;
                break;
            case ParserState::WAIT_FOOTER:
                if (byte == 0x55) {
                    // Validate CRC over header+ID+LEN+PAYLOAD
                    uint8_t frame[66];
                    frame[0] = 0xAA;
                    frame[1] = parser_id_;
                    frame[2] = parser_len_;
                    for (uint8_t i = 0; i < parser_len_; i++) frame[3+i] = parser_payload_[i];
                    uint16_t calc = crc16(frame, 3 + parser_len_);
                    uint16_t recv = parser_crc_l_ | (parser_crc_h_ << 8);
                    if (calc == recv) {
                        handleFrame(parser_id_, parser_len_, parser_payload_);
                    } else {
                        // CRC fail — diagnostic
                        lastRxId_ = parser_id_;
                        lastRxLen_ = parser_len_;
                        lastRxCrcOk_ = false;
                        lastRxOk_ = false;
                        lastErrorCode_ = 3;
                        hasRx_ = true;
                        Serial.printf("[COMM RX] ID: 0x%02X LEN: %u CRC: FAIL (calc %04X != recv %04X)\n", parser_id_, parser_len_, calc, recv);
                        if (resp_mgr_) {
                            resp_mgr_->sendError(parser_id_, 3); // CRC error
                            incTx(0xFE, 2);
                            Serial.printf("[COMM TX] ID: 0xFE LEN: 2 PAYLOAD: %02X 03 CRC: ...\n", parser_id_);
                        }
                    }
                }
                // Reset regardless of footer valid/invalid
                parser_state_ = ParserState::WAIT_HEADER;
                break;
        }
    }
}

void CommunicationTask::sendPeriodicUpdates() {
    static uint32_t last_status = 0;
    static uint32_t last_temps = 0;
    static uint32_t last_vehicle = 0;
    uint32_t now = millis();

    if (!sys_mgr_) return;

    // Send at 1000ms status, 500ms temps, 1000ms vehicle — unified frame AA ID LEN PAYLOAD CRC 55
    if (now - last_status > 1000) {
        last_status = now;
        if (resp_mgr_ && uart_driver_ && uart_driver_->isInitialized()) {
            resp_mgr_->sendStatus(sys_mgr_->getSystemState());
            incTx(0x80, 14);
            Serial.printf("[COMM TX] ID: 0x80 LEN: 14 PAYLOAD: status CRC: ...\n");
        }
    }
    if (now - last_temps > 500 && resp_mgr_) {
        last_temps = now;
        if (uart_driver_ && uart_driver_->isInitialized()) {
            resp_mgr_->sendTemperatureData(sys_mgr_->getTemperatureData());
            incTx(0x81, 14);
            Serial.printf("[COMM TX] ID: 0x81 LEN: 14 PAYLOAD: temps CRC: ...\n");
        }
    }
    if (now - last_vehicle > 1000 && resp_mgr_) {
        last_vehicle = now;
        if (uart_driver_ && uart_driver_->isInitialized()) {
            resp_mgr_->sendVehicleData(sys_mgr_->getVehicleData());
            incTx(0x82, 12);
            Serial.printf("[COMM TX] ID: 0x82 LEN: 12 PAYLOAD: vehicle CRC: ...\n");
        }
    }
}

} // namespace rtos
