// src/sensors/Wiegand.h
#pragma once
#include <Arduino.h>
#include "../config/Config.h"

// Tamanho máximo do buffer de codes recebidos (queue circular)
#define WIEGAND_QUEUE_SIZE 4

class Wiegand {
public:
  static Wiegand& instance();
  void init();  // Configura ISRs nos pinos D0 e D1

  // Não-bloqueante. Retorna true se um código completo foi recebido.
  // O código decodificado fica disponível em lastCode().
  bool poll();

  uint32_t lastCode() const;      // Código Wiegand decodificado (24 bits úteis)
  const char* lastCodeStr() const; // Representação decimal em char[12]

  // Saúde: tempo desde último pulso recebido (para detectar sensor offline/health-checks)
  uint32_t millisSinceLastPulse() const;

private:
  Wiegand() = default;

  // ISR data — volatile pois escrito nas ISRs
  static volatile uint32_t _bits;           // acumula bits recebidos
  static volatile uint8_t  _bit_count;      // quantos bits recebidos até agora
  static volatile uint32_t _last_pulse_ms;  // millis() do último pulso

  uint32_t _last_code        = 0;
  char     _last_code_str[12] = "";
  bool     _code_ready       = false;

  // Timeout: se não chegar o próximo bit em 25ms, o frame está completo ou corrompido
  static const uint32_t FRAME_TIMEOUT_MS = 25;
  
  // Declared friend callbacks para acessar as private volatiles:
  friend void wiegand_isr_d0();
  friend void wiegand_isr_d1();
};

// ISRs globais — precisam ser funções livres para attachInterrupt
void IRAM_ATTR wiegand_isr_d0();
void IRAM_ATTR wiegand_isr_d1();
