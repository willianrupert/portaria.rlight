// src/main.cpp - RLIGHT V7 ZERO-FRICTION (O SOBREVIVENTE)
#include <Arduino.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include <esp_task_wdt.h>

void setup() {
  // 1. SILENCIA O WATCHDOG PARA SEMPRE
  esp_task_wdt_deinit(); 

  // 2. SERIAL IMEDIATO
  Serial.begin(115200);   
  
  // 3. RF OFF
  esp_wifi_stop(); 
  esp_bt_controller_disable();

  Serial.println("\n\n--- RLIGHT_V7_ZERO_FRICTION_START ---");
  Serial.println("{\"status\": \"ALIVE\", \"mode\": \"STABLE\"}");
}

void loop() {
  static uint32_t last_msg = 0;
  uint32_t now = millis();

  if (now - last_msg > 1000) {
    last_msg = now;
    Serial.print("{\"uptime\": "); Serial.print(now); Serial.println(", \"log\": \"HEARTBEAT_STABLE\"}");
  }
}
