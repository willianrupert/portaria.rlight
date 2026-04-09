// src/actuators/Buzzer.cpp
#include "Buzzer.h"
#include <Arduino.h>
#include "../config/Config.h"

void Buzzer::beep(int times, int duration_ms) {
  // Chamada bloqueante leve para feedback de UI
  for (int i=0; i<times; i++) {
    digitalWrite(PIN_BUZZER, HIGH);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    digitalWrite(PIN_BUZZER, LOW);
    if(i < times - 1) vTaskDelay(pdMS_TO_TICKS(100)); // gap
  }
}

void Buzzer::melody_success() {
  beep(2, 200);
}

void Buzzer::continuous(bool on) {
  digitalWrite(PIN_BUZZER, on ? HIGH : LOW);
}
