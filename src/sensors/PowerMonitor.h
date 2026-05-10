// src/sensors/PowerMonitor.h
#pragma once
#include <stdint.h>
#include <Adafruit_INA219.h>

class PowerMonitor {
public:
  static PowerMonitor& P1() { static PowerMonitor i; return i; }
  static PowerMonitor& P2() { static PowerMonitor i; return i; }
  
  bool init(uint8_t i2c_addr);
  bool readWithRecovery(float& out_ma);

private:
  uint8_t _addr = 0x40;
  Adafruit_INA219 _ina;
  PowerMonitor() = default;
};
