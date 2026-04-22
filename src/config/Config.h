// src/config/Config.h — v6.0 DEFINITIVO
#pragma once

// ── PINOS SEGUROS (WROOM-2 OCTAL COMPATIBLE) ────────────────────────
#define PIN_USB_D_MINUS     19
#define PIN_USB_D_PLUS      20
#define PIN_UART0_RX        44   // GM861S — RX
#define PIN_UART0_TX        43   // GM861S — TX
#define PIN_WIEGAND_D0       1   // Remapeado (Antigo 6)
#define PIN_WIEGAND_D1       2   // Remapeado (Antigo 7)
#define PIN_UART1_RX        18   // mmWave
#define PIN_UART1_TX        17   // mmWave
#define PIN_I2C_SDA         21   // Remapeado (Antigo 1)
#define PIN_I2C_SCL         38   // Remapeado (Antigo 2)
#define PIN_HX711_CLK       41   // Remapeado (Antigo 10)
#define PIN_HX711_DATA      42   // Remapeado (Antigo 11)
#define PIN_STRIKE_P1       15   // Remapeado (Antigo 45)
#define PIN_STRIKE_P2       16   // Remapeado (Antigo 46)
#define PIN_SW_P1           35   // Remapeado (Antigo 38)
#define PIN_SW_P2           36   // Remapeado (Antigo 39)
#define PIN_BUTTON          37   // Remapeado (Antigo 35)
#define PIN_BUZZER          48
#define PIN_LED_BTN         39   // Remapeado (Antigo 4)
#define PIN_LED_QR          40   // Remapeado (Antigo 5)
#define PIN_COOLER          47

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

// ── SCORE PESOS DE SAÚDE (Total 100) ─────────────────────────────────
#define SCORE_GM861          25
#define SCORE_WIEGAND        25
#define SCORE_HX711          20
#define SCORE_MMWAVE         15
#define SCORE_INA219_P1      10
#define SCORE_INA219_P2       5
