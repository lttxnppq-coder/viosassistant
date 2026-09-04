#ifndef VIOSASSISTANT_SYSTEM_MANAGER_H
#define VIOSASSISTANT_SYSTEM_MANAGER_H

#pragma once

#include <cstdint>
#include "SystemState.h"
#include "TemperatureData.h"
#include "VehicleData.h"
#include "CommandManager.h"
#include "ClimateController.h"
#include "FanController.h"
#include "AirModeController.h"
#include "MotorPositionController.h"
#include "VehicleDataService.h"
#include "OledDriver.h"
#include "NtcDriver.h"
#include "PwmDriver.h"
#include "RelayDriver.h"
#include "EncoderDriver.h"
#include "MotorDriver.h"
#include "UartDriver.h"
#include "CanDriver.h"
#include "Logger.h"

namespace application {

class SystemManager {
public:
    bool begin();
    void update();
    void handleCommand(const model::Command& cmd, model::CommandResponse& response);
    const model::SystemState& getSystemState() const;
    const model::TemperatureData& getTemperatureData() const;
    const model::VehicleData& getVehicleData() const;
    void setSystemMode(model::SystemMode mode);
    model::SystemMode getSystemMode() const;
    void triggerWatchdog();
    bool isInitialized() const { return initialized_; }
    drivers::OledDriver& getOledDriver() { return oled_; }
    drivers::UartDriver& getUartDriver() { return uart_; }
    drivers::CanDriver& getCanDriver() { return can_; }
    const drivers::UartDriver& getUartDriver() const { return uart_; }
    const drivers::CanDriver& getCanDriver() const { return can_; }
    // OLED debug getters (non-intrusive, read-only)
    const services::ClimateController& getClimateController() const { return climate_ctrl_; }
    const services::FanController& getFanController() const { return fan_ctrl_; }
    const services::MotorPositionController& getMotorController() const { return motor_pos_ctrl_; }
    int32_t getEncoderCount() const;
    int getGpio10State() const;
    int getNtc1Raw() const { return ntc1_raw_; }
    int getNtc2Raw() const { return ntc2_raw_; }
    float getNtc1Voltage() const { return ntc1_voltage_; }
    float getNtc2Voltage() const { return ntc2_voltage_; }

private:
    bool initialized_ = false;
    model::SystemState system_state_;
    model::TemperatureData temp_data_;
    model::VehicleData vehicle_data_;
    model::SystemMode current_mode_ = model::SystemMode::INITIALIZING;

    CommandManager cmd_mgr_;
    services::ClimateController climate_ctrl_;
    services::FanController fan_ctrl_;
    services::AirModeController air_mode_ctrl_;
    services::MotorPositionController motor_pos_ctrl_;
    services::VehicleDataService vehicle_data_svc_;

    drivers::OledDriver oled_;
    drivers::NtcDriver ntc_;
    drivers::PwmDriver fan_pwm_;
    drivers::RelayDriver ac_relay_;
    drivers::RelayDriver fan_relay_;
    drivers::RelayDriver pi_power_relay_;
    drivers::EncoderDriver encoder_;
    drivers::MotorDriver motor_;
    drivers::UartDriver uart_;
    drivers::CanDriver can_;

    uint32_t last_update_ms_ = 0;
    uint32_t last_oled_ms_ = 0;
    uint32_t last_climate_ms_ = 0;
    uint32_t last_vehicle_ms_ = 0;
    uint32_t last_watchdog_ms_ = 0;
    // Raw sensor cache for OLED debug (non-intrusive)
    int ntc1_raw_ = 0;
    int ntc2_raw_ = 0;
    float ntc1_voltage_ = 0.0f;
    float ntc2_voltage_ = 0.0f;
};

} // namespace application

#endif // VIOSASSISTANT_SYSTEM_MANAGER_H
