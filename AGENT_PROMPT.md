# PROMPT PARA IA AGENT-FIRST — rlight Portaria Autônoma v7 DEFINITIVO

---

## 1. VISÃO GERAL DO SISTEMA

Você está implementando a versão 7 do firmware e software de um sistema de **portaria autônoma residencial** chamado **rlight**, em Jaboatão dos Guararapes, PE, Brasil.

O sistema é um **airlock de duas portas** controlado por fusão de sensores físicos, com prova criptográfica de cada entrega assinada por um microcontrolador ESP32-S3 cujo segredo nunca sai do chip.

**Premissa de design inviolável:** física determinística, nunca visão computacional para decisões críticas. Câmeras existem apenas para registro.

---

## 2. ESTADO ATUAL DO REPOSITÓRIO (BASELINE)

O repositório já foi implementado com as seguintes melhorias e está funcional:

### 2.1 Correções já aplicadas (v1–v6, todas ativas)
- #1 String→char[] + snprintf em todos os módulos
- #2 Strike assíncrono (_opened_at + _duration, nunca _open_until)
- #3 Buffer circular não-bloqueante no UsbBridge
- #4 TWDT esp_task_wdt_init(5, true) registrado em ambas as tasks
- #5 ISR com debounce 50ms por timestamp em IRAM_ATTR
- #6 FSM completamente não-bloqueante (zero delay() nos handlers)
- #7 HX711 em FreeRTOS Task no Core 1
- #8 RF desligado no nível de silício antes de qualquer init
- #9 JsonDocument (ArduinoJson v7, sem StaticJsonDocument)
- #10 millis() overflow — aritmética exclusivamente por subtração uint32_t
- #11 TWDT registrado na task do Core 1
- #12 tare() RAM only — NVS apenas para cal_factor
- #13 Telemetria em /dev/shm, SQLite SD apenas para entregas confirmadas
- #14 AUTHORIZED timeout 10s → IDLE se P1 não abrir
- #15 DELIVERING timeout 3min → JWT com unconventional_exit + IDLE
- #16 Stale Data Protection (sample_age_ms > cfg.stale_data_max_ms → skip FSM tick)
- #17 CMD_SYNC_TIME sanity check (ts < 1700000000UL descartado silenciosamente)

### 2.2 Melhorias do documento "rlight sugestões e melhorias" já implementadas
- S1: Proteção contra duplo acionamento do strike (_strike_armed flag)
- S2: Fallback graceful quando mmWave degradado (timer 3× debounce)
- S3: Persistência de contexto de entrega na NVS (namespace "dlv_ctx")
- S4: Watchdog de coerência FSM vs. hardware (checkCoherence())
- S5: Log de duração por estado (prev_duration_ms em STATE_TRANSITION)
- S6: Contador de ciclos do strike na NVS
- S7: Sync worker com backoff exponencial para upload de entregas
- S8: Foto vinculada ao token JWT ({token}.jpg)
- S9: Watchdog Python via sdnotify (WatchdogSec=10 no systemd)
- S10: API local FastAPI porta 8080 (/entregas, /ml/features, /status, /alerts)
- S11: Automações HA com alertas por transportadora
- S12: Abertura remota P1 com confirmação em duas etapas no HA
- S15: Schema ML (tabela delivery_features no SD)
- S19: Rotação de segredo JWT (dois slots NVS, CMD_ROTATE_SECRET)
- S20: Rate limiting de tentativas de QR (5 falhas em 10min → cooldown 5min)

### 2.3 Estrutura atual do repositório
```
host/
  core/
    config.py          ← HostConfig dataclass + DynamicTexts
    machine_state.py   ← MachineState observer pattern
    serial_bridge.py   ← SerialBridge USB CDC com reconexão elástica
    database.py        ← IMPLEMENTADO: dois bancos RAM+SD + delivery_features
  integrations/
    camera_usb.py      ← WebCamCapture, foto nomeada por token JWT
    homeassistant.py   ← MQTT Discovery completo + switch P2 + access status
    oracle_api.py      ← OracleRESTClient com retry exponencial
    sync_worker.py     ← IMPLEMENTADO: worker de sync com backoff
  ui/
    screens.py         ← Hierarquia visual por estado
    window.py          ← KioskWindow pygame kmsdrm
  local_api.py         ← IMPLEMENTADO: FastAPI porta 8080
  main.py              ← Entry point com sdnotify watchdog
  requirements.txt
scripts/
  rtc_sync.py          ← time.time() via hwclock do kernel (sem smbus2)
src/
  actuators/
    Buzzer.cpp/h
    Led.cpp/h
    Strike.cpp/h       ← CORRIGIDO: _strike_armed + aritmética subtração
  comms/
    UsbBridge.cpp/h    ← Buffer circular + sendHeartbeat/sendDelivery completos
  config/
    Config.h           ← Pinos + constantes de hardware
    ConfigManager.h    ← NVS wear-leveling + feature toggles
  crypto/
    JwtSigner.cpp/h    ← IMPLEMENTADO: mbedTLS real + rotação de segredo
  fsm/
    StateMachine.cpp/h ← 10 estados não-bloqueantes
  health/
    HealthMonitor.cpp/h
  middleware/
    SharedMemory.h     ← Mutex + sample_age_ms
  sensors/
    MmWave.cpp/h
    PowerMonitor.cpp/h ← readWithRecovery() I²C bus recovery
    QRReader.cpp/h     ← Buffer circular + identificação transportadora
    Scale.cpp/h        ← RAM tare + auto-zero + ML features
  main.cpp             ← AMP FreeRTOS, TWDT ambos os cores
platformio.ini
```

---

## 3. MAPA DE HARDWARE COMPLETO E DEFINITIVO

### 3.1 Microcontrolador
- **ESP32-S3 N16R8** — dual-core 240MHz, 16MB Flash, 8MB PSRAM
- WiFi/BT desligados permanentemente no setup() antes de qualquer outra inicialização

