#pragma once

#include <cstdint>
#include "SystemManager.h"
#include "CanDriver.h"
#include "UartDriver.h"
#include "ResponseManager.h"

namespace rtos {

class CommunicationTask {
public:
    static bool begin(application::SystemManager* sys_mgr, drivers::CanDriver* can, drivers::UartDriver* uart);
    static bool begin(application::SystemManager* sys_mgr, drivers::CanDriver* can, drivers::UartDriver* uart, application::ResponseManager* resp_mgr);
    static void end();
    static void taskFunction(void* parameter);
    static bool isRunning() { return running_; }

private:
    static application::SystemManager* sys_mgr_;
    static drivers::CanDriver* can_driver_;
    static drivers::UartDriver* uart_driver_;
    static application::ResponseManager* resp_mgr_;
    static TaskHandle_t task_handle_;
    static bool running_;
    static void processCanMessages();
    static void processUartMessages();
    static void sendPeriodicUpdates();
    // Binary frame parser for Jetson (H03) — 0xAA | ID | LEN | PAYLOAD | CRC_L | CRC_H | 0x55
    enum class ParserState { WAIT_HEADER, WAIT_ID, WAIT_LEN, WAIT_PAYLOAD, WAIT_CRC_L, WAIT_CRC_H, WAIT_FOOTER };
    static ParserState parser_state_;
    static uint8_t parser_id_;
    static uint8_t parser_len_;
    static uint8_t parser_payload_[64];
    static uint8_t parser_payload_idx_;
    static uint8_t parser_crc_l_;
    static uint8_t parser_crc_h_;
    static bool handleFrame(uint8_t id, uint8_t len, const uint8_t* payload);
    static uint16_t crc16(const uint8_t* data, uint16_t len);

    // Communication diagnostic — real counters, no fake data
    static volatile uint32_t rxCount_;
    static volatile uint32_t txCount_;
    static volatile uint8_t lastRxId_;
    static volatile uint8_t lastRxLen_;
    static volatile bool lastRxCrcOk_;
    static volatile bool lastRxOk_;
    static volatile uint8_t lastTxId_;
    static volatile uint8_t lastTxLen_;
    static volatile bool lastTxOk_;
    static volatile uint8_t lastErrorCode_;
    static volatile bool hasRx_;
    static volatile bool hasTx_;

public:
    struct CommDiag {
        uint32_t rxCount;
        uint32_t txCount;
        uint8_t lastRxId;
        uint8_t lastRxLen;
        bool lastRxCrcOk;
        bool lastRxOk;
        uint8_t lastTxId;
        uint8_t lastTxLen;
        bool lastTxOk;
        uint8_t lastError;
        bool hasRx;
        bool hasTx;
    };
    static CommDiag getDiag();
    static void incTx(uint8_t id, uint8_t len);
};

} // namespace rtos
