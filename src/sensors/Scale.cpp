// src/sensors/Scale.cpp
#include "Scale.h"
#include <HX711.h>
#include <Preferences.h>
#include "../config/Config.h"
#include "../config/ConfigManager.h"
#include "../health/HealthMonitor.h"

static HX711    _hx;
static float    _zero_offset  = 0.0f;   // RAM — nunca NVS
static float    _cal_factor   = 1.0f;   // NVS via calibrate()
static uint32_t _last_autozero = 0;

Scale& Scale::instance() { static Scale i; return i; }

bool Scale::init() {
  _hx.begin(PIN_HX711_DATA, PIN_HX711_CLK);
  bool ok = _hx.is_ready();
  HealthMonitor::instance().report(SensorID::HX711, ok, "init");
  if (!ok) {
    Serial.println("[Scale] Falha ao iniciar HX711 (Verificar fiação)");
    return false;
  }

  // Carrega APENAS o fator de calibração da NVS
  Preferences prefs;
  prefs.begin("scale", true);
  _cal_factor = prefs.getFloat("cal", 1.0f);
  prefs.end();

  _hx.set_scale(_cal_factor);
  return true;
}

bool Scale::initRaw() {
  // Boot: inicializa sem tare — preserva leitura raw para detecção de pacote residual
  _hx.begin(PIN_HX711_DATA, PIN_HX711_CLK);
  bool ok = _hx.is_ready();
  HealthMonitor::instance().report(SensorID::HX711, ok, "initRaw");
  
  Preferences prefs;
  prefs.begin("scale", true);
  _cal_factor = prefs.getFloat("cal", 1.0f);
  prefs.end();
  _hx.set_scale(_cal_factor);
  return ok;
}

long Scale::readRaw() {
  return _hx.read();
}

void Scale::tare() {
  // Opera EXCLUSIVAMENTE na RAM — zero escrita na Flash
  long raw = _hx.read_average(10);
  if (raw != LONG_MIN) _zero_offset = (float)raw;
}

void Scale::calibrate(float known_g) {
  // Única operação que toca NVS — apenas quando ação humana explícita
  long raw = _hx.read_average(10);
  if (raw == LONG_MIN) return;
  float new_cal = ((float)raw - _zero_offset) / known_g;
  if (new_cal == _cal_factor) return;   // sem mudança: zero ciclos Flash
  _cal_factor = new_cal;
  _hx.set_scale(_cal_factor);

  Preferences prefs;
  prefs.begin("scale", false);
  prefs.putFloat("cal", _cal_factor);
  prefs.end();
}

float Scale::readOne() {
  if (!_hx.is_ready()) {
    HealthMonitor::instance().report(SensorID::HX711, false, "read_timeout");
    return -9999.0f;
  }
  long raw = _hx.read();
  HealthMonitor::instance().report(SensorID::HX711, true, "read_ok");
  return ((float)raw - _zero_offset) / _cal_factor;
}

float Scale::rawToGrams(long raw) {
  return ((float)raw - _zero_offset) / _cal_factor;
}

bool Scale::isReady() {
  return _hx.is_ready();
}

void Scale::autoZeroTick(bool idle_safe) {
  // idle_safe: true apenas quando FSM=IDLE e P1+P2 fechadas
  // Garante que o corredor está vazio antes de ajustar o zero
  if (!idle_safe) return;

  auto& cfg = ConfigManager::instance().cfg;
  // Subtração uint32_t: imune a overflow de 49,7 dias
  if (millis() - _last_autozero < cfg.auto_zero_interval_ms) return;
  _last_autozero = millis();

  float current = readOne();
  if (current < -9000.0f) return;   // sensor offline

  // Só ajusta se a deriva for pequena (gradual = drift térmico)
  // Variação > limite = provavelmente tem objeto — não ajusta
  if (fabsf(current) <= cfg.auto_zero_max_drift_g) {
    long raw = _hx.read_average(5);
    if (raw != LONG_MIN) _zero_offset = (float)raw;
    // Zero NVS write — apenas RAM
  }
}
