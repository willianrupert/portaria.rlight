// src/actuators/Strike.h
#pragma once
#include <stdint.h>

class Strike {
public:
  static Strike& P1()   { static Strike s(1); return s; }
  static Strike& P2()   { static Strike s(2); return s; }
  static Strike& Gate() { static Strike s(3); return s; }

  void open(uint32_t ms);
  void testPulse();
  void tick(float ina_ma); // Chamado a 100Hz pelo Core 0
  uint32_t getCycles(); // Ciclos acumulados (S6)

private:
  Strike(int door);
  int _door;
  uint32_t _opened_at = 0;
  uint32_t _duration = 0;
  uint32_t _cycles_cache = 0xFFFFFFFF;
  bool _is_open = false;
  bool _cycle_recorded = false;
};
