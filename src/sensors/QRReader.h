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
  
  // Controle de hardware via comandos seriais
  void configure();
  void start_scan();
  void stop_scan();

  static void identifyCarrier(const char* code, char* out, size_t sz);
  static bool isAllowedCarrier(const char* carrier);

private:
  char     _rx_buf[QR_RX_BUF_SIZE];
  uint8_t  _rx_pos  = 0;
  char     _result[64];
  bool     _has_result = false;
  
  uint8_t  _failed_reads = 0;
  uint32_t _cooldown_until = 0;
  uint32_t _last_read_time = 0;

  void _send_command(const uint8_t* cmd, size_t len);
};
