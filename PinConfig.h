#pragma once

// ============================================================
// Hardware Pin Configuration - ESP32-S3 N16R8 CH343P
// ============================================================
// SINGLE SOURCE OF TRUTH for all GPIO assignments
// Do not hardcode GPIO numbers in driver/service/application code
// ============================================================

// ------------------------------------------------------------
// Raspberry Pi 4 UART (Application UART)
// ------------------------------------------------------------
#define PIN_PI_UART_TX      17  // ESP32 TX -> Raspberry Pi RX
#define PIN_PI_UART_RX      18  // ESP32 RX <- Raspberry Pi TX

// ------------------------------------------------------------
// OLED I2C (SSD1306 128x64)
// ------------------------------------------------------------
#define PIN_OLED_SDA        8
#define PIN_OLED_SCL        9
#define OLED_I2C_ADDR       0x3C
#define OLED_WIDTH          128
#define OLED_HEIGHT         64
#define OLED_RESET_PIN      -1

// ------------------------------------------------------------
// Application Outputs (Relay/FET)
// ------------------------------------------------------------
// Fan topology:
//   - Fan Relay (GPIO5) cấp/ngắt nguồn quạt (ON/OFF power)
//   - Fan FET (GPIO7) PWM điều chỉnh tốc độ quạt
//   - Hai chức năng ĐỘC LẬP, không dùng chung chân
// Fan FET polarity TBD - HARDWARE CONFIRMATION REQUIRED before H06
//   Current software assumes active-HIGH: duty 0 = OFF, 255 = MAX (LEDC default)
//   Arduino Mega test (FET/codeemfet.ino) showed active-LOW (255=OFF, 0=MAX)
//   but that test is NOT authoritative for ESP32-S3 hardware.
//   DO NOT invert duty until oscilloscope/DMM verifies FET gate behavior on GPIO7.
//   If hardware is active-LOW, invert at PwmDriver or FanController (single point).
//   See FINALIZE_PRODUCTION_BEFORE_HARDWARE.md ISSUE B.
#define PIN_AC_RELAY        4
#define PIN_FAN_RELAY       5
#define PIN_PI_POWER_RELAY  6
#define PIN_FAN_FET_PWM     7

// ------------------------------------------------------------
// ON/OFF Input
// ------------------------------------------------------------
#define PIN_ON_OFF_INPUT    10
// Input mode TBD - HARDWARE CONFIRMATION REQUIRED before H04
// Options: INPUT (external pull required), INPUT_PULLUP, INPUT_PULLDOWN
// Active polarity TBD: active HIGH vs active LOW not determined by software
// Production code MUST NOT assume floating input is safe; external pull or
// explicit mode must be confirmed via schematic/wiring measurement.
// No production driver currently configures GPIO10; T04 uses INPUT_PULLUP
// as placeholder only (TBD). Do not hardcode mode here until hardware verified.
// See FINALIZE_PRODUCTION_BEFORE_HARDWARE.md ISSUE A.

// ------------------------------------------------------------
// Motor / DRV8833 (Parallel Mode)
// GPIO13 = IN1 (AIN1 + BIN1)
// GPIO14 = IN2 (AIN2 + BIN2)
// No ENA pin in parallel mode
// nSLEEP = hardware-controlled / pending electrical verification
// ------------------------------------------------------------
#define PIN_MOTOR_IN1       13  // DRV8833 IN1 (AIN1 + BIN1)
#define PIN_MOTOR_IN2       14  // DRV8833 IN2 (AIN2 + BIN2)

// ------------------------------------------------------------
// NTC ADC
// ------------------------------------------------------------
#define PIN_NTC1_ADC        1
#define PIN_NTC2_ADC        2

// ------------------------------------------------------------
// CAN Module UART (UART-to-CAN module)
// ------------------------------------------------------------
#define PIN_CAN_UART_TX     11  // ESP32 TX -> CAN module RX
#define PIN_CAN_UART_RX     12  // ESP32 RX <- CAN module TX
// CAN UART baud rate configurable (not equal to CAN bus bitrate)

// ------------------------------------------------------------
// CH343P USB-UART Bridge (RESERVED)
// ------------------------------------------------------------
// GPIO43/44 are used by CH343P for upload/debug
// DO NOT USE for application peripherals
#define PIN_CH343P_TX       43
#define PIN_CH343P_RX       44

// ------------------------------------------------------------
// ESP32-S3 RESERVED PINS (DO NOT USE for application)
// ------------------------------------------------------------
// GPIO0, GPIO3, GPIO45, GPIO46 : strapping pins
// GPIO43/44                     : CH343P USB-UART (debug/upload)
// NOTE: GPIO19/20 are USB-OTG D-/D+ on generic S3, but on this
//       N16R8 CH343P board USB is via CH343P (43/44), so 19/20
//       are re-purposed for encoder by user decision (A=19, B=20).
//       PRODUCTION/HARDWARE CONFLICT: 19/20 overlap USB-OTG if
//       USB-OTG is cabled - user confirmed assignment anyway.

// ------------------------------------------------------------
// Encoder GA25 - Quadrature (user decision 2026-08)
// ------------------------------------------------------------
#define PIN_ENC_A           19  // GA25 Encoder A (quadrature)
#define PIN_ENC_B           20  // GA25 Encoder B (quadrature)
// PIN_ENC_BTN not used - GA25 has no push button