### 3.2 Pinos GPIO (definitivos)

| GPIO | Periférico | Interface | Direção | Observação crítica |
|------|-----------|-----------|---------|-------------------|
| 1 | INA219 SDA | I²C | I/O | P1=0x40, Pull-up ext 4,7kΩ. Bus recovery por bit-bang após EMI do strike |
| 2 | INA219 SCL | I²C | O | 9 pulsos de clock liberam SDA travada |
| 4 | LED azul botão | ledc 0 | O | Breathe=IDLE, blink=WAITING, solid=AWAKE |
| 5 | LED ilum. QR | ledc 1 | O | Acende em WAITING_QR |
| 6 | TF9S D0 (Wiegand) | ISR | I | Pull-up externo 2,2kΩ recomendado |
| 7 | TF9S D1 (Wiegand) | ISR | I | Pull-up externo 2,2kΩ recomendado |
| 10 | HX711 CLK | Bit-bang | O | Core 1, TWDT registrado |
| 11 | HX711 DATA | Bit-bang | I | tare() RAM only |
| 17 | LD2410B TX | UART1 | O | 256000 baud, spatial debouncing 1,5s |
| 18 | LD2410B RX | UART1 | I | |
| 19 | OPi USB D- | USB CDC | I/O | Comunicação principal + CMD_SYNC_TIME |
| 20 | OPi USB D+ | USB CDC | I/O | |
| 35 | Botão entregador | GPIO | I | Pull-up interno, debounce 50ms no handler |
| 38 | Micro switch P1 | ISR GPIO | I | NC, pull-up. LOW=fechado, HIGH=aberto |
| 39 | Micro switch P2 | ISR GPIO | I | NC, pull-up. Monitoramento + segurança |
| 43 | GM861S TX | UART0 | O | DESCONECTAR ao flashar pela 1ª vez |
| 44 | GM861S RX | UART0 | I | Buffer circular não-bloqueante |
| 45 | Strike P1 gate | MOSFET | O | HIGH SÓLIDO. NUNCA PWM. Flyback 1N4007 |
| 46 | Strike P2 gate | MOSFET | O | HIGH SÓLIDO. Desabilitado por padrão (enable_strike_p2=false) |
| 47 | Cooler 40mm | ledc 2 | O | PWM por temperatureRead() nativo |
| 48 | Buzzer ativo | GPIO | O | Padrões por estado na FSM |

### 3.3 Novos componentes desta versão
- **TF9S**: controle de acesso com teclado de senha + cartão IC + digital. Interface: **Wiegand 26-bit** via D0 (GPIO 6) e D1 (GPIO 7)
- **Strike P2**: instalado fisicamente (enable_strike_p2 agora default true). Controlado exclusivamente pela lógica do TF9S — **sem botão físico interno**

### 3.4 Orange Pi Zero 3
- Armbian, rootfs OverlayFS (somente-leitura), systemd watchdog
- Ethernet GbE como primário, WiFi como fallback
- DS3231 RTC gerenciado pelo kernel (hwclock), sem smbus2 no Python
- udev rule: /dev/rlight_esp sempre aponta para o ESP32-S3

---

## 4. DECISÕES DE DESIGN DESTA VERSÃO (v7)

### 4.1 Fluxo de acesso de moradores — REDESENHADO

**O botão interno de P2 foi eliminado.** O acesso de moradores funciona exclusivamente assim:

**Via TF9S (novo, principal):**
1. Morador digita senha / passa cartão / usa digital no TF9S
2. TF9S envia código Wiegand → ESP32 decodifica e valida
3. Se código = morador autorizado:
   - P1 abre por `cfg.door_open_ms` (padrão: 5000ms, editável via HA)
   - Morador entra no corredor, P1 fecha
   - Após `cfg.p2_delay_ms` (padrão: 2000ms) → P2 abre por `cfg.door_open_ms`
   - O delay de 2s evita queda de tensão por dois strikes acionados simultaneamente
   - Micro switch P2 confirma abertura
   - Após P2 fechar (sw2 volta ao LOW) → ciclo encerrado, log de acesso

**Via HA (emergência/remoto):**
- Botão `button.unlock_resident` no HA
- Sequência idêntica: P1 por 5s → delay 2s → P2 por 5s
- Com confirmação em duas etapas (já implementada, S12)

**Via chave física:**
- Fallback mecânico sempre disponível, sem interação com o sistema

### 4.2 Fluxo de acesso de coletores de reversa — NOVO

**Via TF9S com código de reversa:**
1. Coletor digita código de reversa cadastrado (específico por transportadora)
2. TF9S envia código Wiegand → ESP32 valida e identifica tipo REVERSA
3. **Apenas P1 abre** por `cfg.door_open_ms`
4. FSM vai para estado `REVERSE_PICKUP` (novo)
5. Balança monitorada: variação negativa = objeto retirado (coleta confirmada)
6. JWT gerado com `"direction": "OUTBOUND"`, `"carrier": transportadora identificada`
7. Morador notificado via HA push

### 4.3 Fluxo de entrega — INALTERADO
O fluxo de entrega via botão + QR Code não muda. Apenas P1 é usada no fluxo de entrega.

### 4.4 Separação de responsabilidade dos strikes
- **Strike P1**: entregadores (QR Code) + moradores (TF9S etapa 1) + coletores reversa
- **Strike P2**: exclusivamente moradores (TF9S etapa 2, após delay de 2s)
- **Interbloqueio**: P2 nunca abre se P1 estiver aberta (sw1=HIGH)

---

## 5. TAREFAS DE IMPLEMENTAÇÃO

### TAREFA 1 — Módulo Wiegand (novo sensor)

**Criar `src/sensors/Wiegand.h` e `src/sensors/Wiegand.cpp`**

