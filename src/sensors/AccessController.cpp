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
  if (!canAttempt()) {
    result.type = AccessType::DENIED;
    strlcpy(result.label, "RATE_LIMITED", sizeof(result.label));
    return result;
  }

  char key[20];
  snprintf(key, sizeof(key), "c_%s", code);

  char val[48] = "";
  Preferences p;
  p.begin("access_db", true);
  p.getString(key, val, sizeof(val));
  p.end();

  if (val[0] == '\0') {
    recordFailure();
    result.type = AccessType::NONE;
    return result;
  }

  resetRateLimit();
  char type_code = val[0];
  // Formato: "T:Label"
  const char* label_start = (strlen(val) > 2) ? val + 2 : "";
  strlcpy(result.label, label_start, sizeof(result.label));

  if      (type_code == '1') result.type = AccessType::RESIDENT;
  else if (type_code == '2') result.type = AccessType::REVERSE_ML;
  else if (type_code == '3') result.type = AccessType::REVERSE_AMAZON;
  else if (type_code == '4') result.type = AccessType::REVERSE_CORREIOS;
  else                       result.type = AccessType::REVERSE_GENERIC;

  return result;
}

bool AccessController::addCode(const char* code, const char* type_label) {
  char key[20];
  snprintf(key, sizeof(key), "c_%s", code);

  Preferences p;
  p.begin("access_db", false);
  p.putString(key, type_label);

  char index_buf[512] = "";
  p.getString("index", index_buf, sizeof(index_buf));

  // Verifica se o código já está no índice (busca manual)
  bool found = false;
  char* ptr = strstr(index_buf, code);
  if (ptr) {
    char before = (ptr == index_buf) ? '\0' : *(ptr - 1);
    char after  = ptr[strlen(code)];
    if ((before == '\0' || before == ',') && (after == '\0' || after == ','))
      found = true;
  }

  if (!found) {
    size_t idx_len = strlen(index_buf);
    if (idx_len > 0 && idx_len < sizeof(index_buf) - strlen(code) - 2) {
      strncat(index_buf, ",", sizeof(index_buf) - idx_len - 1);
      strncat(index_buf, code, sizeof(index_buf) - idx_len - 2);
    } else if (idx_len == 0) {
      strlcpy(index_buf, code, sizeof(index_buf));
    }
    p.putString("index", index_buf);
  }
  p.end();
  return true;
}

bool AccessController::removeCode(const char* code) {
  char key[20];
  snprintf(key, sizeof(key), "c_%s", code);

  Preferences p;
  p.begin("access_db", false);
  p.remove(key);

  char index_buf[512] = "";
  p.getString("index", index_buf, sizeof(index_buf));

  char result_buf[512] = "";
  char tmp[512];
  strlcpy(tmp, index_buf, sizeof(tmp));

  char* tok = strtok(tmp, ",");
  bool first = true;
  while (tok) {
    if (strcmp(tok, code) != 0) {
      if (!first) strncat(result_buf, ",", sizeof(result_buf) - strlen(result_buf) - 1);
      strncat(result_buf, tok, sizeof(result_buf) - strlen(result_buf) - 1);
      first = false;
    }
    tok = strtok(nullptr, ",");
  }

  p.putString("index", result_buf);
  p.end();
  return true;
}

void AccessController::listCodes(char* out_buf, size_t buf_sz) {
  char index_buf[512] = "";
  {
    Preferences p;
    p.begin("access_db", true);
    p.getString("index", index_buf, sizeof(index_buf));
    p.end();
  }

  strlcpy(out_buf, "[", buf_sz);

  if (index_buf[0] == '\0') {
    strlcat(out_buf, "]", buf_sz);
    return;
  }

  char tmp[512];
  strlcpy(tmp, index_buf, sizeof(tmp));
  char* tok = strtok(tmp, ",");
  bool first = true;

  while (tok) {
    char key[20];
    snprintf(key, sizeof(key), "c_%s", tok);

    char val[48] = "";
    {
      Preferences p;
      p.begin("access_db", true);
      p.getString(key, val, sizeof(val));
      p.end();
    }

    if (val[0] != '\0') {
      char entry[128];
      const char* lbl = (strlen(val) > 2) ? val + 2 : "";
      snprintf(entry, sizeof(entry),
        "%s{\"token\":\"%s\",\"type\":\"%c\",\"label\":\"%s\"}",
        first ? "" : ",", tok, val[0], lbl);
      if (strlen(out_buf) + strlen(entry) < buf_sz - 2) {
        strlcat(out_buf, entry, buf_sz);
        first = false;
      }
    }
    tok = strtok(nullptr, ",");
  }
  strlcat(out_buf, "]", buf_sz);
}
