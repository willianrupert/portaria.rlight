// src/actuators/Strike.cpp
#include "Strike.h"
#include <Arduino.h>
#include "../config/Config.h"

Strike::Strike(int door) : _door(door) {}

void Strike::open(uint32_t ms) {
  _duration = ms;
  _opened_at = millis();
  _is_open = true;
  
  if (_door == 1) {
    digitalWrite(PIN_STRIKE_P1, HIGH);
  } else if (_door == 2) {
    digitalWrite(PIN_STRIKE_P2, HIGH);
  }
}

void Strike::testPulse() {
  open(100);
}

void Strike::tick(float ina_ma) {
  if (!_is_open) return;
  
  // Tick assíncrono para não travar a FSM — CORREÇÃO #2
  if (millis() - _opened_at >= _duration) {
    _is_open = false;
    _opened_at = 0;
    
    if (_door == 1) {
      digitalWrite(PIN_STRIKE_P1, LOW);
    } else if (_door == 2) {
      digitalWrite(PIN_STRIKE_P2, LOW);
    }
  }
}
