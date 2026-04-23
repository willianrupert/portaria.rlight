// src/actuators/Strike.cpp
#include "Strike.h"
#include <Arduino.h>
#include <Preferences.h>
#include "../config/Config.h"

Strike::Strike(int door) : _door(door) {}

void Strike::open(uint32_t ms) {
  if (_is_open) return; // S1: Proteção contra duplo acionamento em race condition
  
  _duration = ms;
  _opened_at = millis();
  _is_open = true;
  _cycle_recorded = false;
  
  if (_door == 1) {
    digitalWrite(PIN_STRIKE_P1, HIGH);
  } else if (_door == 2) {
    digitalWrite(PIN_STRIKE_P2, HIGH);
  } else if (_door == 3) {
    digitalWrite(PIN_GATE_MOTOR, HIGH);
  }
}

void Strike::testPulse() {
  open(100);
}

void Strike::tick(float ina_ma) {
  if (!_is_open) return;
  
  // S6: Incrementa contador NVS se identificou corrente de acionamento real > 150mA
  if (!_cycle_recorded && ina_ma > 150.0f) {
    Preferences p;
    char key[16];
    snprintf(key, sizeof(key), "strike_p%d", _door);
    p.begin(key, false);
    uint32_t cycles = p.getUInt("cycles", 0) + 1;
    p.putUInt("cycles", cycles);
    p.end();
    _cycles_cache = cycles;
    _cycle_recorded = true;
  }
  
  // Tick assíncrono para não travar a FSM — CORREÇÃO #2
  if (millis() - _opened_at >= _duration) {
    _is_open = false;
    _opened_at = 0;
    
    if (_door == 1) {
      digitalWrite(PIN_STRIKE_P1, LOW);
    } else if (_door == 2) {
      digitalWrite(PIN_STRIKE_P2, LOW);
    } else if (_door == 3) {
      digitalWrite(PIN_GATE_MOTOR, LOW);
    }
  }
}

uint32_t Strike::getCycles() {
  if (_cycles_cache == 0xFFFFFFFF) {
    Preferences p;
    char key[16];
    snprintf(key, sizeof(key), "strike_p%d", _door);
    p.begin(key, true);
    _cycles_cache = p.getUInt("cycles", 0);
    p.end();
  }
  return _cycles_cache;
}
