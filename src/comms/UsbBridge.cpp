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
    String code = doc["code"].as<String>();
    const char* tl = doc["type_label"] | "RESIDENT:Morador";
    bool ok = AccessController::instance().addCode(code.c_str(), tl);
    if(ok) sendEvent("WIEGAND_CODE_ADDED", fsm.ctx());
  }
  else if (!strcmp(cmd, "CMD_REMOVE_WIEGAND_CODE")) {
    String code = doc["code"].as<String>();
    bool ok = AccessController::instance().removeCode(code.c_str());
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
  else if (!strcmp(cmd, "CMD_UNLOCK_GATE")) {
    // Pulso no motor Garen
    Strike::Gate().open(500); // 500ms pulse for gate motor
    sendEvent("GATE_PULSE_SENT", fsm.ctx());
  }
  else if (!strcmp(cmd, "CMD_PULSE_STRIKE")) {
    Strike::P1().testPulse();
    if(ConfigManager::instance().cfg.enable_strike_p2) Strike::P2().testPulse();
  }
  else if (!strcmp(cmd, "SCREEN_READY")) {
    _scr_rdy = true;
  }
}

// --- Emissões JSON Reais via Serial ---

void UsbBridge::sendHeartbeat(const FsmContext& ctx) {
  JsonDocument doc;
  doc["type"] = "HEARTBEAT";
  doc["ts"]   = ctx.unix_time;
  serializeJson(doc, Serial);
  Serial.println();
}

void UsbBridge::sendTelemetry(const PhysicalState& w, const FsmContext& ctx) {
  JsonDocument doc;
  doc["type"]      = "TELEMETRY";
  doc["state"]     = StateMachine::stateToString(ctx.state);
  doc["weight"]    = w.weight_g;
  doc["p1_open"]   = w.p1_open;
  doc["p2_open"]   = w.p2_open;
  doc["gate_open"] = w.gate_open;
  doc["int_button_pressed"] = w.int_button_pressed;
  
  if (strlen(ctx.carrier) > 0) doc["carrier"] = ctx.carrier;
  if (strlen(ctx.resident_label) > 0) doc["resident_label"] = ctx.resident_label;
  
  serializeJson(doc, Serial);
  Serial.println();
}

void UsbBridge::sendState(State s, const FsmContext& ctx) {
  JsonDocument doc;
  doc["type"]  = "STATE_CHANGE";
  doc["state"] = StateMachine::stateToString(s);
  serializeJson(doc, Serial);
  Serial.println();
}

void UsbBridge::sendAlert(const char* alert_type, const FsmContext& ctx) {
  JsonDocument doc;
  doc["type"]  = "ALERT";
  doc["alert"] = alert_type;
  serializeJson(doc, Serial);
  Serial.println();
}

void UsbBridge::sendPushAlert(const char* alert_type, const FsmContext& ctx) {
  JsonDocument doc;
  doc["type"]  = "PUSH_ALERT";
  doc["alert"] = alert_type;
  serializeJson(doc, Serial);
  Serial.println();
}

void UsbBridge::sendEvent(const char* evt, const FsmContext& ctx) {
  JsonDocument doc;
  doc["type"]  = "EVENT";
  doc["event"] = evt;
  serializeJson(doc, Serial);
  Serial.println();
}

void UsbBridge::sendDelivery(const FsmContext& ctx, const PhysicalState& w, const char* jwt) {
  JsonDocument doc;
  doc["type"]    = "DELIVERY";
  doc["jwt"]     = jwt;
  doc["weight"]  = ctx.weight_g;
  doc["carrier"] = ctx.carrier;
  serializeJson(doc, Serial);
  Serial.println();
}

void UsbBridge::sendReversePickup(const FsmContext& ctx, const PhysicalState& w, const char* jwt) {
  JsonDocument doc;
  doc["type"]    = "REVERSE_PICKUP";
  doc["jwt"]     = jwt;
  doc["weight"]  = w.weight_g;
  doc["carrier"] = ctx.reverse_carrier;
  serializeJson(doc, Serial);
  Serial.println();
}
