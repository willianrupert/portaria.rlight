# src/main.cpp
#include <Arduino.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "config/Config.h"
#include "config/ConfigManager.h"
#include "middleware/SharedMemory.h"
#include "fsm/StateMachine.h"
#include "health/HealthMonitor.h"
#include "sensors/Scale.h"
#include "sensors/MmWave.h"
#include "sensors/QRReader.h"
#include "sensors/PowerMonitor.h"
#include "actuators/Strike.h"
#include "actuators/Led.h"
#include "actuators/Buzzer.h"
#include "comms/UsbBridge.h"

// ISR state
volatile bool     g_p1_changed  = false;
volatile bool     g_p2_changed  = false;
volatile uint32_t g_last_isr_p1 = 0;
volatile uint32_t g_last_isr_p2 = 0;

void IRAM_ATTR isr_p1() {
  uint32_t now = millis();
  if (now - g_last_isr_p1 > DEBOUNCE_SW_MS) {
    g_p1_changed  = true;
    g_last_isr_p1 = now;
  }
}

void IRAM_ATTR isr_p2() {
  uint32_t now = millis();
  if (now - g_last_isr_p2 > DEBOUNCE_SW_MS) {
    g_p2_changed  = true;
    g_last_isr_p2 = now;
  }
}

void taskSensorHub(void* pvParameters) {
  esp_task_wdt_add(NULL);   // CORREÇÃO #11: Core 1 no TWDT

  PhysicalState state = {};
  // Spatial debouncing do radar
  static uint32_t radar_absent_since = 0;

  for (;;) {
    esp_task_wdt_reset();

    // ── Limit switches (ISR flags) ───────────────────────────────
    if (g_p1_changed) { g_p1_changed = false;
      state.p1_open = (digitalRead(PIN_SW_P1) == HIGH); }
    if (g_p2_changed) { g_p2_changed = false;
      state.p2_open = (digitalRead(PIN_SW_P2) == HIGH); }

    // ── LD2410B mmWave com Spatial Debouncing ────────────────────
    if (HealthMonitor::instance().usable(SensorID::MMWAVE)) {
      bool raw = MmWave::instance().personPresent();
      auto& cfg = ConfigManager::instance().cfg;
      if (raw) {
        state.person_present = true;
        radar_absent_since   = 0;
      } else {
        if (radar_absent_since == 0) radar_absent_since = millis();
        if (millis() - radar_absent_since >= cfg.radar_debounce_ms)
          state.person_present = false;
        // Dentro do debounce: mantém estado anterior
      }
      HealthMonitor::instance().report(SensorID::MMWAVE, true);
    }

    // ── GM861S QR Reader (buffer circular não-bloqueante) ─────────
    if (HealthMonitor::instance().usable(SensorID::GM861)) {
      const char* raw = QRReader::instance().poll();
      if (raw && strlen(raw) > 0) {
        strlcpy(state.qr_code, raw, sizeof(state.qr_code));
        QRReader::identifyCarrier(raw, state.carrier, sizeof(state.carrier));
        HealthMonitor::instance().report(SensorID::GM861, true);
      }
    }

    // ── HX711 Balança ─────────────────────────────────────────────
    if (HealthMonitor::instance().usable(SensorID::HX711)) {
      float w = Scale::instance().readOne();
      if (w > -9000.0f) {
        state.weight_g = w;
        HealthMonitor::instance().report(SensorID::HX711, true);
      } else {
        HealthMonitor::instance().report(SensorID::HX711, false, "hx711_read");
      }
    }

    // ── INA219 com I²C Bus Recovery ───────────────────────────────
    if (HealthMonitor::instance().usable(SensorID::INA219_P1)) {
      float ma = 0;
      bool ok = PowerMonitor::P1().readWithRecovery(ma);
      state.ina_p1_ma = ok ? ma : 0.0f;
      HealthMonitor::instance().report(SensorID::INA219_P1, ok, "ina219_emi");
    }

    // ── INA219 P2 se habilitado ───────────────────────────────
    if (ConfigManager::instance().cfg.enable_strike_p2 && HealthMonitor::instance().usable(SensorID::INA219_P2)) {
      float ma = 0;
      bool ok = PowerMonitor::P2().readWithRecovery(ma);
      state.ina_p2_ma = ok ? ma : 0.0f;
      HealthMonitor::instance().report(SensorID::INA219_P2, ok, "ina219_emi_p2");
    }

    // ── Auto-zero da balança (só em IDLE com portas fechadas) ─────
    bool idle_safe = (StateMachine::instance().current() == State::IDLE)
                  && !state.p1_open && !state.p2_open;
    Scale::instance().autoZeroTick(idle_safe);

    // ── Publica snapshot para Core 0 ─────────────────────────────
    SharedMemory::instance().update(state);

    vTaskDelay(pdMS_TO_TICKS(50));   // 20Hz
  }
}

