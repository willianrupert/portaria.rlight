// src/actuators/Buzzer.h
#pragma once
#include <stdint.h>

class Buzzer {
public:
  static void beep(int times, int duration_ms);
  static void melody_success();
  static void continuous(bool on);
};