O protocolo Wiegand 26-bit usa dois pinos (D0 e D1). Cada bit é representado por um pulso de ~50µs: pulso em D0 = bit 0, pulso em D1 = bit 1. O frame completo tem 26 bits (1 bit de paridade par + 8 bits site code + 16 bits card number + 1 bit de paridade ímpar). O payload útil são os 24 bits do meio.

```cpp
// src/sensors/Wiegand.h
#pragma once
#include <Arduino.h>
#include "../config/Config.h"

// Tamanho máximo do buffer de codes recebidos (queue circular)
#define WIEGAND_QUEUE_SIZE 4

class Wiegand {
public:
  static Wiegand& instance();
  void init();  // Configura ISRs nos pinos D0 e D1

  // Não-bloqueante. Retorna true se um código completo foi recebido.
  // O código decodificado fica disponível em lastCode().
  bool poll();

  uint32_t lastCode() const;      // Código Wiegand decodificado (24 bits úteis)
  const char* lastCodeStr() const; // Representação decimal em char[12]

  // Saúde: tempo desde último pulso recebido (para detectar sensor offline)
  uint32_t millisSinceLastPulse() const;

private:
  Wiegand() = default;

  // ISR data — volatile pois escrito nas ISRs
  static volatile uint32_t _bits;           // acumula bits recebidos
  static volatile uint8_t  _bit_count;      // quantos bits recebidos até agora
  static volatile uint32_t _last_pulse_ms;  // millis() do último pulso

  uint32_t _last_code        = 0;
  char     _last_code_str[12] = "";
  bool     _code_ready       = false;

  // Timeout: se não chegar o próximo bit em 25ms, o frame está completo ou corrompido
  static const uint32_t FRAME_TIMEOUT_MS = 25;
};

// ISRs globais — precisam ser funções livres para attachInterrupt
void IRAM_ATTR wiegand_isr_d0();
void IRAM_ATTR wiegand_isr_d1();
```

**Implementação crítica:**

```cpp
// src/sensors/Wiegand.cpp

volatile uint32_t Wiegand::_bits          = 0;
volatile uint8_t  Wiegand::_bit_count     = 0;
volatile uint32_t Wiegand::_last_pulse_ms = 0;

void Wiegand::init() {
  pinMode(PIN_WIEGAND_D0, INPUT_PULLUP);
  pinMode(PIN_WIEGAND_D1, INPUT_PULLUP);
  attachInterrupt(PIN_WIEGAND_D0, wiegand_isr_d0, FALLING);
  attachInterrupt(PIN_WIEGAND_D1, wiegand_isr_d1, FALLING);
}

// ISR D0: recebe bit 0
void IRAM_ATTR wiegand_isr_d0() {
  Wiegand::_bits         <<= 1;        // shift left, bit 0 implícito
  Wiegand::_bit_count++;
  Wiegand::_last_pulse_ms  = millis();
}

// ISR D1: recebe bit 1
void IRAM_ATTR wiegand_isr_d1() {
  Wiegand::_bits          = (Wiegand::_bits << 1) | 1; // shift + set LSB
  Wiegand::_bit_count++;
  Wiegand::_last_pulse_ms = millis();
}

bool Wiegand::poll() {
  _code_ready = false;
  if (_bit_count == 0) return false;

  // Frame completo: timeout de 25ms sem novo bit
  if (millis() - _last_pulse_ms < FRAME_TIMEOUT_MS) return false;

  // Captura atômica (desabilita ISRs brevemente)
  noInterrupts();
  uint32_t bits      = _bits;
  uint8_t  bit_count = _bit_count;
  _bits      = 0;
  _bit_count = 0;
  interrupts();

  // Wiegand 26: descarta bit de paridade MSB e LSB, pega 24 bits do meio
  if (bit_count == 26) {
    _last_code = (bits >> 1) & 0xFFFFFF;
  } else if (bit_count >= 4) {
    // Formatos alternativos (Wiegand 34, 37) — aceita os bits úteis
    _last_code = bits & 0xFFFFFF;
  } else {
    return false; // frame inválido
  }

  snprintf(_last_code_str, sizeof(_last_code_str), "%lu", (unsigned long)_last_code);
  _code_ready = true;
  return true;
}

uint32_t    Wiegand::lastCode()    const { return _last_code; }
const char* Wiegand::lastCodeStr() const { return _last_code_str; }
uint32_t    Wiegand::millisSinceLastPulse() const {
  return millis() - _last_pulse_ms;
}
```

---

### TAREFA 2 — AccessController: validação e tabela de códigos

**Criar `src/sensors/AccessController.h` e `src/sensors/AccessController.cpp`**

Este módulo é responsável por:
1. Receber o código decodificado pelo módulo Wiegand
2. Validar contra a tabela de códigos armazenada na NVS
3. Retornar o tipo de acesso concedido
4. Gerenciar rate limiting de tentativas inválidas

```cpp
// src/sensors/AccessController.h
#pragma once
#include <Arduino.h>

// Tipos de acesso possíveis
enum class AccessType {
  NONE,              // nenhum acesso ativo
  RESIDENT,          // morador — abre P1 + delay + P2
  REVERSE_CORREIOS,  // coleta reversa Correios — só P1
  REVERSE_ML,        // coleta reversa Mercado Livre — só P1
  REVERSE_AMAZON,    // coleta reversa Amazon — só P1
  REVERSE_GENERIC,   // coleta reversa genérica — só P1
  DENIED             // código reconhecido mas bloqueado
};

struct AccessResult {
  AccessType type    = AccessType::NONE;
  char       label[32] = ""; // ex: "Morador Principal", "ML Reversa"
};

class AccessController {
public:
  static AccessController& instance();

  // Tenta validar um código Wiegand.
  // Retorna AccessResult com o tipo de acesso.
  AccessResult validate(uint32_t wiegand_code);

  // Gerenciamento de códigos via comandos do HA (persistidos na NVS)
  // key format: "wieg_{code_decimal}" → value: "RESIDENT:Willian" ou "REVERSE_ML:ML Coleta"
  bool addCode(uint32_t code, const char* type_label);   // CMD_ADD_WIEGAND_CODE
  bool removeCode(uint32_t code);                         // CMD_REMOVE_WIEGAND_CODE
  void listCodes(char* out_buf, size_t buf_sz);           // CMD_LIST_WIEGAND_CODES

  // Rate limiting: retorna true se pode tentar (não está em cooldown)
  bool canAttempt();
  void recordFailure();
  void resetRateLimit();

private:
  AccessController() = default;

  // Rate limiting
  uint8_t  _fail_count      = 0;
  uint32_t _first_fail_ms   = 0;
  uint32_t _cooldown_until  = 0;

  static const uint8_t  MAX_FAILS      = 5;
  static const uint32_t FAIL_WINDOW_MS = 300000; // 5 minutos
  static const uint32_t COOLDOWN_MS    = 600000; // 10 minutos de bloqueio
};
```

