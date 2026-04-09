// src/sensors/QRReader.h
#pragma once
#include <Arduino.h>
#include "../config/Config.h"

class QRReader {
public:
  static QRReader& instance();
  bool init();

  // Não-bloqueante: lê chars disponíveis, retorna código quando completo
  const char* poll();

  static void identifyCarrier(const char* code, char* out, size_t sz);
  static bool isAllowedCarrier(const char* carrier);

private:
  char     _rx_buf[QR_RX_BUF_SIZE];
  uint8_t  _rx_pos  = 0;
  char     _result[64];
  bool     _has_result = false;
};
