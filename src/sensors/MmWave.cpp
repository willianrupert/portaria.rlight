// src/sensors/MmWave.cpp
#include "MmWave.h"
#include <Arduino.h>
#include "../config/Config.h"

bool MmWave::init() {
  Serial2.begin(256000, SERIAL_8N1, PIN_UART1_RX, PIN_UART1_TX);
  return true;
}

bool MmWave::personPresent() {
  // Simulação de leitura de frames do LD2410B
  // Na implementação final isso varreria o buffer em busca da engineering mode flag
  // Retorna false como stub se não implementado
  return false; 
}
