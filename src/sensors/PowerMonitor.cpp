// src/sensors/PowerMonitor.cpp
#include "PowerMonitor.h"
#include <Arduino.h>

bool PowerMonitor::init(uint8_t i2c_addr) {
  _addr = i2c_addr;
  return true;
}

bool PowerMonitor::readWithRecovery(float& out_ma) {
  // Stub leitura I2C INA219
  out_ma = 500.0f; // Simula corrente normal
  return true;
}
