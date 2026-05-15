#include "PowerMonitor.h"
#include <Arduino.h>
#include "../health/HealthMonitor.h"
#include "../config/Config.h"

bool PowerMonitor::init(uint8_t i2c_addr) {
  _addr = i2c_addr;
  _ina = Adafruit_INA219(_addr);
  
  // S22: Check presence on I2C bus
  Wire.beginTransmission(_addr);
  bool ok = (Wire.endTransmission() == 0);
  
  if (ok && _ina.begin()) {
    _ina.setCalibration_32V_2A();
    HealthMonitor::instance().report(_addr == I2C_ADDR_INA219 ? SensorID::INA219_P1 : SensorID::INA219_P2, true, "init");
    return true;
  }
  
  Serial.printf("[PowerMonitor] Falha ao iniciar INA219 em 0x%02X\n", _addr);
  HealthMonitor::instance().report(_addr == I2C_ADDR_INA219 ? SensorID::INA219_P1 : SensorID::INA219_P2, false, "init_fail");
  return false;
}

bool PowerMonitor::readWithRecovery(float& out_ma) {
  // Check connectivity before reading to avoid hanging on missing hardware
  Wire.beginTransmission(_addr);
  if (Wire.endTransmission() != 0) {
    out_ma = 0.0f;
    HealthMonitor::instance().report(_addr == I2C_ADDR_INA219 ? SensorID::INA219_P1 : SensorID::INA219_P2, false, "i2c_lost");
    return false;
  }
  
  out_ma = _ina.getCurrent_mA();
  HealthMonitor::instance().report(_addr == I2C_ADDR_INA219 ? SensorID::INA219_P1 : SensorID::INA219_P2, true, "read_ok");
  return true;
}
