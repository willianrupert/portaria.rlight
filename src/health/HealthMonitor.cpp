// src/health/HealthMonitor.cpp
#include "HealthMonitor.h"
#include "../config/ConfigManager.h"

void HealthMonitor::report(SensorID id, bool ok, const char* context_reason) {
  int idx = (int)id;
  if(ok) {
    _failures[idx] = 0;
  } else {
    if (_failures[idx] < 255) _failures[idx]++;
  }
}

bool HealthMonitor::usable(SensorID id) const {
  return _failures[(int)id] < ConfigManager::instance().cfg.sensor_fail_threshold;
}
