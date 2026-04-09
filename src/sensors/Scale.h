// src/sensors/Scale.h
#pragma once

class Scale {
public:
  static Scale& instance();
  bool  init();
  bool  initRaw();            // boot sem tare — detecta pacote residual
  long  readRaw();
  float rawToGrams(long raw);
  void  tare();               // RAM only — NUNCA toca NVS
  void  calibrate(float known_g); // NVS — apenas ação humana explícita
  float readOne();
  bool  isReady();
  void  autoZeroTick(bool idle_safe); // chamado pelo Core 1
};