**Implementação de `validate()`:**
```cpp
AccessResult AccessController::validate(uint32_t code) {
  AccessResult result;

  // Rate limiting: bloqueia se em cooldown
  if (!canAttempt()) {
    result.type = AccessType::DENIED;
    strlcpy(result.label, "RATE_LIMITED", sizeof(result.label));
    return result;
  }

  // Constrói chave NVS: "wieg_XXXXXXXX"
  char key[20];
  snprintf(key, sizeof(key), "wieg_%lu", (unsigned long)code);

  Preferences p;
  p.begin("wieg_db", true); // read-only
  char val[48] = "";
  p.getString(key, val, sizeof(val));
  p.end();

  if (strlen(val) == 0) {
    // Código não encontrado
    recordFailure();
    result.type = AccessType::NONE;
    return result;
  }

  // Reset rate limit em acesso bem-sucedido
  resetRateLimit();

  // Parseia o valor: "TIPO:Label"
  // ex: "RESIDENT:Willian Rupert" ou "REVERSE_ML:Coleta ML"
  char type_str[24] = "";
  char label_str[24] = "";
  const char* colon = strchr(val, ':');
  if (colon) {
    size_t type_len = colon - val;
    strlcpy(type_str, val, min(type_len + 1, sizeof(type_str)));
    strlcpy(label_str, colon + 1, sizeof(label_str));
  } else {
    strlcpy(type_str, val, sizeof(type_str));
  }
  strlcpy(result.label, label_str, sizeof(result.label));

  if      (!strcmp(type_str, "RESIDENT"))        result.type = AccessType::RESIDENT;
  else if (!strcmp(type_str, "REVERSE_CORREIOS")) result.type = AccessType::REVERSE_CORREIOS;
  else if (!strcmp(type_str, "REVERSE_ML"))       result.type = AccessType::REVERSE_ML;
  else if (!strcmp(type_str, "REVERSE_AMAZON"))   result.type = AccessType::REVERSE_AMAZON;
  else if (!strcmp(type_str, "REVERSE_GENERIC"))  result.type = AccessType::REVERSE_GENERIC;
  else                                             result.type = AccessType::NONE;

  return result;
}
```

---

### TAREFA 3 — Novos estados e lógica da FSM

#### 3.1 Enum de estados atualizado

```cpp
enum class State {
  IDLE,            // standby — LED pulsa
  MAINTENANCE,     // modo manutenção — sem hardware ativo
  AWAKE,           // botão pressionado — aguarda SCREEN_READY
  WAITING_QR,      // aguardando leitura de QR Code (entrega)
  AUTHORIZED,      // strike P1 aberto (entrega ou reversa)
  INSIDE_WAIT,     // P1 fechou, pessoa dentro, aguarda balança
  DELIVERING,      // balança detectou depósito
  DOOR_REOPENED,   // P1 abriu para saída do entregador
  CONFIRMING,      // P1 fechou, corredor vazio, gerando JWT
  RECEIPT,         // comprovante na tela
  ABORTED,         // entrou mas não entregou / não depositou
  RESIDENT_P1,     // NOVO: morador — P1 aberta, aguardando entrar
  RESIDENT_P2,     // NOVO: morador — P1 fechou, delay 2s, abrindo P2
  REVERSE_PICKUP,  // NOVO: coletor de reversa dentro do corredor
  DOOR_ALERT,      // P1 aberta além do timeout
  ERROR            // falha crítica bloqueante
};
```

#### 3.2 Novos campos no FsmContext

```cpp
struct FsmContext {
  // ... campos existentes mantidos ...

  // Contexto de acesso de morador
  AccessType resident_access_type = AccessType::NONE;
  char       resident_label[32]   = "";  // "Willian Rupert"
  uint32_t   resident_p1_opened   = 0;   // millis() quando P1 abriu para morador

  // Contexto de reversa
  char       reverse_carrier[24]  = "";  // transportadora identificada
  float      reverse_weight_delta = 0.0f; // variação de peso (negativa = retirada)

  // Telemetria de entrega (para ML)
  uint32_t   delivery_inside_start  = 0;
  uint8_t    door_open_count        = 0;
  bool       mmwave_fallback_used   = false;

  // Sub-estados não-bloqueantes
  uint32_t   scale_settle_start  = 0;
  uint32_t   authorized_at       = 0;
  uint32_t   delivering_start    = 0;
  uint32_t   resident_p2_timer   = 0;   // início do delay de 2s para P2
  bool       unconventional_exit = false;

  // Timestamp extrapolado
  uint32_t   unix_time        = 0;
  uint32_t   millis_at_sync   = 0;
  bool       system_time_invalid = false;

  // Controle
  bool       lockdown  = false;
  State      state     = State::IDLE;
  uint32_t   state_enter = 0;
  char       qr_code[64]  = "";
  char       carrier[24]  = "";
  float      weight_g     = 0.0f;
  bool       p1_open      = false;
  bool       p2_open      = false;
};
```

