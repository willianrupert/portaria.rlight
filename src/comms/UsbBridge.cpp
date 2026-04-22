// src/comms/UsbBridge.cpp — v6.0
#include "UsbBridge.h"
#include "../config/Config.h"
#include "../config/ConfigManager.h"
#include "../sensors/Scale.h"
#include "../actuators/Strike.h"

void UsbBridge::init() {
  _scr_rdy = false;
  _rx_pos = 0;
}

void UsbBridge::processIncoming() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r' || c == '\n') {
      if (_rx_pos > 0) {
        _rx_buf[_rx_pos] = '\0';
        _dispatch(_rx_buf);
        _rx_pos = 0;
      }
    } else {
      if (_rx_pos < sizeof(_rx_buf) - 1) {
        _rx_buf[_rx_pos++] = c;
      } else {
        _rx_pos = 0;
      }
    }
  }
}

void UsbBridge::_dispatch(const char* raw) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, raw);
  if (error) return;

  const char* cmd = doc["cmd"] | "";
  auto& fsm = StateMachine::instance();

  if (!strcmp(cmd, "CMD_SYNC_TIME")) {
    uint32_t ts = doc["ts"].as<uint32_t>();
    // CORREÇÃO #17: sanity check — rejeita RTC morto ou sem bateria
    // TS_SANITY_MIN = 1700000000UL (Novembro de 2023)
    if (ts < TS_SANITY_MIN) {
      fsm.ctx().system_time_invalid = true;
      sendEvent("SYSTEM_TIME_INVALID_RTC_CHECK", fsm.ctx());
      return;
    }
    fsm.ctx().unix_time       = ts;
    fsm.ctx().millis_at_sync  = millis();
    fsm.ctx().system_time_invalid = false;
  }
  else if (!strcmp(cmd, "CMD_UPDATE_CFG")) {
    const char* key = doc["key"] | "";
    float       val = doc["val"].as<float>();
    ConfigManager::instance().updateParam(key, val);
  }
  else if (!strcmp(cmd, "CMD_TARE_SCALE")) {
    Scale::instance().tare();
  }
  else if (!strcmp(cmd, "CMD_CALIBRATE_SCALE")) {
    Scale::instance().calibrate(doc["weight_g"].as<float>());
  }
  else if (!strcmp(cmd, "CMD_RESIDENT_UNLOCK")) {
    AccessResult synthetic;
    synthetic.type = AccessType::RESIDENT;
    strlcpy(synthetic.label, "HA Remote", sizeof(synthetic.label));
    fsm.ctx().resident_access_type = AccessType::RESIDENT;
    strlcpy(fsm.ctx().resident_label, "HA Remote", sizeof(fsm.ctx().resident_label));
    
    if (fsm.current() == State::IDLE) {
      if (!SharedMemory::instance().getSnapshot().p2_open) {
        Strike::P1().open(ConfigManager::instance().cfg.door_open_ms);
        fsm.transition(State::RESIDENT_P1);
      }
    }
  }
  else if (!strcmp(cmd, "CMD_ADD_WIEGAND_CODE")) {
    uint32_t code  = doc["code"].as<uint32_t>();
    const char* tl = doc["type_label"] | "RESIDENT:Morador";
    bool ok = AccessController::instance().addCode(code, tl);
    if(ok) sendEvent("WIEGAND_CODE_ADDED", fsm.ctx());
  }
  else if (!strcmp(cmd, "CMD_REMOVE_WIEGAND_CODE")) {
    uint32_t code = doc["code"].as<uint32_t>();
    bool ok = AccessController::instance().removeCode(code);
    if(ok) sendEvent("WIEGAND_CODE_REMOVED", fsm.ctx());
  }
  else if (!strcmp(cmd, "CMD_LIST_WIEGAND_CODES")) {
    char buf[512];
    AccessController::instance().listCodes(buf, sizeof(buf));
    sendEvent("WIEGAND_CODE_LIST", fsm.ctx());
  }
  else if (!strcmp(cmd, "CMD_UNLOCK_P1")) {
    fsm.transition(State::AUTHORIZED);
  }
  else if (!strcmp(cmd, "CMD_UNLOCK_P2")) {
    // Apenas se a flag estiver ativada nas configs ou comando expresso for ok.
    if(ConfigManager::instance().cfg.enable_strike_p2) {
      Strike::P2().open(ConfigManager::instance().cfg.door_open_ms);
      sendEvent("P2_UNLOCKED_REMOTELY", fsm.ctx());
    }
  }
  else if (!strcmp(cmd, "CMD_PULSE_STRIKE")) {
    Strike::P1().testPulse();
    if(ConfigManager::instance().cfg.enable_strike_p2) Strike::P2().testPulse();
  }
  else if (!strcmp(cmd, "SCREEN_READY")) {
    _scr_rdy = true;
  }
}

// Stubs for JSON emissions:
void UsbBridge::sendHeartbeat(const FsmContext& ctx) { /* Serial.println */ }
void UsbBridge::sendState(State s, const FsmContext& ctx) { /* ... */ }
void UsbBridge::sendAlert(const char* alert_type, const FsmContext& ctx) { /* ... */ }
void UsbBridge::sendPushAlert(const char* alert_type, const FsmContext& ctx) { /* ... */ }
void UsbBridge::sendEvent(const char* evt, const FsmContext& ctx) { /* ... */ }
void UsbBridge::sendDelivery(const FsmContext& ctx, const PhysicalState& w, const char* jwt) { /* ... */ }
void UsbBridge::sendReversePickup(const FsmContext& ctx, const PhysicalState& w, const char* jwt) { /* ... */ }
