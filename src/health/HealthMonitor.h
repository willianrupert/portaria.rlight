// src/health/HealthMonitor.h
#pragma once
#include <stdint.h>

enum class SensorID {
  GM861,
  HX711,
  MMWAVE,
  INA219_P1,
  INA219_P2
};

class HealthMonitor {
public:
  static HealthMonitor& instance() { static HealthMonitor i; return i; }
  
  void report(SensorID id, bool ok, const char* context_reason = nullptr);
  bool usable(SensorID id) const;
  
  template<typename Func>
  void tryRecover(SensorID id, Func recovery_func) {
    if (!usable(id)) {
      bool ok = recovery_func();
      report(id, ok, "recovery");
    }
  }

private:
  HealthMonitor() = default;
  int _failures[(int)SensorID::INA219_P2 + 1] = {0};
};
