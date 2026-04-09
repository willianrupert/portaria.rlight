// src/actuators/Led.h
#pragma once
#include <stdint.h>

class Led {
public:
  static Led& btn() { static Led l(0); return l; }
  static Led& qr()  { static Led l(1); return l; }

  void init(uint8_t pin, uint8_t channel);
  void off();
  void solid(uint8_t brightness);
  void blink(uint32_t period_ms);
  void breathe(uint32_t period_ms);
  
  void tick(); // Tick assíncrono

private:
  Led(uint8_t id) : _id(id) {}
  uint8_t _id;
  uint8_t _pin = 0;
  uint8_t _channel = 0;
  
  enum Mode { OFF, SOLID, BLINK, BREATHE } _mode = OFF;
  uint32_t _period = 0;
  uint8_t  _brightness = 0;
};
