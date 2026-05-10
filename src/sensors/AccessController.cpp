// src/sensors/AccessController.cpp
#include "AccessController.h"
#include <Preferences.h>
#include <stdio.h>
#include <string.h>

#include "mbedtls/sha256.h"
#include "nvs_flash.h"
#include "nvs.h"

AccessController& AccessController::instance() { static AccessController i; return i; }

static void computeHash(const char* input, char* output_hex) {
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    
    const char* salt = "rlight_v8_secure_salt_";
    mbedtls_sha256_update(&ctx, (const unsigned char*)salt, strlen(salt));
    mbedtls_sha256_update(&ctx, (const unsigned char*)input, strlen(input));
    
    unsigned char hash[32];
    mbedtls_sha256_finish(&ctx, hash);
    mbedtls_sha256_free(&ctx);
    
    for (int i = 0; i < 32; i++) {
        sprintf(output_hex + (i * 2), "%02x", hash[i]);
    }
    output_hex[64] = '\0';
}

static void buildKey(const char* code, char* key_out) {
    char hash_hex[65];
    computeHash(code, hash_hex);
    // Limite do NVS Preferences key é 15 chars (+ \0 = 16)
    // Usamos o prefixo k_ e os primeiros 11 chars do hash
    snprintf(key_out, 15, "k_%.11s", hash_hex);
}

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

AccessResult AccessController::validate(const char* code) {
  AccessResult result;

  // Rate limiting: bloqueia se em cooldown
  if (!canAttempt()) {
    result.type = AccessType::DENIED;
    strlcpy(result.label, "RATE_LIMITED", sizeof(result.label));
    return result;
  }

  char key[16];
  buildKey(code, key);

  Preferences p;
  p.begin("access_db", true); // read-only
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

bool AccessController::addCode(const char* code, const char* type_label) {
  char key[16];
  buildKey(code, key);
  Preferences p;
  p.begin("access_db", false);
  size_t written = p.putString(key, type_label);
  p.end();
  return written > 0;
}

bool AccessController::removeCode(const char* code) {
  char key[16];
  buildKey(code, key);
  Preferences p;
  p.begin("access_db", false);
  bool ok = p.remove(key);
  p.end();
  return ok;
}

void AccessController::listCodes(char* out_buf, size_t buf_sz) {
  nvs_iterator_t it = nvs_entry_find("nvs", "access_db", NVS_TYPE_ANY);
  if (it == NULL) {
    strlcpy(out_buf, "[]", buf_sz);
    return;
  }
  
  strlcpy(out_buf, "[", buf_sz);
  bool first = true;
  
  while (it != NULL) {
    nvs_entry_info_t info;
    nvs_entry_info(it, &info);
    
    // Recupera o valor para esta key
    Preferences p;
    p.begin("access_db", true);
    char val[48] = "";
    p.getString(info.key, val, sizeof(val));
    p.end();

    char entry[128];
    snprintf(entry, sizeof(entry), "%s{\"key\":\"%s\",\"val\":\"%s\"}", 
             first ? "" : ",", info.key, val);
    
    if (strlen(out_buf) + strlen(entry) < buf_sz - 2) {
      strlcat(out_buf, entry, buf_sz);
      first = false;
    } else {
      break; // Buffer cheio
    }
    
    it = nvs_entry_next(it);
  }
  strlcat(out_buf, "]", buf_sz);
  nvs_release_iterator(it);
}
