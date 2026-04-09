// src/sensors/MmWave.h
#pragma once

class MmWave {
public:
  static MmWave& instance() { static MmWave i; return i; }
  bool init();
  bool personPresent();
private:
  MmWave() = default;
};
