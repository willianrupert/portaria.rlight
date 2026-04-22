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

  // Tenta validar uma senha digitada (6 dígitos).
  AccessResult validatePassword(const char* password);

  // Gerenciamento de senhas via comandos do HA
  bool setMasterPassword(const char* pass);
  bool addReversePassword(const char* alias, const char* pass);
  bool removeReversePassword(const char* alias);

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
