// src/actuators/Led.cpp
#include "Led.h"
#include <Arduino.h>

void Led::init(uint8_t pin, uint8_t channel) {
  _pin = pin;
  _channel = channel;
  ledcSetup(_channel, 5000, 8);
  ledcAttachPin(_pin, _channel);
  off();
}

void Led::off() {
  _mode = OFF;
  ledcWrite(_channel, 0);
}

void Led::solid(uint8_t brightness) {
  _mode = SOLID;
  _brightness = brightness;
  ledcWrite(_channel, _brightness);
}

void Led::blink(uint32_t period_ms) {
  _mode = BLINK;
  _period = period_ms;
}

void Led::breathe(uint32_t period_ms) {
  _mode = BREATHE;
  _period = period_ms;
}

void Led::blink_once(uint32_t duration_ms) {
  _blink_until = millis() + duration_ms;
}

void Led::tick() {
  if (millis() < _blink_until) {
    ledcWrite(_channel, 255);
    return;
  }

  if (_mode == BLINK) {
    uint32_t t = millis() % _period;
    ledcWrite(_channel, t < (_period / 2) ? 255 : 0);
  } 
  else if (_mode == BREATHE) {
    float k = 2.0 * PI / (float)_period;
    float val = (exp(sin(millis() * k)) - 0.36787944) * 108.0;
    ledcWrite(_channel, (uint8_t)constrain(val, 0, 255));
  }
}
