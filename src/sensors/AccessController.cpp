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
      _cooldown_until = 0;
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

AccessResult AccessController::validatePassword(const char* password) {
  AccessResult result;

  if (!canAttempt()) {
    result.type = AccessType::DENIED;
    strlcpy(result.label, "RATE_LIMITED", sizeof(result.label));
    return result;
  }

  Preferences p;
  p.begin("access_db", true);
  char master[16] = "";
  // Tenta carregar. Se falhar, usa o default 020304
  p.getString("master_pass", master, sizeof(master));
  if (strlen(master) == 0) strlcpy(master, "020304", sizeof(master));
  p.end();

  // 1. Verificar senha mestre (Morador)
  if (strcmp(password, master) == 0) {
    resetRateLimit();
    result.type = AccessType::RESIDENT;
    strlcpy(result.label, "Morador Master", sizeof(result.label));
    return result;
  }

  // 2. Verificar senhas de reversa (Namespace rev_db)
  // Cada chave em rev_db é um alias (ex: "ML"), valor é a senha.
  // Como simplificação para o firmware, tentaremos buscar pela senha diretamenta na NVS se possível
  // mas o padrão ideal é buscar o alias. Para 6 dígitos, faremos busca por hash/alias via HA futuramente.
  // Por enquanto, apenas Master funciona.
  
  recordFailure();
  result.type = AccessType::NONE;
  return result;
}

bool AccessController::setMasterPassword(const char* pass) {
    if (strlen(pass) < 4) return false;
    Preferences p;
    p.begin("access_db", false);
    size_t ok = p.putString("master_pass", pass);
    p.end();
    return ok > 0;
}

bool AccessController::addReversePassword(const char* alias, const char* pass) {
    Preferences p;
    p.begin("rev_db", false);
    size_t ok = p.putString(alias, pass);
    p.end();
    return ok > 0;
}

bool AccessController::removeReversePassword(const char* alias) {
    Preferences p;
    p.begin("rev_db", false);
    bool ok = p.remove(alias);
    p.end();
    return ok;
}
