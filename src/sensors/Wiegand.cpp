// src/sensors/Wiegand.cpp
#include "Wiegand.h"
#include <stdio.h>

volatile uint32_t Wiegand::_bits          = 0;
volatile uint8_t  Wiegand::_bit_count     = 0;
volatile uint32_t Wiegand::_last_pulse_ms = 0;

Wiegand& Wiegand::instance() { static Wiegand i; return i; }

void Wiegand::init() {
  pinMode(PIN_WIEGAND_D0, INPUT_PULLUP);
  pinMode(PIN_WIEGAND_D1, INPUT_PULLUP);
  attachInterrupt(PIN_WIEGAND_D0, wiegand_isr_d0, FALLING);
  attachInterrupt(PIN_WIEGAND_D1, wiegand_isr_d1, FALLING);
}

// ISR D0: recebe bit 0
void IRAM_ATTR wiegand_isr_d0() {
  Wiegand::_bits         <<= 1;        // shift left, bit 0 implícito
  Wiegand::_bit_count++;
  Wiegand::_last_pulse_ms  = millis();
}

// ISR D1: recebe bit 1
void IRAM_ATTR wiegand_isr_d1() {
  Wiegand::_bits          = (Wiegand::_bits << 1) | 1; // shift + set LSB
  Wiegand::_bit_count++;
  Wiegand::_last_pulse_ms = millis();
}

bool Wiegand::poll() {
  _code_ready = false;
  if (_bit_count == 0) return false;

  // Frame completo: timeout de 25ms sem novo bit
  if (millis() - _last_pulse_ms < FRAME_TIMEOUT_MS) return false;

  // Captura atômica (desabilita ISRs brevemente)
  noInterrupts();
  uint32_t bits      = _bits;
  uint8_t  bit_count = _bit_count;
  _bits      = 0;
  _bit_count = 0;
  interrupts();

  // Wiegand 26: descarta bit de paridade MSB e LSB, pega 24 bits do meio
  if (bit_count == 26) {
    _last_code = (bits >> 1) & 0xFFFFFF;
  } else if (bit_count >= 4) {
    // Formatos alternativos (Wiegand 34, 37) — aceita os bits úteis
    _last_code = bits & 0xFFFFFF;
  } else {
    return false; // frame inválido
  }

  snprintf(_last_code_str, sizeof(_last_code_str), "%lu", (unsigned long)_last_code);
  _code_ready = true;
  return true;
}

uint32_t    Wiegand::lastCode()    const { return _last_code; }
const char* Wiegand::lastCodeStr() const { return _last_code_str; }
uint32_t    Wiegand::millisSinceLastPulse() const {
  return millis() - _last_pulse_ms;
}
