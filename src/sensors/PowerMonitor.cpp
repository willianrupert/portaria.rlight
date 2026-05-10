// src/sensors/PowerMonitor.cpp
#include "PowerMonitor.h"
#include <Arduino.h>

bool PowerMonitor::init(uint8_t i2c_addr) {
  _addr = i2c_addr;
  _ina = Adafruit_INA219(_addr);
  if (!_ina.begin()) {
    Serial.printf("[PowerMonitor] Falha ao iniciar INA219 em 0x%02X\n", _addr);
    return false;
  }
  // Configuração para maior precisão em 32V / 2A (Strike usa ~400-800mA)
  _ina.setCalibration_32V_2A();
  return true;
}

bool PowerMonitor::readWithRecovery(float& out_ma) {
  try {
    out_ma = _ina.getCurrent_mA();
    return true;
  } catch (...) {
    out_ma = 0.0f;
    return false;
  }
}
