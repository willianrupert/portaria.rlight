// src/middleware/SharedMemory.h — v6.0
#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "../sensors/AccessController.h"

struct AccessGranted {
  AccessType type       = AccessType::NONE;
  char       label[32]  = "";
};

struct PhysicalState {
  bool          wiegand_granted  = false;
  AccessGranted wiegand_access   = {};
  float    weight_g        = 0.0f;
  char     qr_code[64]     = "";
  char     carrier[24]     = "";
  bool     person_present  = false;
  bool     p1_open         = false;
  bool     p2_open         = false;   // monitoramento apenas
  float    ina_p1_ma       = 0.0f;
  float    ina_p2_ma       = 0.0f;    // P2 current monitor se tiver INA dual
  uint32_t last_updated_ms = 0;
  uint32_t sample_age_ms   = 0;       // calculado no getSnapshot()
};

class SharedMemory {
public:
  static SharedMemory& instance() { static SharedMemory i; return i; }

  void init() {
    _mutex = xSemaphoreCreateMutex();
    configASSERT(_mutex != NULL);
  }

  void update(const PhysicalState& src) {
    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      _state = src;
      _state.last_updated_ms = millis();
      xSemaphoreGive(_mutex);
    }
  }

  PhysicalState getSnapshot() {
    PhysicalState snap = {};
    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      snap = _state;
      xSemaphoreGive(_mutex);
    }
    // Idade calculada FORA do mutex — não bloqueia
    // Subtração uint32_t: imune a overflow de 49,7 dias
    snap.sample_age_ms = millis() - snap.last_updated_ms;
    return snap;
  }

private:
  PhysicalState     _state = {};
  SemaphoreHandle_t _mutex = nullptr;
  SharedMemory() = default;
};