#### 3.3 Handler RESIDENT_P1 (P1 abriu para morador)

```cpp
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
```

#### 3.4 Handler RESIDENT_P2 (delay 2s → abre P2)

```cpp
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
  // Subtração uint32_t — imune a overflow de 49,7 dias
  if (millis() - _ctx.resident_p2_timer < cfg.p2_delay_ms) return;

  // Delay concluído: abre P2 se habilitada
  if (cfg.enable_strike_p2) {
    // Verifica interbloqueio P1 uma última vez
    if (!w.p1_open) {
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
```

#### 3.5 Handler REVERSE_PICKUP (coletor de reversa)

```cpp
void StateMachine::_handleReversePickup(const PhysicalState& w) {
  auto& cfg = ConfigManager::instance().cfg;

  // P1 abriu: coletor saindo com ou sem coleta
  if (w.p1_open) {
    // Se houve variação negativa significativa: coleta confirmada
    float delta = _ctx.reverse_weight_delta;  // calculado em INSIDE_WAIT
    if (fabsf(delta) > cfg.min_delivery_weight_g) {
      // Gera JWT de coleta reversa
      char token[9];
      snprintf(token, sizeof(token), "%08lx", (unsigned long)esp_random());
      uint32_t ts = _ctx.unix_time + ((millis() - _ctx.millis_at_sync) / 1000);
      char jwt_buf[JWT_BUF_SIZE];
      JwtSigner::instance().sign(token, "REVERSE", _ctx.reverse_carrier,
        fabsf(delta), ts, jwt_buf, sizeof(jwt_buf));
      UsbBridge::instance().sendReversePickup(_ctx, w, jwt_buf);
    } else {
      // Entrou mas não retirou nada
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

  // Monitora variação de peso (negativa = objeto sendo retirado)
  _ctx.reverse_weight_delta = w.weight_g - _ctx.weight_g; // weight_g = peso inicial
}
```

#### 3.6 Atualização do tick() — ponto de entrada do Wiegand

```cpp
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
  // Não interrompe fluxo de entrega em andamento
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
      // Morador: salva contexto e inicia sequência P1 → delay → P2
      _ctx.resident_access_type = atype;
      strlcpy(_ctx.resident_label, world.wiegand_access.label,
              sizeof(_ctx.resident_label));
      // Abre P1
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
      // Coletor de reversa: apenas P1
      const char* carrier_map[] = {
        "", "REVERSE", "CORREIOS", "ML", "AMAZON", "GENERIC"
      };
      // Mapeia enum para string de transportadora
      strlcpy(_ctx.reverse_carrier,
        atype == AccessType::REVERSE_CORREIOS ? "CORREIOS" :
        atype == AccessType::REVERSE_ML       ? "ML"       :
        atype == AccessType::REVERSE_AMAZON   ? "AMAZON"   : "GENERIC",
        sizeof(_ctx.reverse_carrier));
      _ctx.weight_g = world.weight_g; // captura peso inicial para calcular delta
      if (!world.p2_open) {
        Strike::P1().open(cfg.door_open_ms);
        Buzzer::beep(1, 100);
        transition(State::AUTHORIZED); // reutiliza AUTHORIZED para monitorar entrada
        // flag para _handleAuthorized saber que é reversa:
        // quando P1 abrir, vai para REVERSE_PICKUP em vez de INSIDE_WAIT
        _ctx.carrier[0] = 'R'; // flag: 'R' = reversa (verificado em _handleAuthorized)
      }
      return;
    }
  }

  // DOOR_ALERT preemptivo
  if (world.p1_open && _ctx.state == State::IDLE) {
    transition(State::DOOR_ALERT); return;
  }

  switch (_ctx.state) {
    case State::IDLE:          _handleIdle(world);         break;
    case State::MAINTENANCE:   _handleMaintenance(world);  break;
    case State::AWAKE:         _handleAwake(world);        break;
    case State::WAITING_QR:    _handleWaitingQr(world);    break;
    case State::AUTHORIZED:    _handleAuthorized(world);   break;
    case State::INSIDE_WAIT:   _handleInsideWait(world);   break;
    case State::DELIVERING:    _handleDelivering(world);   break;
    case State::DOOR_REOPENED: _handleDoorReopened(world); break;
    case State::CONFIRMING:    _handleConfirming(world);   break;
    case State::RECEIPT:       _handleReceipt(world);      break;
    case State::ABORTED:       _handleAborted(world);      break;
    case State::RESIDENT_P1:   _handleResidentP1(world);   break;
    case State::RESIDENT_P2:   _handleResidentP2(world);   break;
    case State::REVERSE_PICKUP:_handleReversePickup(world);break;
    case State::DOOR_ALERT:    _handleDoorAlert(world);    break;
    default: break;
  }
}
```

**ATENÇÃO — Atualizar `_handleAuthorized`** para distinguir entrega de reversa:
```cpp
void StateMachine::_handleAuthorized(const PhysicalState& w) {
  // ...código existente de abertura de P1 e validação INA219...

  // P1 abriu: destino depende se é entrega normal ou reversa
  if (w.p1_open) {
    _ctx.authorized_at = 0;
    bool is_reverse = (_ctx.carrier[0] == 'R'); // flag setada no tick()
    if (is_reverse) {
      transition(State::REVERSE_PICKUP);
    } else {
      transition(State::INSIDE_WAIT);
    }
    return;
  }
  // ...resto do timeout...
}
```

---

### TAREFA 4 — PhysicalState: novos campos para Wiegand

```cpp
// src/middleware/SharedMemory.h — adicionar ao struct PhysicalState:

struct AccessGranted {
  AccessType type       = AccessType::NONE;
  char       label[32]  = "";
};

// No PhysicalState:
bool          wiegand_granted  = false;  // true por um ciclo quando código chega
AccessGranted wiegand_access   = {};     // tipo e label do acesso
uint32_t      sample_age_ms    = 0;
// ... campos existentes mantidos ...
```

