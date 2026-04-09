// src/config/Config.h — v6.0 DEFINITIVO
#pragma once

// ── PINOS ──────────────────────────────────────────────────────────
#define PIN_USB_D_MINUS     19
#define PIN_USB_D_PLUS      20
#define PIN_UART0_RX        44   // GM861S — DESCONECTAR ao flashar
#define PIN_UART0_TX        43
#define PIN_UART1_RX        18   // LD2410B mmWave
#define PIN_UART1_TX        17
#define PIN_I2C_SDA          1   // INA219 dual
#define PIN_I2C_SCL          2
#define PIN_HX711_CLK       10
#define PIN_HX711_DATA      11
#define PIN_STRIKE_P1       45   // MOSFET LR7843 — HIGH SÓLIDO, NUNCA PWM
#define PIN_STRIKE_P2       46   // Reservado p/ futura instalação P2
#define PIN_SW_P1           38   // Micro switch P1 (NC, pull-up interno)
#define PIN_SW_P2           39   // Micro switch P2 — apenas monitoramento
#define PIN_BUTTON          35   // Botão 30mm inox
#define PIN_BUZZER          48
#define PIN_LED_BTN          4   // PWM ledc canal 0
#define PIN_LED_QR           5   // PWM ledc canal 1
#define PIN_COOLER          47   // PWM ledc canal 2

// ── CONSTANTES DE HARDWARE (nunca mudam) ────────────────────────────
#define DEBOUNCE_SW_MS         50
#define TWDT_TIMEOUT_S          5
#define HEALTH_REPORT_MS    60000
#define NTP_SYNC_INTERVAL   600000  // 10 min
#define TS_SANITY_MIN  1700000000UL  // Nov/2023 — rejeita RTC morto

// ── BUFFER SIZES (stack — zero heap) ────────────────────────────────
#define UART_RX_BUF_SIZE    512
#define JWT_BUF_SIZE        512
#define JSON_OUT_BUF_SIZE   768
#define JSON_IN_BUF_SIZE    256
#define QR_RX_BUF_SIZE      128   // buffer circular GM861S

// ── SCORE PESOS DE SAÚDE ─────────────────────────────────────────────
#define SCORE_GM861          30   // QR = sensor mais crítico da v6
#define SCORE_HX711          25
#define SCORE_MMWAVE         25
#define SCORE_INA219_P1      20
#define SCORE_INA219_P2      20
