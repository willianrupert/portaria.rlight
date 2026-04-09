// src/fsm/StateMachine.cpp
#include "StateMachine.h"
#include "../config/Config.h"
#include "../config/ConfigManager.h"
#include "../health/HealthMonitor.h"
#include "../actuators/Strike.h"
#include "../actuators/Buzzer.h"
#include "../actuators/Led.h"
#include "../comms/UsbBridge.h"
#include "../crypto/JwtSigner.h"

StateMachine& StateMachine::instance() { static StateMachine i; return i; }
State             StateMachine::current() const { return _ctx.state; }
const FsmContext& StateMachine::ctx()     const { return _ctx; }
FsmContext&       StateMachine::ctx()           { return _ctx; }

void StateMachine::transition(State next) {
  _onExit(_ctx.state);
  _ctx.state       = next;
  _ctx.state_enter = millis();
  _onEnter(next);
  UsbBridge::instance().sendState(next, _ctx);
}

void StateMachine::_onEnter(State s) {
  switch (s) {
    case State::IDLE:        Led::btn().breathe(2000); Led::qr().off(); break;
    case State::MAINTENANCE: Led::btn().blink(1000);   Buzzer::beep(5,100); break;
    case State::AWAKE:       Led::btn().solid(255);    Buzzer::beep(1,100); break;
    case State::WAITING_QR:  Led::qr().solid(200);     Led::btn().blink(300); break;
    case State::AUTHORIZED:  Buzzer::beep(2,150);      break;
    case State::RECEIPT:     Buzzer::melody_success(); Led::btn().breathe(500); break;
    case State::DOOR_ALERT:  Buzzer::continuous(true); break;
    case State::ABORTED:     Buzzer::beep(3,200);      break;
    default: break;
  }
}

void StateMachine::_onExit(State s) {
  if (s == State::DOOR_ALERT) Buzzer::continuous(false);
}

void StateMachine::tick(const PhysicalState& world) {
  auto& cfg = ConfigManager::instance().cfg;

  // MAINTENANCE tem prioridade absoluta
  if (cfg.maintenance_mode && _ctx.state != State::MAINTENANCE) {
    transition(State::MAINTENANCE); return;
  }
  if (!cfg.maintenance_mode && _ctx.state == State::MAINTENANCE) {
    transition(State::IDLE); return;
  }

  // DOOR_ALERT preemptivo: P1 aberta em qualquer estado perigoso
  if (world.p1_open && _ctx.state == State::IDLE) {
    transition(State::DOOR_ALERT); return;
  }

  // Lógica Opcional sugerida: DOOR_ALERT se interlock falhar e P2 estiver aberta
  if (cfg.enable_strike_p2 && world.p2_open && _ctx.state == State::IDLE) {
    // A definir comportamentos avançados pro futuro
  }

  switch (_ctx.state) {
    case State::IDLE:         _handleIdle(world);         break;
    case State::MAINTENANCE:  _handleMaintenance(world);  break;
    case State::AWAKE:        _handleAwake(world);        break;
    case State::WAITING_QR:   _handleWaitingQr(world);    break;
    case State::AUTHORIZED:   _handleAuthorized(world);   break;
    case State::INSIDE_WAIT:  _handleInsideWait(world);   break;
    case State::DELIVERING:   _handleDelivering(world);   break;
    case State::DOOR_REOPENED:_handleDoorReopened(world); break;
    case State::CONFIRMING:   _handleConfirming(world);   break;
    case State::RECEIPT:      _handleReceipt(world);      break;
    case State::ABORTED:      _handleAborted(world);      break;
    case State::DOOR_ALERT:   _handleDoorAlert(world);    break;
    default: break;
  }
}

// ── HANDLERS ────────────────────────────────────────────────────────

void StateMachine::_handleIdle(const PhysicalState& w) {
  // Botão inicia fluxo de entrega
  if (digitalRead(PIN_BUTTON) == LOW) {
    delay(50);   // debounce simples — não é ISR
    if (digitalRead(PIN_BUTTON) == LOW) { transition(State::AWAKE); return; }
  }
  // Loitering: mmWave detecta presença prolongada sem autorização
  if (w.person_present && ConfigManager::instance().cfg.enable_loitering_alarm) {
    static uint32_t loiter_start = 0;
    if (loiter_start == 0) loiter_start = millis();
    if (millis() - loiter_start >= ConfigManager::instance().cfg.loitering_alert_ms) {
      UsbBridge::instance().sendAlert("SUSPICIOUS_LOITERING", _ctx);
      loiter_start = 0;
    }
  } else {
    static uint32_t loiter_start = 0;
    loiter_start = 0;
  }
}