**No `taskSensorHub`**, após inicializar todos os sensores:
```cpp
// Wiegand — polling do frame completo
if (Wiegand::instance().poll()) {
  uint32_t code = Wiegand::instance().lastCode();
  AccessResult result = AccessController::instance().validate(code);

  if (result.type != AccessType::NONE && result.type != AccessType::DENIED) {
    state.wiegand_granted = true;
    state.wiegand_access.type = result.type;
    strlcpy(state.wiegand_access.label, result.label,
            sizeof(state.wiegand_access.label));
    HealthMonitor::instance().report(SensorID::WIEGAND, true);
  } else if (result.type == AccessType::DENIED) {
    UsbBridge::instance().sendAlert("WIEGAND_ACCESS_DENIED", ...);
    HealthMonitor::instance().report(SensorID::WIEGAND, true);
  } else {
    UsbBridge::instance().sendAlert("WIEGAND_CODE_UNKNOWN", ...);
  }
} else {
  // Flag consumida pela FSM — resetar a cada ciclo
  state.wiegand_granted = false;
  // Verifica saúde pelo tempo desde último pulso
  // Se > 5min sem nenhum pulso E houve tentativa: pode ser sinal de hardware offline
  // (Mas silêncio normal em standby é OK — não marcar como degradado por isso)
}
```

---

### TAREFA 5 — ConfigManager: novos parâmetros

Adicionar ao `SystemConfig` e ao `loadAll()` / `updateParam()`:

```cpp
// Timing de acesso de moradores
uint32_t p2_delay_ms    = 2000;  // delay entre abrir P1 e abrir P2 (evita queda de tensão)

// Flags já existentes — manter:
bool enable_strike_p2  = true;  // MUDANÇA: agora true por padrão (hardware instalado)
```

**No `updateParam()`**, adicionar:
```cpp
UPD_UINT("p2_delay_ms", p2_delay_ms)
```

**Adicionar `CMD_ADD_WIEGAND_CODE` e `CMD_REMOVE_WIEGAND_CODE` ao UsbBridge:**
```cpp
else if (!strcmp(cmd, "CMD_ADD_WIEGAND_CODE")) {
  uint32_t code  = doc["code"].as<uint32_t>();
  const char* tl = doc["type_label"] | "RESIDENT:Morador";
  bool ok = AccessController::instance().addCode(code, tl);
  // Confirma via sendEvent
}
else if (!strcmp(cmd, "CMD_REMOVE_WIEGAND_CODE")) {
  uint32_t code = doc["code"].as<uint32_t>();
  AccessController::instance().removeCode(code);
}
else if (!strcmp(cmd, "CMD_LIST_WIEGAND_CODES")) {
  char buf[512];
  AccessController::instance().listCodes(buf, sizeof(buf));
  // Envia via sendEvent
}
```

---

### TAREFA 6 — HealthMonitor: adicionar SensorID::WIEGAND

```cpp
// src/health/HealthMonitor.h
enum class SensorID {
  GM861,
  HX711,
  MMWAVE,
  INA219_P1,
  INA219_P2,
  WIEGAND,   // NOVO
  _COUNT     // SEMPRE ÚLTIMO
};
```

Atualizar array `_failures` e os pesos de score em Config.h:
```cpp
#define SCORE_WIEGAND  20  // sensor crítico para acesso de moradores
// Rebalancear: GM861=25, HX711=20, MMWAVE=15, INA219_P1=10, INA219_P2=5, WIEGAND=25
// Total = 100
```

---

### TAREFA 7 — Home Assistant: discovery para controle de acessos

No `host/integrations/homeassistant.py`, adicionar ao `_publish_discovery()`:

```python
# Sensor — estado de acesso atual
s_access = {
  "name": "Último Acesso", 
  "state_topic": f"rlight/{self.DEVICE_ID}/last_access",
  "unique_id": f"{self.DEVICE_ID}_last_access",
  "icon": "mdi:door-open",
  "device": device_def
}

# Button — cadastrar novo código Wiegand
# (payload: JSON com code e type_label)
b_add_code = {
  "name": "Cadastrar Código de Acesso",
  "command_topic": f"rlight/{self.DEVICE_ID}/wiegand_add",
  "unique_id": f"{self.DEVICE_ID}_wiegand_add",
  "device": device_def
}

# Button — abertura remota de morador (sequência P1 → delay → P2)
b_resident = {
  "name": "Abrir Portaria (Morador)",
  "command_topic": f"rlight/{self.DEVICE_ID}/cmd_resident_unlock",
  "unique_id": f"{self.DEVICE_ID}_resident_unlock",
  "device": device_def
}

# Number — delay P2 (ms)
n_p2_delay = {
  "name": "Delay P1→P2 (ms)",
  "command_topic": f"rlight/{self.DEVICE_ID}/set_p2_delay",
  "state_topic": f"rlight/{self.DEVICE_ID}/cfg_p2_delay",
  "min": 500, "max": 5000, "step": 100,
  "unique_id": f"{self.DEVICE_ID}_p2_delay",
  "device": device_def
}
```

No `_on_message`, adicionar:
```python
elif topic == f"rlight/{self.DEVICE_ID}/cmd_resident_unlock":
    # Sequência idêntica ao TF9S: P1 → delay → P2
    esp32_bridge.send_cmd("CMD_RESIDENT_UNLOCK")

elif topic == f"rlight/{self.DEVICE_ID}/wiegand_add":
    data = json.loads(payload)
    esp32_bridge.send_cmd("CMD_ADD_WIEGAND_CODE", {
        "code": data["code"],
        "type_label": data["type_label"]
    })

elif topic == f"rlight/{self.DEVICE_ID}/set_p2_delay":
    esp32_bridge.send_cmd("CMD_UPDATE_CFG", {
        "key": "p2_delay_ms",
        "val": float(payload)
    })
```

