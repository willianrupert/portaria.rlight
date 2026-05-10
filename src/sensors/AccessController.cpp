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

  char key[16];
  snprintf(key, sizeof(key), "c_%s", code);

  Preferences p;
  p.begin("access_db", true);
  String val = p.getString(key, "");
  p.end();

  if (val.length() == 0) {
    recordFailure();
    result.type = AccessType::NONE;
    return result;
  }

  resetRateLimit();
  // Formato no NVS: "T:Label" (T: 1=Res, 2=ML, 3=Amazon...)
  char type_code = val[0];
  strlcpy(result.label, val.c_str() + 2, sizeof(result.label));

  if (type_code == '1') result.type = AccessType::RESIDENT;
  else if (type_code == '2') result.type = AccessType::REVERSE_ML;
  else if (type_code == '3') result.type = AccessType::REVERSE_AMAZON;
  else if (type_code == '4') result.type = AccessType::REVERSE_CORREIOS;
  else result.type = AccessType::REVERSE_GENERIC;

  return result;
}

bool AccessController::addCode(const char* code, const char* type_label) {
  char key[16];
  snprintf(key, sizeof(key), "c_%s", code);
  
  Preferences p;
  p.begin("access_db", false);
  p.putString(key, type_label);
  
  // Atualiza índice CSV
  String index = p.getString("index", "");
  if (index.indexOf(code) == -1) {
    if (index.length() > 0) index += ",";
    index += code;
    p.putString("index", index);
  }
  p.end();
  return true;
}

bool AccessController::removeCode(const char* code) {
  char key[16];
  snprintf(key, sizeof(key), "c_%s", code);
  
  Preferences p;
  p.begin("access_db", false);
  p.remove(key);
  
  // Remove do índice
  String index = p.getString("index", "");
  int pos = index.indexOf(code);
  if (pos != -1) {
    int end = pos + strlen(code);
    if (index[end] == ',') end++;
    else if (pos > 0 && index[pos-1] == ',') pos--;
    index.remove(pos, end - pos);
    p.putString("index", index);
  }
  p.end();
  return true;
}

void AccessController::listCodes(char* out_buf, size_t buf_sz) {
  Preferences p;
  p.begin("access_db", true);
  String index = p.getString("index", "");
  p.end();

  strlcpy(out_buf, "[", buf_sz);
  if (index.length() == 0) {
    strlcat(out_buf, "]", buf_sz);
    return;
  }

  char temp[index.length() + 1];
  strcpy(temp, index.c_str());
  char* token = strtok(temp, ",");
  bool first = true;

  while (token != NULL) {
    char key[16];
    snprintf(key, sizeof(key), "c_%s", token);
    
    p.begin("access_db", true);
    String val = p.getString(key, "");
    p.end();

    if (val.length() > 0) {
      char entry[128];
      snprintf(entry, sizeof(entry), "%s{\"token\":\"%s\",\"type\":\"%c\",\"label\":\"%s\"}",
               first ? "" : ",", token, val[0], val.c_str() + 2);
      if (strlen(out_buf) + strlen(entry) < buf_sz - 2) {
        strlcat(out_buf, entry, buf_sz);
        first = false;
      }
    }
    token = strtok(NULL, ",");
  }
  strlcat(out_buf, "]", buf_sz);
}
