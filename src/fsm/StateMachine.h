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
  DOOR_ALERT,     // P1 aberta > timeout
  ERROR           // falha crítica bloqueante
};

struct FsmContext {
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
};

class StateMachine {
public:
  static StateMachine& instance();
  void tick(const PhysicalState& world);
  void transition(State next);
  State             current() const;
  const FsmContext& ctx()     const;
  FsmContext&       ctx();

private:
  FsmContext _ctx;
  void _onEnter(State s);
  void _onExit(State s);
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
  void _handleDoorAlert(const PhysicalState& w);
};
