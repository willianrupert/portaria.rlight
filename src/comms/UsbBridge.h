// src/comms/UsbBridge.h — v6.0
#pragma once
#include <Arduino.h>
#include "../fsm/StateMachine.h"
#include <ArduinoJson.h>

class UsbBridge {
public:
  static UsbBridge& instance() { static UsbBridge i; return i; }

  void init();
  void checkStatus();
  bool screenReady() const { return _scr_rdy; }

  void sendHeartbeat(const FsmContext& ctx);
  void sendTelemetry(const PhysicalState& w, const FsmContext& ctx);
  void sendState(State s, const FsmContext& ctx);
  void sendAlert(const char* alert_type, const FsmContext& ctx);
  void sendPushAlert(const char* alert_type, const FsmContext& ctx);
  void sendEvent(const char* evt, const FsmContext& ctx);
  void sendDelivery(const FsmContext& ctx, const PhysicalState& w, const char* jwt);
  void sendReversePickup(const FsmContext& ctx, const PhysicalState& w, const char* jwt);

  void processIncoming();

private:
  UsbBridge() = default;
  bool _scr_rdy = false;
  char _rx_buf[256];
  uint8_t _rx_pos = 0;

  void _dispatch(const char* raw);
};
