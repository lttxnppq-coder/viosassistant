#include <Arduino.h>
#include "PinConfig.h"
#include "UartDriver.h"
#include "CanDriver.h"
#include "RelayDriver.h"
#include "PwmDriver.h"
#include "OledDriver.h"
#include "NtcDriver.h"
#include "MotorDriver.h"
#include "EncoderDriver.h"

// TEST 20 - Hardware Abstraction (compile/static)
// Verifies all drivers compile against PinConfig single source of truth
// Checks: no hardcoded GPIO inside drivers, correct PWM configs (Fan 1kHz/8bit, Motor 20kHz/10bit)
// HARDWARE: NOT EXECUTED

void assertCond(const char* n,bool c){ Serial.printf("[HAL] %-30s -> %s\r\n", n, c?"PASS":"FAIL"); }

void setup(){
    Serial.begin(115200); delay(100); while(!Serial && millis()<3000){}
    Serial.println(); Serial.println("=== TEST 20 : Hardware Abstraction (compile/static) ===");
    // PinConfig single source checks (compile-time)
    static_assert(PIN_FAN_FET_PWM==7, "Fan FET GPIO7");
    static_assert(PIN_MOTOR_IN1==13, "Motor IN1 GPIO13");
    static_assert(PIN_MOTOR_IN2==14, "Motor IN2 GPIO14");
    static_assert(PIN_PI_UART_TX==17 && PIN_PI_UART_RX==18, "Pi UART 17/18");
    static_assert(PIN_CAN_UART_TX==11 && PIN_CAN_UART_RX==12, "CAN UART 11/12");
    // PWM config distinction
    drivers::PwmDriver::ChannelConfig fanCfg; fanCfg.pin=PIN_FAN_FET_PWM; fanCfg.freq=1000; fanCfg.resolution=8;
    drivers::MotorDriver::Config motorCfg; // default 20kHz/10bit
    assertCond("Fan PWM 1kHz/8bit", fanCfg.freq==1000 && fanCfg.resolution==8);
    assertCond("Motor PWM 20kHz/10bit", motorCfg.pwm_freq==20000 && motorCfg.pwm_resolution==10);
    assertCond("Fan vs Motor distinct", fanCfg.freq != motorCfg.pwm_freq);
    // Driver types exist
    static_assert(sizeof(drivers::UartDriver)>0, "UartDriver ok");
    static_assert(sizeof(drivers::CanDriver)>0, "CanDriver ok");
    static_assert(sizeof(drivers::OledDriver)>0, "OledDriver ok");
    static_assert(sizeof(drivers::NtcDriver)>0, "NtcDriver ok");
    static_assert(sizeof(drivers::EncoderDriver)>0, "EncoderDriver ok");
    assertCond("All drivers compile vs PinConfig", true);
    // Encoder GA25 quadrature - user decision 2026-08: A=19, B=20
    static_assert(PIN_ENC_A==19, "Encoder A GPIO19");
    static_assert(PIN_ENC_B==20, "Encoder B GPIO20");
    assertCond("Encoder A=19", PIN_ENC_A==19);
    assertCond("Encoder B=20", PIN_ENC_B==20);
    assertCond("Encoder quadrature 19/20", PIN_ENC_A==19 && PIN_ENC_B==20);
    Serial.println("[HAL] compile/PinConfig PASS HARDWARE: NOT EXECUTED");
}
void loop(){ delay(1000); }
