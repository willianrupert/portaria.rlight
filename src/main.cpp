// src/main.cpp - RLIGHT V8 ORCHESTRATOR
#include <Arduino.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include <esp_task_wdt.h>

#include "config/Config.h"
#include "config/ConfigManager.h"
#include "middleware/SharedMemory.h"
#include "fsm/StateMachine.h"
#include "sensors/Scale.h"
#include "sensors/MmWave.h"
#include "sensors/QRReader.h"
#include "sensors/KeypadHandler.h"
#include "actuators/Strike.h"
#include "comms/UsbBridge.h"

// Protótipos das Tasks
void taskLogicBrain(void* p);
void taskSensorHub(void* p);

void setup() {
  esp_task_wdt_init(TWDT_TIMEOUT_S, true); 
  Serial.begin(115200);
  
  // RF OFF para estabilidade da Balança e ADC
  esp_wifi_stop(); 
  esp_bt_controller_disable();

  // Inicialização de Middleware e Configs
  SharedMemory::instance().init();
  ConfigManager::instance().loadAll();
  UsbBridge::instance().init();

  // Inicialização de Sensores
  Scale::instance().init();
  MmWave::instance().init();
  QRReader::instance().init();
  KeypadHandler::instance().init(I2C_ADDR_KEYPAD); 
  PowerMonitor::P1().init(I2C_ADDR_INA219);
  PowerMonitor::P2().init(I2C_ADDR_INA219_P2);

  // Configuração de Pinos v8
  pinMode(PIN_SW_P1, INPUT_PULLUP);
  pinMode(PIN_SW_P2, INPUT_PULLUP);
  pinMode(PIN_SW_GATE, INPUT_PULLUP);
  pinMode(PIN_INT_BUTTON, INPUT_PULLUP);
  
  pinMode(PIN_STRIKE_P1, OUTPUT);
  pinMode(PIN_STRIKE_P2, OUTPUT);
  pinMode(PIN_GATE_MOTOR, OUTPUT);
  digitalWrite(PIN_STRIKE_P1, LOW);
  digitalWrite(PIN_STRIKE_P2, LOW);
  digitalWrite(PIN_GATE_MOTOR, LOW);

  // Criação das Tasks (Arquitetura AMP)
  // Core 0: Lógica e FSM (Alta frequência)
  xTaskCreatePinnedToCore(taskLogicBrain, "Brain", 8192, NULL, 5, NULL, 0);
  
  // Core 1: Sensor Hub (Baixa frequência)
  xTaskCreatePinnedToCore(taskSensorHub, "Sensors", 8192, NULL, 2, NULL, 1);

  Serial.println("\n\n--- RLIGHT_V8_ORCHESTRATOR_ALIVE ---");
}

void loop() {
  // Loop vazio — tudo roda via Tasks FreeRTOS
  vTaskDelay(pdMS_TO_TICKS(1000));
}

// ── CORE 0: LOGIC BRAIN (100Hz) ──────────────────────────────────────
void taskLogicBrain(void* p) {
  esp_task_wdt_add(NULL);
  TickType_t last_wake = xTaskGetTickCount();
  
  while (true) {
    esp_task_wdt_reset();
    // 1. Obtém snapshot do mundo físico
    PhysicalState world = SharedMemory::instance().getSnapshot();
    
    // 2. Processa FSM
    StateMachine::instance().tick(world);
    
    // 3. Processa Comandos USB (Host OPi)
    UsbBridge::instance().processIncoming();
    
    // 4. Tick dos Atuadores (Asíncrono)
    float current_p1 = world.ina_p1_ma;
    float current_p2 = world.ina_p2_ma;
    Strike::P1().tick(current_p1);
    Strike::P2().tick(current_p2);
    Strike::Gate().tick(0.0f); // Gate não tem INA monitorado via shunt direto v8
    
    // 5. Envio de Telemetria Periódica (500ms)
    static uint32_t last_telemetry = 0;
    if (millis() - last_telemetry >= 500) {
      UsbBridge::instance().sendTelemetry(world, StateMachine::instance().ctx());
      last_telemetry = millis();
    }
    
    vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10)); // 100Hz
  }
}

// ── CORE 1: SENSOR HUB (20Hz) ────────────────────────────────────────
void taskSensorHub(void* p) {
  esp_task_wdt_add(NULL);
  TickType_t last_wake = xTaskGetTickCount();
  PhysicalState s;

  while (true) {
    esp_task_wdt_reset();
    // A. Sensores Digitais e Reed Switches (v8)
    // Reed Switch NO + pull-up interno:
    // Imã presente (porta FECHADA) = LOW
    // Imã ausente (porta ABERTA)   = HIGH
    s.p1_open = (digitalRead(PIN_SW_P1) == HIGH); 
    s.p2_open = (digitalRead(PIN_SW_P2) == HIGH);
    s.gate_open = (digitalRead(PIN_SW_GATE) == HIGH);
    s.int_button_pressed = (digitalRead(PIN_INT_BUTTON) == LOW);

    // B. Sensores Analógicos / Serial
    s.weight_g = Scale::instance().readOne();
    s.person_present = MmWave::instance().personPresent();
    
    // C. QR Reader e Keypad (v7/v8)
    const char* code_qr = QRReader::instance().poll();
    if (code_qr && strlen(code_qr) > 0) {
      strlcpy(s.qr_code, code_qr, sizeof(s.qr_code));
      QRReader::identifyCarrier(code_qr, s.carrier, sizeof(s.carrier));
    } else {
      s.qr_code[0] = '\0';
    }

    KeypadHandler::instance().update();
    const char* code_key = KeypadHandler::instance().getCode();
    if (code_key != NULL) {
      AccessResult r = AccessController::instance().validate(code_key);
      s.keypad_granted = (r.type != AccessType::NONE && r.type != AccessType::DENIED);
      s.keypad_access.type = r.type;
      strlcpy(s.keypad_access.label, r.label, sizeof(s.keypad_access.label));
    } else {
      s.keypad_granted = false;
    }

    // D. Power Monitor (INA219)
    PowerMonitor::P1().readWithRecovery(s.ina_p1_ma);
    PowerMonitor::P2().readWithRecovery(s.ina_p2_ma);

    // E. Escrita na Memória Compartilhada
    SharedMemory::instance().update(s);
    
    vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(50)); // 20Hz
  }
}