void taskLogicBrain(void* pvParameters) {
  esp_task_wdt_add(NULL);

  uint32_t last_heartbeat = 0;
  uint32_t last_recovery  = 0;

  for (;;) {
    esp_task_wdt_reset();

    PhysicalState world = SharedMemory::instance().getSnapshot();

    // CORREÇÃO #16: Stale Data Protection
    if (world.sample_age_ms > (uint32_t)ConfigManager::instance().cfg.stale_data_max_ms) {
      vTaskDelay(pdMS_TO_TICKS(10)); continue;
    }

    StateMachine::instance().tick(world);

    // Tick dos atuadores assíncronos (CORREÇÃO #2 + #10)
    Strike::P1().tick(world.ina_p1_ma);
    
    if(ConfigManager::instance().cfg.enable_strike_p2) {
      Strike::P2().tick(world.ina_p2_ma);
    }
    
    Led::btn().tick();
    Led::qr().tick();

    UsbBridge::instance().processIncoming();

    if (millis() - last_heartbeat > HEALTH_REPORT_MS) {
      last_heartbeat = millis();
      UsbBridge::instance().sendHeartbeat(StateMachine::instance().ctx());
    }

    if (millis() - last_recovery > 30000) {
      last_recovery = millis();
      auto& hm = HealthMonitor::instance();
      hm.tryRecover(SensorID::GM861,  [] { return QRReader::instance().init(); });
      hm.tryRecover(SensorID::MMWAVE, [] { return MmWave::instance().init(); });
      hm.tryRecover(SensorID::HX711,  [] { return Scale::instance().init(); });
    }

    // Cooler térmico (feature toggle)
    if (ConfigManager::instance().cfg.enable_auto_cooler) {
      float tc = temperatureRead();
      auto& cfg = ConfigManager::instance().cfg;
      uint8_t pwm = (tc > cfg.cooler_temp_min_c)
        ? (uint8_t)constrain(map((long)tc,
            cfg.cooler_temp_min_c, cfg.cooler_temp_max_c, 64, 255), 0, 255)
        : 0;
      ledcWrite(2, pwm);
    }

    vTaskDelay(pdMS_TO_TICKS(10));   // 100Hz
  }
}

void setup() {
  // 1. RF desligado imediatamente — air-gapped
  esp_wifi_stop(); esp_wifi_deinit(); esp_bt_controller_disable();

  Serial.begin(115200);   // USB CDC pinos 19/20

  // 2. AMP infrastructure
  SharedMemory::instance().init();
  ConfigManager::instance().loadAll();

  // 3. Task Watchdog (5s panic)
  esp_task_wdt_init(TWDT_TIMEOUT_S, true);

  // 4. Pinos
  pinMode(PIN_SW_P1,     INPUT_PULLUP);
  pinMode(PIN_SW_P2,     INPUT_PULLUP);
  pinMode(PIN_BUTTON,    INPUT_PULLUP);
  pinMode(PIN_STRIKE_P1, OUTPUT); digitalWrite(PIN_STRIKE_P1, LOW);
  pinMode(PIN_STRIKE_P2, OUTPUT); digitalWrite(PIN_STRIKE_P2, LOW); // Setup paralelo P2
  pinMode(PIN_BUZZER,    OUTPUT); digitalWrite(PIN_BUZZER,    LOW);
  attachInterrupt(PIN_SW_P1, isr_p1, CHANGE);
  attachInterrupt(PIN_SW_P2, isr_p2, CHANGE);

  // 5. LEDs PWM
  Led::btn().init(PIN_LED_BTN, 0);
  Led::qr().init(PIN_LED_QR,   1);
  ledcSetup(2, 25000, 8); ledcAttachPin(PIN_COOLER, 2);

  // 6. Sensores com health report imediato
  auto& hm = HealthMonitor::instance();

  // HX711: boot raw para detectar pacote residual
  bool hx_ok = Scale::instance().initRaw();
  hm.report(SensorID::HX711, hx_ok, "hx711_init");
  if (hx_ok) {
    long raw = Scale::instance().readRaw();
    bool p1_closed = (digitalRead(PIN_SW_P1) == LOW);
    float est_g = Scale::instance().rawToGrams(raw);
    if (est_g > ConfigManager::instance().cfg.min_delivery_weight_g && p1_closed) {
      UsbBridge::instance().sendAlert("BOOT_RESIDUAL_WEIGHT", StateMachine::instance().ctx());
    }
    // Tare sempre na RAM — zero NVS
    Scale::instance().tare();
  }

  hm.report(SensorID::GM861,     QRReader::instance().init(),     "gm861_init");
  hm.report(SensorID::MMWAVE,    MmWave::instance().init(),       "mmwave_init");
  hm.report(SensorID::INA219_P1, PowerMonitor::P1().init(0x40),  "ina_p1_init");
  
  if(ConfigManager::instance().cfg.enable_strike_p2) {
    hm.report(SensorID::INA219_P2, PowerMonitor::P2().init(0x41),  "ina_p2_init");
  }

  // 7. Boot report
  UsbBridge::instance().sendHeartbeat(StateMachine::instance().ctx());

  // 8. FreeRTOS Tasks AMP
  xTaskCreatePinnedToCore(taskSensorHub,  "SensorHub",  4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(taskLogicBrain, "LogicBrain", 8192, NULL, 2, NULL, 0);
}

void loop() {
  // FreeRTOS puro: deleta a task do Arduino
  vTaskDelete(NULL);
}
