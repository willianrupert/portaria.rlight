// src/config/ConfigManager.h — v6.0
#pragma once
#include <Preferences.h>
#include <string.h>

struct SystemConfig {
  // ── Thresholds operacionais (alteráveis via HA) ─────────────────
  uint32_t door_open_ms          = 3000;    // pulso do strike P1
  uint32_t p2_open_ms            = 3000;    // pulso do strike P2
  uint32_t gate_relay_ms         = 500;     // pulso do relé do portão Garen
  uint32_t led_btn_breathe_ms    = 2000;    // período PWM do LED do botão
  uint32_t led_btn_brightness_max= 200;     // brilho máximo PWM
  uint32_t awake_timeout_ms      = 60000;   // AWAKE sem ação → IDLE
  uint32_t qr_timeout_ms         = 30000;   // WAITING_QR sem leitura → IDLE
  uint32_t door_alert_ms         = 90000;   // P1 aberta → DOOR_ALERT
  uint32_t inside_timeout_ms     = 180000;  // INSIDE_WAIT sem balança → ABORTED
  uint32_t authorized_timeout_ms = 10000;   // AUTHORIZED sem P1 abrir → IDLE
  uint32_t delivering_timeout_ms = 180000;  // DELIVERING travado → IDLE forçado
  uint32_t loitering_alert_ms    = 30000;   // mmWave sem autorização → alerta
  uint32_t scale_settle_ms       = 2000;    // estabilização balança
  uint32_t p2_delay_ms           = 2000;    // delay entre P1 fechar e P2 abrir (evita queda tensão)
  uint32_t radar_debounce_ms     = 1500;    // ausência contínua → person=false
  uint32_t auto_zero_interval_ms = 600000;  // auto-zero RAM a cada 10min
  float    auto_zero_max_drift_g = 50.0f;   // drift máximo para auto-zero
  float    min_delivery_weight_g = 50.0f;   // variação mínima de entrega
  float    ina_strike_min_ma     = 400.0f;  // corrente mínima strike OK
  float    ina_strike_max_ma     = 1200.0f; // sobrecorrente (curto)
  float    stale_data_max_ms     = 150.0f;  // dados > 150ms → espera segura
  uint8_t  cooler_temp_min_c     = 45;
  uint8_t  cooler_temp_max_c     = 70;
  uint8_t  sensor_fail_threshold = 3;

  // ── Feature toggles (controle via HA, sem reflash) ───────────────
  bool maintenance_mode       = false; // FSM vai para MAINTENANCE
  bool enable_loitering_alarm = true;  // alerta mmWave prolongado
  bool require_mmwave_empty   = true;  // radar obrigatório para CONFIRMING
  bool enable_auto_cooler     = true;  // controle térmico automático
  bool enable_strike_p2       = true;  // Ativa lógica de interlock P2 (HARDWARE PRESENTE na v7)
};

class ConfigManager {
public:
  static ConfigManager& instance() {
    static ConfigManager inst; return inst;
  }
  SystemConfig cfg;