void StateMachine::_handleMaintenance(const PhysicalState& w) {
  // Em MAINTENANCE: sensores atualizam normalmente para o HA
  // FSM não aciona hardware, não gera alertas, não processa QR
  // Saída: HA desativa maintenance_mode via CMD_UPDATE_CFG
  (void)w;
}

void StateMachine::_handleAwake(const PhysicalState& w) {
  (void)w;
  if (millis() - _ctx.state_enter > ConfigManager::instance().cfg.awake_timeout_ms) {
    UsbBridge::instance().sendEvent("DELIVERY_ABANDONED", _ctx);
    transition(State::IDLE);
  }
  // Transição para WAITING_QR ocorre via handshake SCREEN_READY do OPi
  // UsbBridge seta flag que a FSM verifica via UsbBridge::instance().screenReady()
  if (UsbBridge::instance().screenReady()) transition(State::WAITING_QR);
}

void StateMachine::_handleWaitingQr(const PhysicalState& w) {
  auto& cfg = ConfigManager::instance().cfg;
  // Timeout sem leitura
  if (millis() - _ctx.state_enter > cfg.qr_timeout_ms) {
    UsbBridge::instance().sendAlert("QR_TIMEOUT", _ctx);
    transition(State::IDLE); return;
  }
  // QR lido (campo qr_code foi preenchido pelo Core 1 via SharedMemory)
  if (strlen(w.qr_code) > 0 && QRReader::isAllowedCarrier(w.carrier)) {
    strlcpy(_ctx.qr_code, w.qr_code, sizeof(_ctx.qr_code));
    strlcpy(_ctx.carrier, w.carrier, sizeof(_ctx.carrier));
    transition(State::AUTHORIZED);
  }
}

void StateMachine::_handleAuthorized(const PhysicalState& w) {
  auto& cfg = ConfigManager::instance().cfg;

  // Inicia strike na primeira entrada
  if (_ctx.authorized_at == 0) {
    // Interbloqueio P2 opcional: se P2 tiver atuador futuramente
    if (cfg.enable_strike_p2 && w.p2_open) {
      UsbBridge::instance().sendAlert("INTERLOCK_P2_OPEN", _ctx);
      transition(State::ERROR); return;
    }
    Strike::P1().open(cfg.door_open_ms);
    // Valida corrente do strike via INA219
    vTaskDelay(pdMS_TO_TICKS(200));   // aguarda rampa de corrente
    if (w.ina_p1_ma < cfg.ina_strike_min_ma) {
      UsbBridge::instance().sendAlert("STRIKE_P1_FAIL", _ctx);
      transition(State::ERROR); return;
    }
    _ctx.authorized_at = millis();
    return;
  }

  // P1 abriu: sucesso
  if (w.p1_open) {
    _ctx.authorized_at = 0;
    transition(State::INSIDE_WAIT); return;
  }

  // CORREÇÃO #14: timeout — entregador não empurrou a porta
  // Subtração segura: imune a overflow de 49,7 dias
  if (millis() - _ctx.authorized_at >= cfg.authorized_timeout_ms) {
    _ctx.authorized_at = 0;
    UsbBridge::instance().sendEvent("AUTHORIZATION_TIMED_OUT", _ctx);
    transition(State::IDLE);
  }
}

void StateMachine::_handleInsideWait(const PhysicalState& w) {
  auto& cfg = ConfigManager::instance().cfg;

  // P1 abriu antes de balança variar = entrou e saiu sem deixar nada
  if (w.p1_open && !w.person_inside) {
    transition(State::ABORTED); return;
  }

  // Timeout: pessoa dentro mas balança não variou
  if (millis() - _ctx.state_enter > cfg.inside_timeout_ms) {
    UsbBridge::instance().sendEvent("INSIDE_TIMEOUT", _ctx);
    transition(State::ABORTED); return;
  }

  // Balança detectou objeto: avança
  if (w.weight_g > cfg.min_delivery_weight_g) {
    _ctx.weight_g = w.weight_g;
    transition(State::DELIVERING);
  }
}