No `UsbBridge.cpp`, adicionar CMD_RESIDENT_UNLOCK:
```cpp
else if (!strcmp(cmd, "CMD_RESIDENT_UNLOCK")) {
  // Dispara o mesmo fluxo que o TF9S para morador
  // Injeta acesso sintético na FSM
  AccessResult synthetic;
  synthetic.type = AccessType::RESIDENT;
  strlcpy(synthetic.label, "HA Remote", sizeof(synthetic.label));
  // O taskLogicBrain vai processar no próximo tick via flag especial
  StateMachine::instance().ctx().resident_access_type = AccessType::RESIDENT;
  strlcpy(StateMachine::instance().ctx().resident_label, "HA Remote",
          sizeof(StateMachine::instance().ctx().resident_label));
  // Transição direta se em IDLE
  if (StateMachine::instance().current() == State::IDLE) {
    if (!SharedMemory::instance().getSnapshot().p2_open) {
      Strike::P1().open(ConfigManager::instance().cfg.door_open_ms);
      StateMachine::instance().transition(State::RESIDENT_P1);
    }
  }
}
```

---

### TAREFA 8 — setup() e taskSensorHub: inicializar Wiegand

No `src/main.cpp`:

```cpp
// No setup(), após inicializar outros sensores:
Wiegand::instance().init();
AccessController::instance(); // inicializa singleton (NVS carregado no construtor)
hm.report(SensorID::WIEGAND, true, "wiegand_init"); // Wiegand não tem init que pode falhar

// No taskSensorHub, adicionar PIN_INTERNAL_BTN (REMOVIDO — sem botão interno)
// PIN_ACCESS_CTRL_RX/TX (REMOVIDO — sem UART, Wiegand usa ISRs D0/D1)
```

---

### TAREFA 9 — Telas do Orange Pi para novos estados

No `host/ui/screens.py`, adicionar:

```python
@staticmethod
def render_resident_p1():
    """P1 aberta para morador — instrução de entrar."""
    kiosk.dpms_control(True)
    kiosk.clear()
    ScreenRender.draw_centered_text("BEM-VINDO", kiosk.font_title, kiosk.NEON_GREEN, 150)
    ScreenRender.draw_centered_text("Entre e aguarde a porta interna abrir",
                                    kiosk.font_subtitle, kiosk.WHITE, 320)

@staticmethod
def render_resident_p2():
    """Delay antes de P2 abrir — feedback visual."""
    kiosk.clear()
    ScreenRender.draw_centered_text("AGUARDE...", kiosk.font_title, kiosk.WHITE, 200)
    pulse = (math.sin(time.time() * 4) + 1) / 2
    g = int(50 + pulse * 200)
    ScreenRender.draw_centered_text("Abrindo porta interna",
                                    kiosk.font_subtitle, (0, g, 0), 350)

@staticmethod
def render_reverse_pickup():
    """Coletor de reversa dentro do corredor."""
    kiosk.clear()
    ScreenRender.draw_centered_text("COLETA AUTORIZADA", kiosk.font_title, kiosk.WHITE, 150)
    ScreenRender.draw_centered_text("Retire o pacote e feche a porta",
                                    kiosk.font_subtitle, (255, 200, 0), 300)
    ScreenRender.draw_centered_text("Aguardando saída...",
                                    kiosk.font_info, (150, 150, 150), 400)
```

No `main.py`, adicionar ao mapeamento de estados:
```python
elif state == "RESIDENT_P1":
    StateScreens.render_resident_p1()
elif state == "RESIDENT_P2":
    StateScreens.render_resident_p2()
elif state == "REVERSE_PICKUP":
    StateScreens.render_reverse_pickup()
```

---

## 6. REGRAS ABSOLUTAS (não negociáveis)

1. **Zero `String`** do Arduino em qualquer path — `char[]` + `snprintf` + `strlcpy`
2. **Zero `delay()`** em handlers FSM — toda espera via `millis() - timestamp`
3. **Toda aritmética de tempo** por subtração `uint32_t`: `millis() - _opened_at >= _duration`
4. **NVS wear-leveling**: `putX()` apenas quando valor mudou (comparar antes)
5. **tare()** da balança nunca toca NVS — RAM only
6. **ArduinoJson v7**: `JsonDocument doc` sem template
7. **TWDT em ambas as tasks**: `esp_task_wdt_add` + `reset` a cada ciclo
8. **ISRs Wiegand em IRAM_ATTR** com captura atômica (`noInterrupts()`/`interrupts()`)
9. **Interbloqueio P1/P2**: nenhum strike abre se o outro portão estiver aberto
10. **enable_strike_p2 agora default = true** (hardware instalado)
11. **P2 delay de 2s**: `p2_delay_ms` configurável, implementado como timer não-bloqueante
12. **Sem botão físico interno para P2** — acesso de moradores exclusivamente via TF9S ou HA
13. **Stale Data Protection**: skip FSM tick se `sample_age_ms > cfg.stale_data_max_ms`
14. **RF desligado no setup()** antes de qualquer outra inicialização
15. **Foto nomeada pelo token JWT** (ex: `a3f9c2b1.jpg`)
16. **Telemetria em `/dev/shm`**, entregas em SD com SQLite WAL

---

## 7. TABELA DE ESTADOS FINAL (16 estados)

