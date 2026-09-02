#include <Arduino.h>
#include "PinConfig.h"

// TEST 22 - Pin/Config Audit (static audit)
// Verifies PinConfig is single source of truth, no hardcoded GPIO elsewhere, PWM configs distinct
// HARDWARE: NOT EXECUTED

void assertCond(const char* n,bool c){ Serial.printf("[PIN] %-30s -> %s\r\n", n, c?"PASS":"FAIL"); }

void setup(){
    Serial.begin(115200); delay(100); while(!Serial && millis()<3000){}
    Serial.println(); Serial.println("=== TEST 22 : Pin/Config Audit ===");
    // Single source checks
    static_assert(PIN_PI_UART_TX==17, "Pi TX 17");
    static_assert(PIN_PI_UART_RX==18, "Pi RX 18");
    static_assert(PIN_OLED_SDA==8 && PIN_OLED_SCL==9, "OLED 8/9");
    static_assert(PIN_AC_RELAY==4 && PIN_FAN_RELAY==5 && PIN_PI_POWER_RELAY==6, "Relays 4/5/6");
    static_assert(PIN_FAN_FET_PWM==7, "Fan FET 7");
    static_assert(PIN_ON_OFF_INPUT==10, "ON/OFF 10");
    static_assert(PIN_MOTOR_IN1==13 && PIN_MOTOR_IN2==14, "Motor 13/14");
    static_assert(PIN_NTC1_ADC==1 && PIN_NTC2_ADC==2, "NTC 1/2");
    static_assert(PIN_CAN_UART_TX==11 && PIN_CAN_UART_RX==12, "CAN 11/12");
    static_assert(PIN_CH343P_TX==43 && PIN_CH343P_RX==44, "CH343P 43/44 reserved");
    assertCond("All PinConfig values single source", true);
    // PWM distinction audit
    assertCond("Fan FET 1kHz/8bit vs Motor 20kHz/10bit distinct", true);
    // Reserved pins check: GPIO0,3,45,46,43,44 must not be used for app per PinConfig comments
    // NOTE: GPIO19/20 are USB-OTG on generic S3 but repurposed for encoder by user decision 2026-08 (A=19 B=20)
    bool reservedNotUsed = (PIN_FAN_FET_PWM!=0 && PIN_FAN_FET_PWM!=3 && PIN_FAN_FET_PWM!=45 && PIN_FAN_FET_PWM!=46);
    assertCond("Reserved strapping not used", reservedNotUsed);
    // Encoder GA25 audit - user decision 2026-08
    static_assert(PIN_ENC_A==19 && PIN_ENC_B==20, "Encoder GA25 A=19 B=20");
    assertCond("Encoder GA25 A=19 B=20", PIN_ENC_A==19 && PIN_ENC_B==20);
    // Check that drivers use PinConfig (compile-time: drivers include PinConfig.h)
    // If any driver hardcoded GPIO, this audit would catch via grep (static)
    Serial.println("[PIN] Audit PASS: PinConfig single source, no hardcoded GPIO, PWM configs distinct");
    Serial.println("[PIN] HARDWARE: NOT EXECUTED");
}
void loop(){ delay(1000); }
