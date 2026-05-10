// src/fsm/StateMachine.h — v6.0
#pragma once
#include <Arduino.h>
#include "../middleware/SharedMemory.h"

enum class State {
  IDLE,           // standby — LED pulsa
  MAINTENANCE,    // estado FSM próprio (não flag booleana)
  AWAKE,          // botão pressionado
  WAITING_QR,     // aguardando código de barras
  AUTHORIZED,     // strike abriu P1
  INSIDE_WAIT,    // P1 fechou, pessoa dentro
  DELIVERING,     // balança detectou objeto
  DOOR_REOPENED,  // P1 abriu para saída
  CONFIRMING,     // P1 fechou, corredor vazio
  RECEIPT,        // comprovante na tela
  ABORTED,        // entrou mas não entregou
  RESIDENT_P1,    // NOVO: morador P1
  RESIDENT_P2,    // NOVO: morador P2
  REVERSE_PICKUP, // NOVO: coletor reversa
  DOOR_ALERT,     // P1 aberta > timeout
  ERROR,           // falha crítica bloqueante
  WAITING_PASS,   // digitando senha (v8)
  LOCKOUT_KEYPAD, // teclado bloqueado (v8)
  OUT_OF_HOURS    // fora do horário comercial (v8)
};

struct FsmContext {
  // Contexto de acesso de morador
  AccessType resident_access_type = AccessType::NONE;
  char       resident_label[32]   = "";  // "Willian Rupert"
  uint32_t   resident_p1_opened   = 0;   // millis() quando P1 abriu para morador

  // Contexto de reversa
  char       reverse_carrier[24]  = "";  // transportadora identificada
  float      reverse_weight_delta = 0.0f; // variação de peso (negativa = retirada)

  State    state              = State::IDLE;
  uint32_t state_enter        = 0;
  char     qr_code[64]        = "";
  char     carrier[24]        = "";
  float    weight_g           = 0.0f;
  bool     p1_open            = false;
  bool     p2_open            = false;
  bool     person_inside      = false;
  uint32_t unix_time          = 0;
  uint32_t millis_at_sync     = 0;    // para extrapolação de timestamp
  bool     system_time_invalid= false;
  bool     lockdown           = false;
  // Sub-estado CONFIRMING (não-bloqueante)
  uint32_t scale_settle_start = 0;
  // Sub-estado AUTHORIZED (timeout)
  uint32_t authorized_at      = 0;
  // Sub-estado DELIVERING (timeout)
  uint32_t delivering_start   = 0;
  bool     unconventional_exit= false;
  // S2: Fallback mmWave 
  uint32_t door_reopen_at     = 0;
  bool     mmwave_fallback_used= false;
  
  uint32_t resident_p2_timer  = 0;   // início do delay de 2s para P2
  uint8_t  keypad_fails       = 0;   // contador de tentativas falhas (v8)
  uint32_t lockout_until      = 0;   // tempo de desbloqueio (v8)
  uint32_t prev_duration_ms   = 0;   // duração do estado anterior (substitui millis_at_sync em transição)
  uint32_t loiter_start       = 0;   // para controle de timeout em IDLE (S1)
};

class StateMachine {
public:
  static StateMachine& instance();
  static const char* stateToString(State s);
  void tick(const PhysicalState& world);
  void transition(State next);
  State             current() const;
  const FsmContext& ctx()     const;
  FsmContext&       ctx();

private:
  FsmContext _ctx;
  void _onEnter(State s);
  void _onExit(State s);
  void _persistDeliveryContext(); // S3: NVS Context Snapshot
  // Handlers — um por estado
  void _handleIdle(const PhysicalState& w);
  void _handleMaintenance(const PhysicalState& w);
  void _handleAwake(const PhysicalState& w);
  void _handleWaitingQr(const PhysicalState& w);
  void _handleAuthorized(const PhysicalState& w);
  void _handleInsideWait(const PhysicalState& w);
  void _handleDelivering(const PhysicalState& w);
  void _handleDoorReopened(const PhysicalState& w);
  void _handleConfirming(const PhysicalState& w);
  void _handleReceipt(const PhysicalState& w);
  void _handleAborted(const PhysicalState& w);
  void _handleResidentP1(const PhysicalState& w);
  void _handleResidentP2(const PhysicalState& w);
  void _handleReversePickup(const PhysicalState& w);
  void _handleDoorAlert(const PhysicalState& w);
  void _handleWaitingPass(const PhysicalState& w);
  void _handleLockoutKeypad(const PhysicalState& w);
  void _handleOutOfHours(const PhysicalState& w);
};