| Estado | Ativado por | Comportamento | Transições saída |
|--------|-------------|---------------|-----------------|
| IDLE | Boot / fim de ciclo | LED pulsa, auto-zero balança, polling loitering | AWAKE, MAINTENANCE, RESIDENT_P1, AUTHORIZED(reversa), DOOR_ALERT |
| MAINTENANCE | HA toggle | Sensores ativos, zero hardware acionado | IDLE (quando HA desativa) |
| AWAKE | Botão entregador | Tela ativa, aguarda SCREEN_READY | WAITING_QR, IDLE (timeout) |
| WAITING_QR | SCREEN_READY | GM861S ativo, buffer circular | AUTHORIZED, IDLE (timeout/rate-limit) |
| AUTHORIZED | QR válido ou código reversa | P1 abre assíncrona, INA219 valida | INSIDE_WAIT, REVERSE_PICKUP, IDLE (timeout), ERROR |
| INSIDE_WAIT | P1 fechou + pessoa dentro | Aguarda balança > min_delivery_g | DELIVERING, ABORTED (P1 abre sem balança) |
| DELIVERING | Balança variou positivo | Aguarda P1 abrir (saída) | DOOR_REOPENED, IDLE (timeout unconventional) |
| DOOR_REOPENED | P1 abriu na saída | Aguarda P1 fechar + mmWave vazio | CONFIRMING, DOOR_ALERT (timeout) |
| CONFIRMING | P1 fechou + corredor vazio | Estabiliza balança, gera JWT | RECEIPT, DELIVERING (P1 abre = segundo pacote) |
| RECEIPT | JWT gerado | Comprovante na tela 120s | IDLE, DELIVERING (P1 abre = novo entregador) |
| ABORTED | Entrou e saiu sem depositar | Log evento, 3s → IDLE | IDLE |
| RESIDENT_P1 | TF9S RESIDENT / HA | P1 aberta aguardando morador entrar | RESIDENT_P2, DOOR_ALERT |
| RESIDENT_P2 | P1 fechou com morador dentro | Delay 2s (cfg.p2_delay_ms), abre P2 | IDLE (P2 abriu e fechou), IDLE (timeout) |
| REVERSE_PICKUP | TF9S REVERSE_* | Coletor dentro, monitora peso saindo | DOOR_REOPENED (P1 abre), IDLE (timeout) |
| DOOR_ALERT | P1 aberta > door_alert_ms | Buzzer contínuo, tela vermelha | IDLE (P1 fecha) |
| ERROR | Falha crítica | Bloqueio total, alerta HA | IDLE (CMD_UNLOCK_P1 do HA) |

---

## 8. ORDEM DE IMPLEMENTAÇÃO

Execute nesta sequência para minimizar erros de dependência:

1. `src/config/Config.h` — adicionar PIN_WIEGAND_D0 (GPIO 6), PIN_WIEGAND_D1 (GPIO 7), remover PIN_INTERNAL_BTN e PIN_ACCESS_CTRL
2. `src/config/ConfigManager.h` — adicionar p2_delay_ms, mudar enable_strike_p2 default para true
3. `src/health/HealthMonitor.h` — adicionar SensorID::WIEGAND antes de _COUNT, rebalancear pesos
4. `src/sensors/Wiegand.h` + `Wiegand.cpp` — módulo completo com ISRs IRAM_ATTR
5. `src/sensors/AccessController.h` + `AccessController.cpp` — validação + NVS + rate limiting
6. `src/middleware/SharedMemory.h` — adicionar AccessGranted struct e wiegand_granted ao PhysicalState
7. `src/fsm/StateMachine.h` — novos estados no enum + novos campos no FsmContext
8. `src/fsm/StateMachine.cpp` — novos handlers + tick() atualizado + _handleAuthorized atualizado
9. `src/comms/UsbBridge.cpp` — CMD_ADD_WIEGAND_CODE, CMD_REMOVE_WIEGAND_CODE, CMD_RESIDENT_UNLOCK, CMD_LIST_WIEGAND_CODES + sendReversePickup()
10. `src/main.cpp` — Wiegand::init() e AccessController no setup(), leitura Wiegand no taskSensorHub, remover PIN_INTERNAL_BTN
11. `host/ui/screens.py` — render_resident_p1(), render_resident_p2(), render_reverse_pickup()
12. `host/main.py` — mapear novos estados para telas
13. `host/integrations/homeassistant.py` — discovery para controles Wiegand + p2_delay
14. `platformio.ini` — sem novas bibliotecas necessárias (Wiegand implementado nativamente)

---

## 9. CHECKLIST DE VERIFICAÇÃO FINAL

Antes de entregar, o agente deve verificar:

- [ ] Nenhum `String` do Arduino em `Wiegand.cpp` ou `AccessController.cpp`
- [ ] ISRs `wiegand_isr_d0` e `wiegand_isr_d1` decoradas com `IRAM_ATTR`
- [ ] `noInterrupts()` / `interrupts()` envolvendo captura dos bits no `poll()`
- [ ] `p2_delay_ms` verificado com subtração uint32_t (não comparação com soma)
- [ ] `enable_strike_p2 = true` no default do ConfigManager
- [ ] `RESIDENT_P1` e `RESIDENT_P2` têm interbloqueio verificando P1/P2 antes de abrir
- [ ] `REVERSE_PICKUP` não acessa `INSIDE_WAIT` — fluxo paralelo independente
- [ ] `AccessController::validate()` consulta NVS com namespace "wieg_db"
- [ ] Rate limiting zera ao reiniciar o ESP32 (em RAM, comportamento esperado)
- [ ] `SensorID::_COUNT` é sempre o último item do enum
- [ ] Soma dos pesos em Config.h (SCORE_*) == 100
- [ ] `taskSensorHub` reseta `wiegand_granted = false` em todo ciclo (flag de um ciclo só)
- [ ] Tela `render_resident_p2()` não bloqueia — usa `math.sin(time.time())` para animação
- [ ] `CMD_RESIDENT_UNLOCK` verifica estado IDLE antes de acionar strike
- [ ] `platformio.ini` continua compilando com `ArduinoJson@7` e `HX711@0.7.5`

---

*rlight.com.br — Portaria Autônoma v7 — Jaboatão dos Guararapes, PE*
