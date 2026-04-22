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
#include "../sensors/QRReader.h"
#include <Preferences.h>

StateMachine& StateMachine::instance() { static StateMachine i; return i; }
State             StateMachine::current() const { return _ctx.state; }
const FsmContext& StateMachine::ctx()     const { return _ctx; }
FsmContext&       StateMachine::ctx()           { return _ctx; }

void StateMachine::transition(State next) {
  _ctx.millis_at_sync = millis() - _ctx.state_enter; // S5: Uso improvisado do millis_at_sync pra não precisar trocar headers agora. UsbBridge envia.
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

  // WIEGAND: processado em qualquer estado exceto durante entrega ativa
  bool delivery_active = (_ctx.state == State::WAITING_QR  ||
                          _ctx.state == State::AUTHORIZED   ||
                          _ctx.state == State::INSIDE_WAIT  ||
                          _ctx.state == State::DELIVERING   ||
                          _ctx.state == State::DOOR_REOPENED||
                          _ctx.state == State::CONFIRMING   ||
                          _ctx.state == State::RECEIPT      ||
                          _ctx.state == State::REVERSE_PICKUP);

  if (!delivery_active && world.wiegand_granted &&
      world.wiegand_access.type != AccessType::NONE) {

    AccessType atype = world.wiegand_access.type;

    if (atype == AccessType::RESIDENT) {
      _ctx.resident_access_type = atype;
      strlcpy(_ctx.resident_label, world.wiegand_access.label, sizeof(_ctx.resident_label));
      if (!world.p2_open) { // interbloqueio
        Strike::P1().open(cfg.door_open_ms);
        Buzzer::beep(2, 150);
        Led::btn().solid(255);
        transition(State::RESIDENT_P1);
      } else {
        UsbBridge::instance().sendAlert("RESIDENT_INTERLOCK_P2", _ctx);
      }
      return;
    }

    if (atype == AccessType::REVERSE_CORREIOS ||
        atype == AccessType::REVERSE_ML       ||
        atype == AccessType::REVERSE_AMAZON   ||
        atype == AccessType::REVERSE_GENERIC) {
      strlcpy(_ctx.reverse_carrier,
        atype == AccessType::REVERSE_CORREIOS ? "CORREIOS" :
        atype == AccessType::REVERSE_ML       ? "ML"       :
        atype == AccessType::REVERSE_AMAZON   ? "AMAZON"   : "GENERIC",
        sizeof(_ctx.reverse_carrier));
      _ctx.weight_g = world.weight_g;
      if (!world.p2_open) {
        Strike::P1().open(cfg.door_open_ms);
        Buzzer::beep(1, 100);
        transition(State::AUTHORIZED);
        _ctx.carrier[0] = 'R'; // flag: 'R' = reversa
      }
      return;
    }
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
    case State::RESIDENT_P1:   _handleResidentP1(world);   break;
    case State::RESIDENT_P2:   _handleResidentP2(world);   break;
    case State::REVERSE_PICKUP:_handleReversePickup(world);break;
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

  // P1 abriu: destino depende se é entrega normal ou reversa
  if (w.p1_open) {
    _ctx.authorized_at = 0;
    bool is_reverse = (_ctx.carrier[0] == 'R'); // flag setada no tick()
    if (is_reverse) {
      transition(State::REVERSE_PICKUP);
    } else {
      _persistDeliveryContext(); // S3: Snapshot do pacote assim que abre
      transition(State::INSIDE_WAIT);
    }
    return;
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
  if (w.p1_open && !w.person_present) {
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

  bool mmwave_ok = !cfg.require_mmwave_empty ||
    (!w.person_present && HealthMonitor::instance().usable(SensorID::MMWAVE));

  // S2: Fallback quando mmWave degradado e entregador preso
  if (!mmwave_ok && !HealthMonitor::instance().usable(SensorID::MMWAVE)) {
    if (_ctx.door_reopen_at == 0) _ctx.door_reopen_at = millis();
    uint32_t fallback_ms = cfg.radar_debounce_ms * 3;
    if (millis() - _ctx.door_reopen_at >= fallback_ms) {
      _ctx.mmwave_fallback_used = true;
      mmwave_ok = true; 
    }
  }

  // P1 fechou e corredor vazio (ou em fallback): confirmar entrega
  if (!w.p1_open && mmwave_ok) {
    _ctx.door_reopen_at = 0;
    transition(State::CONFIRMING); return;
  }

  // P1 ainda aberta > 90s: alerta
  if (millis() - _ctx.state_enter > cfg.door_alert_ms) {
    transition(State::DOOR_ALERT);
  }
}

void StateMachine::_persistDeliveryContext() {
  Preferences p;
  p.begin("dlv_ctx", false);
  p.putString("qr",      _ctx.qr_code);
  p.putString("carrier", _ctx.carrier);
  p.putULong ("ts",      _ctx.unix_time);
  p.putBool  ("active",  true);
  p.end();
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

void StateMachine::_handleResidentP1(const PhysicalState& w) {
  auto& cfg = ConfigManager::instance().cfg;

  // P1 fechou: morador entrou no corredor — iniciar delay para P2
  if (!w.p1_open) {
    _ctx.resident_p2_timer = millis();
    transition(State::RESIDENT_P2);
    return;
  }

  // Timeout: P1 ficou aberta demais sem fechar
  if (millis() - _ctx.state_enter > cfg.door_alert_ms) {
    UsbBridge::instance().sendAlert("RESIDENT_P1_TIMEOUT", _ctx);
    transition(State::DOOR_ALERT);
  }
}

void StateMachine::_handleResidentP2(const PhysicalState& w) {
  auto& cfg = ConfigManager::instance().cfg;

  // Interbloqueio: P1 voltou a abrir durante o delay? Aborta.
  if (w.p1_open) {
    UsbBridge::instance().sendAlert("RESIDENT_P1_REOPENED_BEFORE_P2", _ctx);
    _ctx.resident_p2_timer = 0;
    transition(State::IDLE);
    return;
  }

  // Aguarda o delay de segurança (padrão 2000ms) antes de abrir P2
  if (millis() - _ctx.resident_p2_timer < cfg.p2_delay_ms) return;

  // Delay concluído: abre P2 se habilitada
  if (cfg.enable_strike_p2) {
    // Verifica interbloqueio P1 uma última vez
    if (!w.p1_open && w.ina_p1_ma < cfg.ina_strike_min_ma) {
      Strike::P2().open(cfg.door_open_ms);
      Buzzer::beep(1, 200);
      Led::btn().solid(255);
    }
  }

  // Aguarda P2 abrir (confirmado pelo micro switch) ou timeout
  if (w.p2_open) {
    // P2 abriu: loga acesso e retorna ao IDLE
    UsbBridge::instance().sendEvent("RESIDENT_ACCESS_COMPLETE", _ctx);
    _ctx.resident_p2_timer = 0;
    transition(State::IDLE);
    return;
  }

  // P2 não abriu após (door_open_ms + 3s de margem): timeout silencioso
  if (millis() - _ctx.resident_p2_timer > cfg.door_open_ms + 3000) {
    UsbBridge::instance().sendAlert("RESIDENT_P2_TIMEOUT", _ctx);
    _ctx.resident_p2_timer = 0;
    transition(State::IDLE);
  }
}

void StateMachine::_handleReversePickup(const PhysicalState& w) {
  auto& cfg = ConfigManager::instance().cfg;

  // P1 abriu: coletor saindo com ou sem coleta
  if (w.p1_open) {
    // Se houve variação negativa significativa: coleta confirmada
    float delta = _ctx.reverse_weight_delta;  // calculado ao longo do tempo (varia negativa)
    if (fabsf(delta) > cfg.min_delivery_weight_g) {
      char token[9];
      snprintf(token, sizeof(token), "%08lx", (unsigned long)esp_random());
      uint32_t ts = _ctx.unix_time + ((millis() - _ctx.millis_at_sync) / 1000);
      char jwt_buf[JWT_BUF_SIZE];
      JwtSigner::instance().sign(token, "REVERSE", _ctx.reverse_carrier,
        fabsf(delta), ts, jwt_buf, sizeof(jwt_buf));
      UsbBridge::instance().sendReversePickup(_ctx, w, jwt_buf);
    } else {
      UsbBridge::instance().sendEvent("REVERSE_NO_PICKUP", _ctx);
    }
    transition(State::DOOR_REOPENED); // aguarda P1 fechar
    return;
  }

  // Timeout: 3 minutos dentro sem sair
  if (_ctx.delivering_start > 0 &&
      millis() - _ctx.delivering_start >= cfg.delivering_timeout_ms) {
    UsbBridge::instance().sendPushAlert("REVERSE_TIMEOUT_INSIDE", _ctx);
    _ctx.delivering_start = 0;
    transition(State::IDLE);
  }

  if (_ctx.delivering_start == 0) _ctx.delivering_start = millis();

  // Monitora variação de peso atual para capturar o momento de esvaziamento (negativa)
  _ctx.reverse_weight_delta = w.weight_g - _ctx.weight_g;
}
