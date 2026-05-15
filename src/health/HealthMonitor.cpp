// src/health/HealthMonitor.cpp
#include "HealthMonitor.h"
#include "../config/Config.h"
#include "../config/ConfigManager.h"

void HealthMonitor::report(SensorID id, bool ok, const char* context_reason) {
  int idx = (int)id;
  bool was_usable = usable(id);
  
  if(ok) {
    _failures[idx] = 0;
    if (!was_usable) Serial.printf("[HEALTH] Sensor %d RECOVERED (%s)\n", idx, context_reason ? context_reason : "none");
  } else {
    if (_failures[idx] < 255) _failures[idx]++;
    if (was_usable && !usable(id)) {
      Serial.printf("[HEALTH] Sensor %d FAILED (%s)\n", idx, context_reason ? context_reason : "none");
    }
  }
}

bool HealthMonitor::usable(SensorID id) const {
  return _failures[(int)id] < ConfigManager::instance().cfg.sensor_fail_threshold;
}

int HealthMonitor::systemScore() const {
  int score = 0;
  if (usable(SensorID::GM861))     score += SCORE_GM861;
  if (usable(SensorID::KEYPAD))    score += SCORE_KEYPAD;
  if (usable(SensorID::HX711))     score += SCORE_HX711;
  if (usable(SensorID::MMWAVE))    score += SCORE_MMWAVE;
  if (usable(SensorID::INA219_P1)) score += SCORE_INA219_P1;
  if (usable(SensorID::INA219_P2)) score += SCORE_INA219_P2;
  return score;
}
