#include "SystemManager.h"
#include "SystemConfig.h"
#include "PinConfig.h"
#include <Arduino.h>

namespace application {

bool SystemManager::begin() {
    system_state_.mode = model::SystemMode::INITIALIZING;

    // Hardware baseline log
    LOG_INFO("MAIN", "=== ViosAssistant Hardware Baseline ===");
    LOG_INFO("MAIN", "MCU: ESP32-S3");
    LOG_INFO("MAIN", "Hardware: ESP32-S3 N16R8 CH343P");
    LOG_INFO("MAIN", "");
    LOG_INFO("MAIN", "PI UART:");
    LOG_INFO("MAIN", "  TX = GPIO%d", PIN_PI_UART_TX);
    LOG_INFO("MAIN", "  RX = GPIO%d", PIN_PI_UART_RX);
    LOG_INFO("MAIN", "");
    LOG_INFO("MAIN", "OLED I2C:");
    LOG_INFO("MAIN", "  SDA = GPIO%d", PIN_OLED_SDA);
    LOG_INFO("MAIN", "  SCL = GPIO%d", PIN_OLED_SCL);
    LOG_INFO("MAIN", "");
    LOG_INFO("MAIN", "OUTPUT:");
    LOG_INFO("MAIN", "  AC RELAY        = GPIO%d", PIN_AC_RELAY);
    LOG_INFO("MAIN", "  FAN RELAY       = GPIO%d", PIN_FAN_RELAY);
    LOG_INFO("MAIN", "  PI POWER RELAY  = GPIO%d", PIN_PI_POWER_RELAY);
    LOG_INFO("MAIN", "  FAN FET PWM     = GPIO%d", PIN_FAN_FET_PWM);
    LOG_INFO("MAIN", "");
    LOG_INFO("MAIN", "ON/OFF INPUT:");
    LOG_INFO("MAIN", "  GPIO%d", PIN_ON_OFF_INPUT);
    LOG_INFO("MAIN", "");
    LOG_INFO("MAIN", "MOTOR / DRV8833:");
    LOG_INFO("MAIN", "  IN1 = GPIO%d", PIN_MOTOR_IN1);
    LOG_INFO("MAIN", "  IN2 = GPIO%d", PIN_MOTOR_IN2);
    LOG_INFO("MAIN", "  MODE = PARALLEL");
    LOG_INFO("MAIN", "  nSLEEP = HARDWARE CONTROLLED / PENDING VERIFICATION");
    LOG_INFO("MAIN", "");
    LOG_INFO("MAIN", "NTC ADC:");
    LOG_INFO("MAIN", "  GPIO%d", PIN_NTC1_ADC);
    LOG_INFO("MAIN", "  GPIO%d", PIN_NTC2_ADC);
    LOG_INFO("MAIN", "");
    LOG_INFO("MAIN", "CAN UART:");
    LOG_INFO("MAIN", "  TX = GPIO%d", PIN_CAN_UART_TX);
    LOG_INFO("MAIN", "  RX = GPIO%d", PIN_CAN_UART_RX);
    LOG_INFO("MAIN", "");
    LOG_INFO("MAIN", "CH343P:");
    LOG_INFO("MAIN", "  GPIO%d/%d RESERVED", PIN_CH343P_TX, PIN_CH343P_RX);
    LOG_INFO("MAIN", "");
    LOG_INFO("MAIN", "ENCODER GA25 quadrature:");
    LOG_INFO("MAIN", "  A = GPIO%d (PIN_ENC_A)", PIN_ENC_A);
    LOG_INFO("MAIN", "  B = GPIO%d (PIN_ENC_B)", PIN_ENC_B);
    LOG_INFO("MAIN", "  BTN = not used (GA25 no button)");
    LOG_INFO("MAIN", "  NOTE: GPIO19/20 = USB-OTG on generic S3, CH343P board uses 43/44 -> conflict accepted");
    LOG_INFO("MAIN", "");
    LOG_INFO("MAIN", "Hardware baseline initialized.");

    if (!cmd_mgr_.begin()) return false;

    services::ClimateController::Config climate_cfg;
    if (!climate_ctrl_.begin(climate_cfg)) return false;

    // Initialize PWM driver for fan FET (speed control)
    drivers::PwmDriver::ChannelConfig fan_pwm_cfg;
    fan_pwm_cfg.pin = PIN_FAN_FET_PWM;
    fan_pwm_cfg.freq = 1000;
    fan_pwm_cfg.resolution = 8;

    if (!fan_pwm_.begin(fan_pwm_cfg)) return false;

    if (!fan_ctrl_.begin(&fan_pwm_, PIN_FAN_FET_PWM)) return false;

    // Initialize output relays (polarity configurable, default active_high=true)
    // Fan relay: on/off fan power. Pi power relay: on/off Raspberry Pi power.
    if (!ac_relay_.begin(PIN_AC_RELAY, true)) return false;
    if (!fan_relay_.begin(PIN_FAN_RELAY, true)) return false;
    if (!pi_power_relay_.begin(PIN_PI_POWER_RELAY, true)) return false;

    // Initialize motor driver (DRV8833 Parallel Mode: IN1=AIN1+BIN1, IN2=AIN2+BIN2)
    // No ENA pin in parallel mode
    // nSLEEP = hardware-controlled / pending electrical verification
    drivers::MotorDriver::Config motor_cfg;
    motor_cfg.in1_pin = PIN_MOTOR_IN1;
    motor_cfg.in2_pin = PIN_MOTOR_IN2;
    motor_cfg.pwm_freq = 20000;
    motor_cfg.pwm_resolution = 10;
    motor_cfg.pwm_channel_in1 = 0;
    motor_cfg.pwm_channel_in2 = 1;
    if (!motor_.begin(motor_cfg)) return false;
    // Start with motor stopped (coast)
    motor_.coast();

    // Encoder GA25 quadrature: A=GPIO19, B=GPIO20 (user decision 2026-08)
    // nSLEEP = NOT USED (hardware-controlled / externally handled)
    if (!motor_pos_ctrl_.begin(&motor_, &encoder_, PIN_ENC_A, PIN_ENC_B, 0)) return false;
    // if (!air_mode_ctrl_.begin(&motor_)) return false;

    // Initialize NTC driver
    drivers::NtcDriver::Config ntc_cfg;
    if (!ntc_.begin(ntc_cfg, PIN_NTC1_ADC)) return false;

    // Initialize OLED
    if (!oled_.begin()) return false;

    // Initialize Pi UART (GPIO17/18)
    if (!uart_.begin(115200, PIN_PI_UART_TX, PIN_PI_UART_RX)) return false;

    // Initialize CAN UART (GPIO11/12) - baud rate configurable
    if (!can_.begin(500000, PIN_CAN_UART_TX, PIN_CAN_UART_RX)) return false;

    if (!vehicle_data_svc_.begin(&can_, &uart_)) return false;

    system_state_.mode = model::SystemMode::NORMAL;
    initialized_ = true;

    LOG_INFO("MAIN", "Hardware baseline initialized.");

    return true;
}

void SystemManager::update() {
    if (!initialized_) return;

    uint32_t now = millis();

    vehicle_data_svc_.update();
    vehicle_data_ = vehicle_data_svc_.getData();

    // Cache raw ADC for OLED debug (non-intrusive)
    ntc1_raw_ = analogRead(PIN_NTC1_ADC);
    ntc2_raw_ = analogRead(PIN_NTC2_ADC);
    ntc1_voltage_ = ntc1_raw_ * 3.3f / 4095.0f;
    ntc2_voltage_ = ntc2_raw_ * 3.3f / 4095.0f;
    {
        float tInside = ntc_.readTemperatureC(PIN_NTC1_ADC);
        float tOutside = ntc_.readTemperatureC(PIN_NTC2_ADC);
        temp_data_.inside_temp_c = tInside;
        temp_data_.outside_temp_c = tOutside;
        // Valid if: NTC driver initialized, raw not floating (not 0/4095), voltage in plausible range, temperature in SystemConfig range
        // KHONG hardcode true — chi true khi sensor hop le
        bool insideOk = ntc_.isInitialized() && ntc1_raw_ > 10 && ntc1_raw_ < 4085 && ntc1_voltage_ > 0.05f && ntc1_voltage_ < 3.25f && tInside > SystemConfig::TEMP_MIN_C && tInside < SystemConfig::TEMP_MAX_C;
        bool outsideOk = ntc_.isInitialized() && ntc2_raw_ > 10 && ntc2_raw_ < 4085 && ntc2_voltage_ > 0.05f && ntc2_voltage_ < 3.25f && tOutside > SystemConfig::TEMP_MIN_C && tOutside < SystemConfig::TEMP_MAX_C;
        // Additional guard: 0.0f from resistanceToTemp indicates open/short (voltage==0 or resistance<=0)
        if (tInside == 0.0f && ntc1_voltage_ <= 0.05f) insideOk = false;
        if (tOutside == 0.0f && ntc2_voltage_ <= 0.05f) outsideOk = false;
        temp_data_.inside_valid = insideOk;
        temp_data_.outside_valid = outsideOk;
    }
    // PIN_NTC_EVAP and PIN_NTC_AMB not defined in new pin map - commented out
    // temp_data_.evaporator_temp_c = ntc_.readTemperatureC(PIN_NTC_EVAP);
    // temp_data_.ambient_temp_c = ntc_.readTemperatureC(PIN_NTC_AMB);
    temp_data_.last_update_ms = now;

    climate_ctrl_.update(temp_data_, system_state_);
    // Auto fan hysteresis: climate decision overrides fan speed each cycle (user decision 2026-08)
    // Manual SET_FAN_SPEED is transient - auto re-applies on next update()
    fan_ctrl_.setSpeed(climate_ctrl_.getFanOn() ? SystemConfig::FAN_SPEED_MAX : SystemConfig::FAN_SPEED_MIN);
    fan_ctrl_.update();
    // air_mode_ctrl_.update(climate_ctrl_.getAirMode());
    // motor_pos_ctrl_.update();

    ac_relay_.set(climate_ctrl_.getAC());
    // fan_relay_ / pi_power_relay_ control implemented in a later build

    if (now - last_watchdog_ms_ > SystemConfig::WATCHDOG_TIMEOUT_MS) {
        system_state_.watchdog_ok = false;
    }

    system_state_.uptime_ms = now;
    system_state_.free_heap = ESP.getFreeHeap();

    last_update_ms_ = now;
}

void SystemManager::handleCommand(const model::Command& cmd, model::CommandResponse& response) {
    cmd_mgr_.processCommand(cmd, response);
    // processCommand already sets response for VALID/INVALID/NOT_IMPLEMENTED.
    // Only perform actuation for SUCCESS cases; preserve error_code for others.
    if (!response.success) {
        // Keep error_code from CommandManager (1=INVALID, 2=NOT_IMPLEMENTED)
        // but ensure default INVALID for truly unknown types not covered above
        if (response.error_code == 0) {
            response.error_code = 1;
        }
        return;
    }
    switch (cmd.type) {
        case model::CommandType::SET_TEMPERATURE:
            if (cmd.payload_len >= 2) {
                float temp = (cmd.payload[0] + cmd.payload[1] * 0.1f);
                climate_ctrl_.setTemperature(temp);
            } else if (cmd.payload_len == 1) {
                // Single byte absolute 23-30 from Jetson (FROZEN 23-30)
                float temp = (float)cmd.payload[0];
                climate_ctrl_.setTemperature(temp);
            }
            break;
        case model::CommandType::SET_FAN_SPEED:
            if (cmd.payload_len == 0) {
                // FAN ON (6) restore last level
                fan_ctrl_.fanOn();
            } else if (cmd.payload_len >= 1) {
                uint8_t v = cmd.payload[0];
                if (v == 0) {
                    // FAN OFF (7)
                    fan_ctrl_.fanOff();
                } else if (v <= 5) {
                    // Level 1-5 from Jetson 101-105 (H03, PROPOSED)
                    fan_ctrl_.setLevel(v);
                } else {
                    fan_ctrl_.setSpeed(v);
                }
            }
            break;
        case model::CommandType::SET_AIR_MODE:
            if (cmd.payload_len >= 1) {
                climate_ctrl_.setAirMode(static_cast<services::ClimateController::AirMode>(cmd.payload[0]));
            }
            break;
        case model::CommandType::SET_RECIRCULATION:
            if (cmd.payload_len >= 1) {
                climate_ctrl_.setRecirculation(static_cast<services::ClimateController::RecircMode>(cmd.payload[0]));
            }
            break;
        case model::CommandType::SET_AC:
            if (cmd.payload_len >= 1) {
                climate_ctrl_.setAC(cmd.payload[0] != 0);
            }
            break;
        case model::CommandType::SET_HEATER:
            if (cmd.payload_len >= 1) {
                climate_ctrl_.setHeater(cmd.payload[0] != 0);
            }
            break;
        case model::CommandType::SYSTEM_RESET:
            ESP.restart();
            break;
        default:
            // Should not reach here for SUCCESS; treat as NOT_IMPLEMENTED
            response.success = false;
            response.error_code = 2;
            break;
    }
}

const model::SystemState& SystemManager::getSystemState() const {
    return system_state_;
}

const model::TemperatureData& SystemManager::getTemperatureData() const {
    return temp_data_;
}

const model::VehicleData& SystemManager::getVehicleData() const {
    return vehicle_data_;
}

void SystemManager::setSystemMode(model::SystemMode mode) {
    current_mode_ = mode;
    system_state_.mode = mode;
}

model::SystemMode SystemManager::getSystemMode() const {
    return current_mode_;
}

void SystemManager::triggerWatchdog() {
    last_watchdog_ms_ = millis();
    system_state_.watchdog_ok = true;
}

int32_t SystemManager::getEncoderCount() const {
    return encoder_.isInitialized() ? encoder_.getPosition() : 0;
}

int SystemManager::getGpio10State() const {
    // No production pinMode for GPIO10; read current level (may float if no pull)
    return digitalRead(PIN_ON_OFF_INPUT);
}

} // namespace application
