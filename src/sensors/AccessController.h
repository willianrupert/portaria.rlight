// src/sensors/AccessController.h
#pragma once
#include <Arduino.h>

// Tipos de acesso possíveis
enum class AccessType {
  NONE,              // nenhum acesso ativo
  RESIDENT,          // morador — abre P1 + delay + P2
  REVERSE_CORREIOS,  // coleta reversa Correios — só P1
  REVERSE_ML,        // coleta reversa Mercado Livre — só P1
  REVERSE_AMAZON,    // coleta reversa Amazon — só P1
  REVERSE_GENERIC,   // coleta reversa genérica — só P1
  DENIED             // código reconhecido mas bloqueado/rate-limited
};

struct AccessResult {
  AccessType type    = AccessType::NONE;
  char       label[32] = ""; // ex: "Morador Principal", "ML Reversa"
};

class AccessController {
public:
  static AccessController& instance();

  // Tenta validar um código de senha.
  // Retorna AccessResult com o tipo de acesso.
  AccessResult validate(const char* code);

  // Gerenciamento de códigos via comandos do HA (persistidos na NVS)
  // key format: "key_{code}" → value: "RESIDENT:Willian" ou "REVERSE_ML:ML Coleta"
  bool addCode(const char* code, const char* type_label);   // CMD_ADD_KEY_CODE
  bool removeCode(const char* code);                        // CMD_REMOVE_KEY_CODE
  void listCodes(char* out_buf, size_t buf_sz);             // CMD_LIST_KEY_CODES

  // Rate limiting: retorna true se pode tentar (não está em cooldown)
  bool canAttempt();
  void recordFailure();
  void resetRateLimit();

private:
  AccessController() = default;

  // Rate limiting
  uint8_t  _fail_count      = 0;
  uint32_t _first_fail_ms   = 0;
  uint32_t _cooldown_until  = 0;

  static const uint8_t  MAX_FAILS      = 5;
  static const uint32_t FAIL_WINDOW_MS = 300000; // 5 minutos
  static const uint32_t COOLDOWN_MS    = 600000; // 10 minutos de bloqueio
};