  void loadAll() {
    Preferences p;
    p.begin("rlcfg", true);
    cfg.door_open_ms          = p.getUInt ("door_ms",    3000);
    cfg.p2_open_ms            = p.getUInt ("p2_open_ms", 3000);
    cfg.gate_relay_ms         = p.getUInt ("gate_ms",    500);
    cfg.led_btn_breathe_ms    = p.getUInt ("btn_breath", 2000);
    cfg.led_btn_brightness_max= p.getUInt ("btn_brt",    200);
    cfg.awake_timeout_ms      = p.getUInt ("awake_ms",   60000);
    cfg.qr_timeout_ms         = p.getUInt ("qr_ms",      30000);
    cfg.door_alert_ms         = p.getUInt ("alert_ms",   90000);
    cfg.inside_timeout_ms     = p.getUInt ("inside_ms",  180000);
    cfg.authorized_timeout_ms = p.getUInt ("auth_ms",    10000);
    cfg.delivering_timeout_ms = p.getUInt ("dlv_ms",     180000);
    cfg.loitering_alert_ms    = p.getUInt ("loiter_ms",  30000);
    cfg.scale_settle_ms       = p.getUInt ("scale_ms",   2000);
    cfg.p2_delay_ms           = p.getUInt ("p2_delay",   2000);
    cfg.radar_debounce_ms     = p.getUInt ("radar_dbc",  1500);
    cfg.auto_zero_interval_ms = p.getUInt ("az_int",     600000);
    cfg.auto_zero_max_drift_g = p.getFloat("az_max",     50.0f);
    cfg.min_delivery_weight_g = p.getFloat("min_w_g",    50.0f);
    cfg.ina_strike_min_ma     = p.getFloat("ina_min",    400.0f);
    cfg.ina_strike_max_ma     = p.getFloat("ina_max",    1200.0f);
    cfg.stale_data_max_ms     = p.getFloat("stale_ms",   150.0f);
    cfg.cooler_temp_min_c     = p.getUChar("cool_min",   45);
    cfg.cooler_temp_max_c     = p.getUChar("cool_max",   70);
    cfg.sensor_fail_threshold = p.getUChar("fail_thr",   3);
    cfg.maintenance_mode      = p.getBool ("maint",      false);
    cfg.enable_loitering_alarm= p.getBool ("loiter_en",  true);
    cfg.require_mmwave_empty  = p.getBool ("mmw_req",    true);
    cfg.enable_auto_cooler    = p.getBool ("cooler_en",  true);
    cfg.enable_strike_p2      = p.getBool ("strike_p2",  true);
    p.end();
  }

  // REGRA DE OURO: só grava se o valor mudou — preserva ciclos da Flash
  void updateParam(const char* key, float value) {
    Preferences p;
    p.begin("rlcfg", false);

    // Macro interna: compara antes de gravar
    #define UPD_UINT(k, field) \
      if(!strcmp(key,k)){ uint32_t v=(uint32_t)value; \
        if(v!=cfg.field){ cfg.field=v; p.putUInt(k,v); } }
    #define UPD_FLOAT(k, field) \
      if(!strcmp(key,k)){ if(value!=cfg.field){ cfg.field=value; p.putFloat(k,value); } }
    #define UPD_BOOL(k, field) \
      if(!strcmp(key,k)){ bool v=(bool)value; \
        if(v!=cfg.field){ cfg.field=v; p.putBool(k,v); } }

    UPD_UINT ("door_ms",    door_open_ms)
    UPD_UINT ("p2_open_ms", p2_open_ms)
    UPD_UINT ("gate_ms",    gate_relay_ms)
    UPD_UINT ("btn_breath", led_btn_breathe_ms)
    UPD_UINT ("btn_brt",    led_btn_brightness_max)
    UPD_UINT ("awake_ms",   awake_timeout_ms)
    UPD_UINT ("qr_ms",      qr_timeout_ms)
    UPD_UINT ("alert_ms",   door_alert_ms)
    UPD_UINT ("auth_ms",    authorized_timeout_ms)
    UPD_UINT ("dlv_ms",     delivering_timeout_ms)
    UPD_UINT ("radar_dbc",  radar_debounce_ms)
    UPD_UINT ("az_int",     auto_zero_interval_ms)
    UPD_UINT ("p2_delay",   p2_delay_ms)
    UPD_FLOAT("az_max",     auto_zero_max_drift_g)
    UPD_FLOAT("min_w_g",    min_delivery_weight_g)
    UPD_FLOAT("ina_min",    ina_strike_min_ma)
    UPD_FLOAT("ina_max",    ina_strike_max_ma)
    UPD_FLOAT("stale_ms",   stale_data_max_ms)
    UPD_UINT ("cool_min",   cooler_temp_min_c)
    UPD_BOOL ("maint",      maintenance_mode)
    UPD_BOOL ("loiter_en",  enable_loitering_alarm)
    UPD_BOOL ("mmw_req",    require_mmwave_empty)
    UPD_BOOL ("cooler_en",  enable_auto_cooler)
    UPD_BOOL ("strike_p2",  enable_strike_p2)

    #undef UPD_UINT
    #undef UPD_FLOAT
    #undef UPD_BOOL
    p.end();
  }
};