void StateMachine::_handleDelivering(const PhysicalState& w) {
  auto& cfg = ConfigManager::instance().cfg;

  if (_ctx.delivering_start == 0) _ctx.delivering_start = millis();

  // Fluxo normal: P1 abriu (entregador saindo)
  if (w.p1_open) {
    _ctx.delivering_start = 0;
    transition(State::DOOR_REOPENED); return;
  }

  // CORREÇÃO #15: timeout — saída não convencional (calço, etc.)
  if (millis() - _ctx.delivering_start >= cfg.delivering_timeout_ms) {
    _ctx.delivering_start    = 0;
    _ctx.unconventional_exit = true;
    _ctx.weight_g            = w.weight_g;
    // Gera JWT com flag de saída não convencional
    char jwt_buf[JWT_BUF_SIZE];
    uint32_t ts = _ctx.unix_time + ((millis() - _ctx.millis_at_sync) / 1000);
    char token[9];
    snprintf(token, sizeof(token), "%08lx", (unsigned long)esp_random());
    JwtSigner::instance().sign(token, _ctx.qr_code, _ctx.carrier,
      _ctx.weight_g, ts, jwt_buf, sizeof(jwt_buf));
    UsbBridge::instance().sendDelivery(_ctx, w, jwt_buf);
    UsbBridge::instance().sendPushAlert("DELIVERY_EXIT_UNCONVENTIONAL", _ctx);
    _ctx.unconventional_exit = false;
    transition(State::IDLE);
  }
}

void StateMachine::_handleDoorReopened(const PhysicalState& w) {
  auto& cfg = ConfigManager::instance().cfg;

  // P1 fechou e corredor vazio: confirmar entrega
  bool mmwave_ok = !cfg.require_mmwave_empty ||
    (!w.person_inside && HealthMonitor::instance().usable(SensorID::MMWAVE));

  if (!w.p1_open && mmwave_ok) {
    transition(State::CONFIRMING); return;
  }

  // P1 ainda aberta > 90s: alerta
  if (millis() - _ctx.state_enter > cfg.door_alert_ms) {
    transition(State::DOOR_ALERT);
  }
}

void StateMachine::_handleConfirming(const PhysicalState& w) {
  auto& cfg = ConfigManager::instance().cfg;

  // FSM preemptiva: P1 abriu durante CONFIRMING = segundo pacote
  if (w.p1_open) {
    _ctx.scale_settle_start = 0;
    transition(State::DELIVERING); return;
  }

  // Fase 1: inicia contagem de estabilização (não-bloqueante)
  if (_ctx.scale_settle_start == 0) {
    _ctx.scale_settle_start = millis(); return;
  }
  // Subtração segura: imune a overflow de 49,7 dias
  if (millis() - _ctx.scale_settle_start < cfg.scale_settle_ms) return;

  // Fase 2: lê peso final e gera JWT
  float weight = HealthMonitor::instance().usable(SensorID::HX711)
    ? w.weight_g : 0.0f;

  // CORREÇÃO v6: timestamp extrapolado entre sincronizações DS3231
  uint32_t ts = _ctx.unix_time +
    ((millis() - _ctx.millis_at_sync) / 1000);

  char token[9];
  snprintf(token, sizeof(token), "%08lx", (unsigned long)esp_random());

  char jwt_buf[JWT_BUF_SIZE];
  JwtSigner::instance().sign(token, _ctx.qr_code, _ctx.carrier,
    weight, ts, jwt_buf, sizeof(jwt_buf));

  UsbBridge::instance().sendDelivery(_ctx, w, jwt_buf);
  _ctx.scale_settle_start = 0;
  transition(State::RECEIPT);
}

void StateMachine::_handleReceipt(const PhysicalState& w) {
  // Preemptivo: P1 abre durante RECEIPT = segundo entregador
  if (w.p1_open) {
    transition(State::DELIVERING); return;
  }
  if (millis() - _ctx.state_enter > 120000) transition(State::IDLE);
}

void StateMachine::_handleAborted(const PhysicalState& w) {
  (void)w;
  UsbBridge::instance().sendEvent("DELIVERY_ABORTED", _ctx);
  vTaskDelay(pdMS_TO_TICKS(3000));
  transition(State::IDLE);
}

void StateMachine::_handleDoorAlert(const PhysicalState& w) {
  if (!w.p1_open) {
    UsbBridge::instance().sendAlert("DOOR_ALERT_RESOLVED", _ctx);
    transition(State::IDLE);
  }
}
