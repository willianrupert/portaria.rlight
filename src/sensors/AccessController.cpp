// src/sensors/AccessController.cpp
#include "AccessController.h"
#include <Preferences.h>
#include <stdio.h>
#include <string.h>

AccessController& AccessController::instance() { static AccessController i; return i; }

bool AccessController::canAttempt() {
  if (_cooldown_until > 0) {
    if (millis() < _cooldown_until) {
      return false;
    } else {
      _cooldown_until = 0; // Cooldown expirou
      resetRateLimit();
    }
  }
  return true;
}

void AccessController::recordFailure() {
  if (_fail_count == 0 || (millis() - _first_fail_ms > FAIL_WINDOW_MS)) {
    _fail_count = 1;
    _first_fail_ms = millis();
  } else {
    _fail_count++;
    if (_fail_count >= MAX_FAILS) {
      _cooldown_until = millis() + COOLDOWN_MS;
    }
  }
}

void AccessController::resetRateLimit() {
  _fail_count = 0;
  _first_fail_ms = 0;
  _cooldown_until = 0;
}

AccessResult AccessController::validate(uint32_t code) {
  AccessResult result;

  // Rate limiting: bloqueia se em cooldown
  if (!canAttempt()) {
    result.type = AccessType::DENIED;
    strlcpy(result.label, "RATE_LIMITED", sizeof(result.label));
    return result;
  }

  // Constrói chave NVS: "wieg_XXXXXXXX"
  char key[20];
  snprintf(key, sizeof(key), "wieg_%lu", (unsigned long)code);

  Preferences p;
  p.begin("wieg_db", true); // read-only
  char val[48] = "";
  p.getString(key, val, sizeof(val));
  p.end();

  if (strlen(val) == 0) {
    // Código não encontrado
    recordFailure();
    result.type = AccessType::NONE;
    return result;
  }

  // Reset rate limit em acesso bem-sucedido
  resetRateLimit();

  // Parseia o valor: "TIPO:Label"
  // ex: "RESIDENT:Willian Rupert" ou "REVERSE_ML:Coleta ML"
  char type_str[24] = "";
  char label_str[24] = "";
  const char* colon = strchr(val, ':');
  if (colon) {
    size_t type_len = colon - val;
    // O strlen(type_str) max is sizeof(type_str)-1. 
    // Usamos min para evitar overflow se type_len for anormal
    size_t len_to_copy = (type_len < sizeof(type_str) - 1) ? type_len : sizeof(type_str) - 1;
    strncpy(type_str, val, len_to_copy);
    type_str[len_to_copy] = '\0';
    strlcpy(label_str, colon + 1, sizeof(label_str));
  } else {
    strlcpy(type_str, val, sizeof(type_str));
  }
  strlcpy(result.label, label_str, sizeof(result.label));

  if      (!strcmp(type_str, "RESIDENT"))          result.type = AccessType::RESIDENT;
  else if (!strcmp(type_str, "REVERSE_CORREIOS"))  result.type = AccessType::REVERSE_CORREIOS;
  else if (!strcmp(type_str, "REVERSE_ML"))        result.type = AccessType::REVERSE_ML;
  else if (!strcmp(type_str, "REVERSE_AMAZON"))    result.type = AccessType::REVERSE_AMAZON;
  else if (!strcmp(type_str, "REVERSE_GENERIC"))   result.type = AccessType::REVERSE_GENERIC;
  else                                             result.type = AccessType::NONE;

  return result;
}

bool AccessController::addCode(uint32_t code, const char* type_label) {
  char key[20];
  snprintf(key, sizeof(key), "wieg_%lu", (unsigned long)code);
  Preferences p;
  p.begin("wieg_db", false);
  size_t written = p.putString(key, type_label);
  p.end();
  return written > 0;
}

bool AccessController::removeCode(uint32_t code) {
  char key[20];
  snprintf(key, sizeof(key), "wieg_%lu", (unsigned long)code);
  Preferences p;
  p.begin("wieg_db", false);
  bool ok = p.remove(key);
  p.end();
  return ok;
}

void AccessController::listCodes(char* out_buf, size_t buf_sz) {
  // O NVS Preferences da ESP32 não possui método direto "listar keys" nativo na API simplificada
  // da framework Arduino. Precisaríamos usar nvs_entry_find de nvs_flash.h.
  // Como simplificação via string JSON temporária sem lotar a IRAM com blobs:
  strlcpy(out_buf, "{\"error\": \"Function requires nvs_entry_find (lower level ESP-IDF) or specific HA template indexing\"}", buf_sz);
}
