// src/sensors/PowerMonitor.h
#pragma once
#include <stdint.h>

class PowerMonitor {
public:
  static PowerMonitor& P1() { static PowerMonitor i; return i; }
  static PowerMonitor& P2() { static PowerMonitor i; return i; } // Para suporte a interlock e P2 futuramente
  
  bool init(uint8_t i2c_addr);
  bool readWithRecovery(float& out_ma);

private:
  uint8_t _addr = 0x40;
  PowerMonitor() = default;
};
