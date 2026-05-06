# rlight v8 — Ideação Completa
## Portaria Autônoma: Auditoria do HA, Arquitetura de Configuração e Roadmap

> *"Feito para funcionar, não para incomodar."*
> O sistema perfeito é aquele que o morador configura pelo celular sem abrir um único arquivo de código.

---

## 1. Auditoria do Home Assistant — Estado Atual

### 1.1 O que está implementado hoje

O HA atua como **Cockpit** do sistema via MQTT. A integração está em `host/integrations/homeassistant.py` e usa MQTT Auto-Discovery — o HA descobre e cria as entidades automaticamente quando o Orange Pi conecta ao broker Mosquitto.

#### Entidades existentes (implementadas)

| Entidade | Tipo HA | Tópico MQTT | Descrição |
|---|---|---|---|
| `sensor.status_portaria` | Sensor | `rlight/portaria/fsm_state` | Estado FSM atual (IDLE, AWAKE, etc.) |
| `sensor.balança_peso` | Sensor (weight, g) | `rlight/portaria/weight_g` | Peso em tempo real da plataforma |
| `sensor.health_score` | Sensor (%) | `rlight/portaria/health_score` | Score 0–100 de saúde dos sensores |
| `sensor.temperatura_chip` | Sensor (°C) | `rlight/portaria/temp_c` | Temperatura interna do ESP32-S3 |
| `sensor.uptime` | Sensor (s) | `rlight/portaria/uptime_s` | Tempo desde o último boot |
| `sensor.último_acesso` | Sensor | `rlight/portaria/last_access` | Tipo e label do último acesso |
| `sensor.strike_p1_ciclos` | Sensor | `rlight/portaria/strike_p1_cycles` | Contador de acionamentos do strike P1 |
| `switch.modo_manutenção` | Switch | `rlight/portaria/cfg` | FSM vai para MAINTENANCE |
| `switch.alarme_loitering` | Switch | `rlight/portaria/cfg` | Alerta de presença prolongada |
| `switch.mmwave_obrigatório` | Switch | `rlight/portaria/cfg` | Exige radar vazio para CONFIRMING |
| `switch.cooler_automático` | Switch | `rlight/portaria/cfg` | Controle térmico via temperatureRead() |
| `switch.strike_p2_habilitado` | Switch | `rlight/portaria/cfg` | Ativa/desativa Strike P2 |
| `button.abrir_p1_remoto` | Button | `rlight/portaria/cmd` | CMD_UNLOCK_P1 com confirmação 2 etapas |
| `button.abrir_morador_remoto` | Button | `rlight/portaria/cmd` | Sequência P1→delay→P2 |
| `button.calibrar_balança` | Button | `rlight/portaria/cmd` | CMD_CALIBRATE_SCALE |
| `button.tarar_balança` | Button | `rlight/portaria/cmd` | CMD_TARE_SCALE |
| `button.testar_strike_p1` | Button | `rlight/portaria/cmd` | Pulso 100ms P1 |
| `button.girar_segredo_jwt` | Button | `rlight/portaria/cmd` | CMD_ROTATE_SECRET |
| `number.delay_p1_p2` | Number (ms) | `rlight/portaria/cfg` | Delay entre P1 fechar e P2 abrir |
| `text.endereço` | Text | `rlight/portaria/set_endereco` | Texto exibido na tela |
| `text.nomes_moradores` | Text | `rlight/portaria/set_nomes` | Nomes exibidos em AWAKE |
| `text.mensagem_erro` | Text | `rlight/portaria/set_msg_erro` | Mensagem customizável na tela ERROR |
| `text.telefone_contato` | Text | `rlight/portaria/set_telefone` | Número exibido no popup de fallback |

#### Automações existentes

- **Alerta por transportadora:** push notification com emoji e peso ao receber `DELIVERY` MQTT
- **Alerta DOOR_ALERT:** push urgente quando P1 aberta > 90s
- **Relatório semanal:** script Python cron segunda-feira 9h com resumo de entregas

#### O que **não está** no HA ainda (gaps identificados)

| Gap | Impacto | Prioridade |
|---|---|---|
| Nenhum controle de timing do strike P1/P2 via HA | Modificar `door_open_ms` exige CMD manual | Alta |
| Sem gestão de senhas do teclado PCF8574 | Cadastrar/revogar acesso exige acesso ao terminal | Crítica |
| Sem controle de timers da FSM (QR timeout, door alert, etc.) | Ajuste comportamental exige reflash | Alta |
| Sem controle de sensibilidade do mmWave por gate | Ajuste fino para ambiente específico impossível | Média |
| Sem janela de horário de funcionamento | Portaria "não tem horário" — abre 24h | Alta |
| Sem controle do LED breathing (período/intensidade) | Feedback visual fixo no firmware | Baixa |
| Sem gestão de códigos de reversa | Adicionar nova transportadora exige código | Alta |
| Sem histórico visual de entregas no HA | Morador vai ao portal para ver histórico | Média |
| Sem controle do Tapo D210 integrado | Ativação manual ou por automação isolada | Média |
| Sem ajuste de rampa do cooler | Temperatura de liga/desliga fixa | Baixa |

---

### 1.2 Arquitetura atual do fluxo de configuração

```
HA Dashboard
    ↓ MQTT publish
Mosquitto Broker (Proxmox LXC)
    ↓
Orange Pi serial_bridge.py
    ↓ USB CDC (JSON + CRC8)
ESP32-S3 UsbBridge::_dispatch()
    ↓
ConfigManager::updateParam()
    ↓ (somente se valor mudou)
NVS Flash
```

Este fluxo é sólido. O problema não é o canal — é que **metade dos parâmetros configuráveis ainda não têm entidade no HA**.

---

## 2. Arquitetura de Configuração v8 — Zero Código

### 2.1 Princípio de design

Toda propriedade do sistema que pode variar por ambiente, preferência ou operação deve ser ajustável **sem compilar, sem SSH, sem terminal**. O morador ajusta pelo celular. O técnico ajusta pelo portaria.rlight.com.br. O código nunca precisa ser tocado após a instalação.

Isso implica três camadas de configuração:

```
┌──────────────────────────────────────────────────────┐
│  Camada 1: Home Assistant                            │
│  Para: Morador no dia a dia                          │
│  Interface: Dashboard mobile / tablet                │
│  Escopo: Ajustes operacionais, senhas, timers        │
├──────────────────────────────────────────────────────┤
│  Camada 2: portaria.rlight.com.br                   │
│  Para: Técnico / morador avançado                    │
│  Interface: Web admin panel (Node.js/Express)        │
│  Escopo: Configurações de segurança, JWT, firmwares  │
├──────────────────────────────────────────────────────┤
│  Camada 3: ESP32 NVS direta                         │
│  Para: Comissionamento inicial apenas                │
│  Interface: Serial Monitor / OTA                     │
│  Escopo: Segredo JWT, cal_factor da balança          │
└──────────────────────────────────────────────────────┘
```

### 2.2 Decisão: HA vs portaria.rlight.com.br

| Critério | Home Assistant | portaria.rlight.com.br |
|---|---|---|
| **Acesso offline** | ✓ Funciona na rede local sem internet | ✗ Requer internet |
| **Integração com automações** | ✓ Aciona push, scripts, Tapo | ✗ Isolado |
| **Histórico e gráficos** | ✓ Recorder nativo | ✓ Banco de entregas próprio |
| **Gestão de usuários** | ✗ Um único "dono" | ✓ Multi-usuário com roles |
| **Controle de senhas de acesso** | ⚠ Sensível — evitar MQTT plain text | ✓ HTTPS + autenticação |
| **Verificação de JWTs** | ✗ Sem suporte nativo | ✓ Endpoint dedicado |
| **Histórico de entregas com foto** | ✗ Sem suporte a imagens | ✓ Nativo |
| **Configuração de segurança crítica** | ⚠ Não recomendado | ✓ Camada correta |

**Conclusão:**
- **HA** → configurações operacionais e de comportamento (timers, feedback visual, toggles de feature)
- **portaria.rlight.com.br** → gestão de senhas, segurança, histórico com fotos, verificação de JWT, configurações críticas
- **Tudo que puder estar em ambos, deve estar em ambos** com sincronização via API

---

## 3. Mapa Completo de Configurações v8

### Categoria A — Controle de Atuadores (Hardware)

#### A1. Timing do Strike P1

```yaml
# HA: number entity
name: "Pulso Strike P1 (ms)"
key: door_open_ms
min: 500
max: 10000
step: 100
default: 3000
unit: ms
```

**Por que importa:** Mola aérea diferente exige pulso diferente. P1 com mola pesada pode precisar de 5000ms. Sem ajuste via HA, o instalador precisa reflashar para calibrar.

**Nota de segurança:** Valor máximo hardcoded de 10s no ConfigManager. Acima disso, o booleano `_strike_armed` já reseta o pino — proteção contra travamento de bobina.

#### A2. Timing do Strike P2

```yaml
name: "Pulso Strike P2 (ms)"
key: p2_open_ms   # novo parâmetro separado do door_open_ms
min: 500
max: 8000
step: 100
default: 3000
```

**Separado de P1 porque:** P2 é uma porta interna, geralmente mais leve, pode precisar de pulso diferente. Usar `door_open_ms` para ambas era uma simplificação.

#### A3. Delay P1→P2 (anti-queda de tensão)

```yaml
name: "Delay P1 → P2 (ms)"
key: p2_delay_ms
min: 500
max: 5000
step: 100
default: 2000
```

#### A4. Pulso da botoeira do portão

```yaml
name: "Pulso Botoeira Portão (ms)"
key: gate_relay_ms
min: 200
max: 3000
step: 50
default: 500
unit: ms
```

**Contexto:** O relé que fecha contato da botoeira do motor de portão. Cada motor tem tempo mínimo de pulso diferente (PPA, Garen, Nice, etc.).

#### A5. Rampa do cooler (controle fino)

```yaml
# Dois parâmetros separados:
name: "Cooler: temperatura de início (°C)"
key: cooler_temp_min_c
min: 35
max: 60
step: 1
default: 45

name: "Cooler: temperatura máxima (°C)"
key: cooler_temp_max_c
min: 50
max: 80
step: 1
default: 70
```

**Ajuste avançado:** Em Jaboatão no verão (45°C ambiente), pode ser necessário ligar o cooler a partir de 38°C para manter o HX711 estável. Em inverno, pode elevar para 55°C.

#### A6. Controle do Tapo via buck PWM

```yaml
name: "Tapo: duty cycle do buck (%)"
key: tapo_buck_duty
min: 0
max: 100
step: 5
default: 75
unit: "%"

name: "Tapo: tempo de ativação por evento (s)"
key: tapo_active_duration_s
min: 10
max: 300
step: 10
default: 60
unit: s
```

**Contexto:** O videoporteiro Tapo é alimentado por um buck converter controlado por PWM do ESP32. Desligar quando não há atividade reduz consumo e aquecimento. Liga automaticamente em DOOR_ALERT, AWAKE e detecção de loitering.

**Nota técnica:** Adicionar GPIO para controle do buck na próxima versão do Config.h. Sugestão: `PIN_TAPO_BUCK` em um GPIO PWM livre.

---

### Categoria B — Timers de Alerta e FSM

#### B1. Timers da FSM (todos editáveis)

```yaml
# AWAKE sem ação → IDLE
name: "Timeout: modo entregador (s)"
key: awake_timeout_ms  # dividir por 1000 na UI
min: 15
max: 120
step: 5
default: 60

# WAITING_QR sem leitura
name: "Timeout: leitura QR (s)"
key: qr_timeout_ms
min: 10
max: 60
step: 5
default: 30

# P1 aberta → DOOR_ALERT
name: "Alerta porta P1 aberta (s)"
key: door_alert_ms
min: 30
max: 300
step: 10
default: 90

# P2 aberta sem autorização → alerta
name: "Alerta porta P2 aberta (s)"
key: p2_alert_ms      # novo parâmetro
min: 30
max: 300
step: 10
default: 60

# Portão aberto sem autorização → alerta
name: "Alerta portão aberto (s)"
key: gate_alert_ms    # novo parâmetro
min: 30
max: 600
step: 30
default: 120

# INSIDE_WAIT sem balança variar
name: "Timeout: espera de depósito (s)"
key: inside_timeout_ms
min: 60
max: 600
step: 30
default: 180

# AUTHORIZED sem P1 abrir
name: "Timeout: autorização sem empurrar (s)"
key: authorized_timeout_ms
min: 5
max: 30
step: 1
default: 10

# Janela do botão P2 após acesso concedido pelo teclado
name: "Janela de acesso P2 (s)"
key: p2_access_window_ms  # dividir por 1000 na UI
min: 10
max: 120
step: 5
default: 30
```

#### B2. Janela de funcionamento da portaria

```yaml
name: "Horário de início (h)"
key: operating_start_h
min: 0
max: 23
step: 1
default: 7

name: "Horário de fim (h)"
key: operating_end_h
min: 0
max: 23
step: 1
default: 22

switch: "Portaria 24 horas"
key: always_open
default: true
```

**Comportamento quando fora do horário:**
- Estado FSM: `OUT_OF_HOURS` (novo estado — ver Seção 5)
- Tela exibe: mensagem customizável (ex: "Portaria em funcionamento das 7h às 22h")
- QR Code e teclado desativados
- HA ainda recebe heartbeats e pode abrir remotamente (morador com chave tem prioridade)
- DOOR_ALERT continua ativo (segurança não tem horário)

**Nota:** Verificado usando o timestamp do DS3231 extrapolado no FsmContext.

#### B3. Spatial debouncing do mmWave

```yaml
name: "Debounce presença mmWave (ms)"
key: radar_debounce_ms
min: 500
max: 5000
step: 100
default: 1500
```

---

### Categoria C — Sensores e Calibração

#### C1. Balança — ajuste fino completo

```yaml
# Tara manual
button: "Tarar balança (peso zero)"
cmd: CMD_TARE_SCALE

# Calibração com peso conhecido
number: "Peso de referência para calibração (g)"
key: cal_weight_g
min: 100
max: 30000
step: 10
unit: g

button: "Calibrar com peso acima"
cmd: CMD_CALIBRATE_SCALE
args: {weight_g: cal_weight_g}

# Threshold de entrega
number: "Peso mínimo de entrega (g)"
key: min_delivery_weight_g
min: 20
max: 500
step: 10
default: 50

# Auto-zero drift máximo
number: "Drift máximo para auto-zero (g)"
key: auto_zero_max_drift_g
min: 5
max: 200
step: 5
default: 50

# Intervalo de auto-zero
number: "Intervalo de auto-zero (min)"
key: auto_zero_interval_ms  # dividir por 60000 na UI
min: 5
max: 120
step: 5
default: 10
```

#### C2. mmWave — sensibilidade por gate

O LD2410B tem configuração de sensibilidade por "gate" (faixas de distância de 0,75m cada). Configurar via HA permite ajustar sem firmware:

```yaml
# Para cada gate 0–8:
name: "mmWave Gate {n}: sensibilidade movimento"
key: mmwave_gate_{n}_motion
min: 0
max: 100
step: 5
default: 50

name: "mmWave Gate {n}: sensibilidade estática"
key: mmwave_gate_{n}_static
min: 0
max: 100
step: 5
default: 50
```

**Na prática:** Gates 0–2 (0–1,5m) são os mais relevantes para o corredor de ~1m. Reduzir sensibilidade nos gates 3+ evita falsos positivos de vegetação ou chuva na fachada.

**Implementação:** `CMD_UPDATE_MMWAVE_GATE` envia configuração via USB → ESP32 repassa via UART1 para o LD2410B usando o protocolo de engenharia do sensor (estrutura de 18 bytes documentada no datasheet).

#### C3. INA219 — thresholds de corrente

```yaml
number: "Corrente mínima strike P1 (mA)"
key: ina_strike_min_ma
min: 100
max: 800
step: 25
default: 400

number: "Corrente máxima strike P1 (mA)"
key: ina_strike_max_ma
min: 500
max: 2000
step: 50
default: 1200

# Alerta de desgaste preventivo
number: "Alerta de ciclos do strike P1"
key: strike_cycle_alert
min: 10000
max: 200000
step: 5000
default: 80000
```

---

### Categoria D — Controle de Acesso e Senhas

> ⚠️ **Senhas trafegam por MQTT com criptografia TLS.** Configurar Mosquitto com TLS antes de habilitar gestão de senhas pelo HA. Alternativa mais segura: usar exclusivamente o portaria.rlight.com.br para gestão de senhas.

#### D1. Senhas do teclado PCF8574

```yaml
# Senha mestre do morador (único slot residente por padrão)
# Armazenada no NVS como hash BCrypt-like via mbedtls SHA-256
text: "Senha do morador principal"
key: pass_resident_1
type: password
max_length: 8
pattern: "[0-9]+"  # apenas dígitos

# Senha temporária de logística reversa (com validade)
text: "Senha reversa temporária"
key: pass_reverse_temp
type: password
max_length: 6

datetime: "Validade da senha temporária"
key: pass_reverse_temp_expires
```

**Gerenciamento avançado de senhas** (portaria.rlight.com.br):
- Lista de senhas por tipo (RESIDENT, REVERSE_ML, REVERSE_CORREIOS, DELIVERY_ONLY)
- Validade por data/hora ou número de usos
- Log de uso de cada senha
- Geração de senha temporária com QR Code para enviar por WhatsApp

#### D2. QR Code — modo de autorização

```yaml
select: "Modo de autorização QR Code"
key: qr_auth_mode
options:
  - "QUALQUER_CODIGO"       # qualquer código válido abre (padrão atual)
  - "APENAS_TRANSPORTADORAS" # filtra por carrier identificado
  - "LISTA_BRANCA"          # somente códigos pré-cadastrados
default: QUALQUER_CODIGO

# Se modo LISTA_BRANCA:
# Gerenciado exclusivamente pelo portaria.rlight.com.br
# (lista de prefixos/códigos autorizados sincronizada para NVS)
```

**Segurança progressiva:**
- Hoje: qualquer código de barras de tamanho válido abre (modo mais operacional)
- Futuro próximo: apenas transportadoras identificadas
- Futuro avançado: lista branca de prefixos por contrato com transportadora

#### D3. Rate limiting do teclado

```yaml
number: "Tentativas inválidas antes do bloqueio"
key: keypad_max_fails
min: 3
max: 10
step: 1
default: 5

number: "Tempo de bloqueio após tentativas (min)"
key: keypad_lockout_min
min: 1
max: 60
step: 1
default: 10
```

---

### Categoria E — Feedback Visual e Sonoro

#### E1. LED do botão — efeito de breathing

```yaml
number: "LED botão: período do breathing (ms)"
key: led_btn_breathe_ms
min: 500
max: 5000
step: 100
default: 2000

number: "LED botão: brilho máximo (0–255)"
key: led_btn_brightness_max
min: 50
max: 255
step: 5
default: 200

number: "LED QR: brilho durante leitura (0–255)"
key: led_qr_brightness
min: 50
max: 255
step: 5
default: 200
```

**Utilidade:** Em ambientes muito escuros (portaria sem luz externa), o LED forte pode ser desconfortável. Em ambientes claros, pode precisar de brilho máximo para ser visível.

#### E2. Buzzer — padrões configuráveis

```yaml
switch: "Buzzer habilitado"
key: buzzer_enabled
default: true

number: "Volume do buzzer: duração do bipe (ms)"
key: buzzer_beep_ms
min: 50
max: 500
step: 25
default: 100

# Silenciar buzzer à noite (usa horário operacional)
switch: "Silenciar fora do horário"
key: buzzer_silent_offhours
default: false
```

#### E3. Texto da tela — dinâmico

```yaml
text: "Endereço exibido na tela"
key: screen_address
max_length: 80

text: "Nomes dos moradores"
key: screen_residents
max_length: 60

text: "Mensagem de porta aberta"
key: screen_door_alert_msg
max_length: 100
default: "FECHE A PORTA IMEDIATAMENTE"

text: "Mensagem fora do horário"
key: screen_offhours_msg
max_length: 100
default: "Portaria em funcionamento das 7h às 22h"

text: "Telefone de contato"
key: screen_phone
max_length: 20
```

---

### Categoria F — Segurança Avançada (portaria.rlight.com.br preferencial)

#### F1. Segredo JWT

```yaml
# Via HA (requer MQTT TLS):
button: "Rotacionar segredo JWT"
cmd: CMD_ROTATE_SECRET

# Via portaria.rlight.com.br:
# Interface segura com confirmação por senha do painel
# Exibe data da última rotação, não o segredo em si
```

#### F2. Secure Boot e OTA

```yaml
# Status apenas (não configurável via HA por segurança):
sensor: "Versão firmware ESP32"
sensor: "Hash do firmware ativo"

# OTA:
button: "Verificar e aplicar atualização OTA"
# Orange Pi busca em /var/lib/rlight/firmware.bin
# Compara SHA256 com versão atual antes de flashar
```

#### F3. Log de auditoria

```yaml
# HA mostra resumo; portaria.rlight.com.br mostra log completo
sensor: "Último evento de segurança"
sensor: "Tentativas de acesso negadas (24h)"
sensor: "Último acesso de morador"
```

---

## 4. Dashboard HA — Estrutura Sugerida

```
┌─────────────────────────────────────────────────────┐
│  🏠 rlight Portaria  ●  ONLINE  14:32  Score: 97%  │
├──────────────┬──────────────────────────────────────┤
│  STATUS      │  ÚLTIMAS ENTREGAS                    │
│  ──────────  │  ──────────────────────────────────  │
│  IDLE        │  14:22  SHOPEE   1.24kg  ✓ Sync      │
│  P1: ●Fech   │  11:05  ML       0.45kg  ✓ Sync      │
│  P2: ●Fech   │  Ontem  CORREIOS 2.10kg  ✓ Sync      │
│  Portão: ●F  │                                      │
│  Balança: 0g │  📊 Histórico completo →             │
├──────────────┼──────────────────────────────────────┤
│  AÇÕES       │  SAÚDE DOS SENSORES                  │
│  ──────────  │  ──────────────────────────────────  │
│  [Abrir P1]  │  QR Reader   ●●●●●  100%            │
│  [Morador]   │  Balança HX711 ●●●●●  100%          │
│  [Manu.]     │  mmWave      ●●●●●  100%             │
│              │  INA219 P1   ●●●●●  100%             │
│              │  Teclado     ●●●●●  100%             │
├──────────────┴──────────────────────────────────────┤
│  CONFIGURAÇÕES RÁPIDAS                              │
│  Pulso P1: [──────●──] 3000ms                      │
│  Pulso P2: [──●────────] 3000ms                    │
│  Timeout QR: [────●────] 30s                       │
│  Modo QR: [QUALQUER_CODIGO ▾]                      │
│  Horário: [07:00] até [22:00]  [●] Ativo           │
└─────────────────────────────────────────────────────┘
```

---

## 5. Novos Estados FSM para v8

### 5.1 `OUT_OF_HOURS` (novo)

| Campo | Valor |
|---|---|
| Ativado por | Timestamp fora de `[operating_start_h, operating_end_h]` quando `always_open = false` |
| Comportamento | QR/teclado desabilitados. Tela mostra `screen_offhours_msg`. DOOR_ALERT ainda ativo |
| Prioridade | Abaixo de MAINTENANCE e DOOR_ALERT |
| Saída | Horário entra no intervalo OU `CMD_UNLOCK_P1` do HA |
| Tela | Background âmbar suave. Relógio grande centralizado. Mensagem customizável |

### 5.2 `WAITING_PASS` (novo — já referenciado no README v7/v8)

| Campo | Valor |
|---|---|
| Ativado por | Tecla pressionada no PCF8574 em IDLE ou AWAKE |
| Comportamento | Exibe `****` (asteriscos) ocultando quantidade real. Aceita até 8 dígitos. Confirma com `#`. Limpa com `*` |
| Tela | Campo de senha centralizado, pulsante. Feedback imediato via WebSocket (asterisco aparece ao pressionar) |
| Timeout | `cfg.qr_timeout_ms` segundos sem tecla → IDLE |
| Sucesso | Senha válida → mesmo fluxo do QR (AUTHORIZED ou RESIDENT_P1) |

**Lógica de detecção de tipo de senha:**
```
Senha 1111# → lookup NVS "pass_1111_sha" → tipo RESIDENT → fluxo RESIDENT_P1
Senha 9999# → lookup NVS "pass_9999_sha" → tipo REVERSE_ML → fluxo AUTHORIZED
Senha XXXX# → não encontrado → fail_count++ → se >= max_fails → LOCKOUT
```

**Hash de senha no NVS:**
A senha nunca é armazenada em texto plano. `SHA-256("senha" + "salt_fixo_nvs")` é o que está no NVS. A comparação é feita com o hash da entrada.

### 5.3 `LOCKOUT_KEYPAD` (novo)

| Campo | Valor |
|---|---|
| Ativado por | `keypad_fail_count >= cfg.keypad_max_fails` |
| Comportamento | Teclado desabilitado por `cfg.keypad_lockout_min` minutos. Alerta MQTT urgente. Tapo ativa |
| Tela | Conta-regressiva do tempo de bloqueio. Instrução para usar interfone |
| Saída | Timer expira OU `CMD_RESET_KEYPAD_LOCKOUT` do HA |

---

## 6. Novos Comandos USB v8

Adições ao `UsbBridge::_dispatch()`:

```cpp
// Configuração de gate do mmWave
CMD_UPDATE_MMWAVE_GATE  { gate: 0-8, motion: 0-100, static: 0-100 }

// Gestão de senhas (hash SHA-256, nunca plain text)
CMD_ADD_PASSWORD        { hash: "abc123...", type: "RESIDENT|REVERSE_ML|..." }
CMD_REMOVE_PASSWORD     { hash: "abc123..." }
CMD_LIST_PASSWORDS      {}  // retorna hashes e tipos, nunca senhas

// Gestão de horário de funcionamento
CMD_SET_OPERATING_HOURS { start_h: 7, end_h: 22 }
CMD_SET_ALWAYS_OPEN     { value: true|false }

// Controle do Tapo buck
CMD_SET_TAPO_BUCK       { duty: 0-100 }
CMD_TAPO_ACTIVATE       { duration_s: 60 }

// Reset de lockout
CMD_RESET_KEYPAD_LOCKOUT {}

// Configuração de senhas separada (alta segurança)
CMD_SET_P1_PULSE        { ms: 3000 }   // alias de CMD_UPDATE_CFG door_open_ms
CMD_SET_P2_PULSE        { ms: 3000 }   // novo parâmetro p2_open_ms
CMD_SET_GATE_RELAY_PULSE { ms: 500 }   // novo parâmetro gate_relay_ms
```

---

## 7. portaria.rlight.com.br — Escopo Definitivo v8

O portal web é a camada de maior confiança do sistema. Roda em Oracle Cloud Always Free (ARM Ampere A1, 4 OCPUs, 24GB RAM — mais que suficiente). Node.js/Express + NGINX + SSL Let's Encrypt.

### 7.1 Funcionalidades exclusivas do portal (não replicar no HA)

| Funcionalidade | Justificativa |
|---|---|
| **Gestão completa de senhas** | Senhas nunca devem trafegar por MQTT sem TLS. O portal usa HTTPS |
| **Verificação pública de JWT** | `GET /verify/:token` — qualquer pessoa pode confirmar autenticidade de um comprovante |
| **Histórico com fotos** | HA não tem galeria de imagens nativa. Portal exibe foto + JWT + peso por entrega |
| **Log de auditoria completo** | Todas as transições de estado com duração, todos os acessos, todos os alertas |
| **Configuração de Secure Boot** | Operação irreversível que exige confirmação explícita |
| **OTA de firmware** | Upload do binário assinado, SHA256 verificado antes de aplicar |
| **Rotação de segredo JWT com confirmação** | Operação sensível que exige senha do portal |
| **Multi-usuário** | Convidar outros moradores com permissões diferenciadas |
| **Relatórios e analytics** | Gráficos de frequência por transportadora, horários de pico, tendência de peso |
| **API pública documentada** | Para integração com outros sistemas (Notion, Google Sheets, etc.) |

### 7.2 Funcionalidades em ambos (sincronizadas)

| Funcionalidade | HA | Portal | Sincronização |
|---|---|---|---|
| Status em tempo real | ✓ | ✓ | WebSocket → Portal; MQTT → HA |
| Últimas 10 entregas | ✓ (resumo) | ✓ (completo) | API REST do OPi |
| Abrir P1 remotamente | ✓ | ✓ | Ambos enviam MQTT |
| Modo manutenção | ✓ | ✓ | MQTT bidirecional |
| Score de saúde | ✓ | ✓ | Heartbeat MQTT |

---

## 8. Mapa de Implementação v8

### 8.1 Prioridade crítica (bloqueia operação)

- [ ] Implementar `WAITING_PASS` na FSM com hash SHA-256 de senha
- [ ] `CMD_ADD_PASSWORD` / `CMD_REMOVE_PASSWORD` no UsbBridge
- [ ] Gestão de senhas no portaria.rlight.com.br (HTTPS obrigatório)
- [ ] Parâmetros `p2_open_ms` e `gate_relay_ms` no ConfigManager
- [ ] GPIO e lógica para relé da botoeira do portão no ESP32

### 8.2 Alta prioridade (operação completa)

- [ ] Estado `OUT_OF_HOURS` na FSM
- [ ] Estado `LOCKOUT_KEYPAD` na FSM
- [ ] `CMD_UPDATE_MMWAVE_GATE` com protocolo de engenharia do LD2410B
- [ ] Todas as entidades HA das categorias A–E (entities completas)
- [ ] Tela `WAITING_PASS` na UI web com feedback via WebSocket
- [ ] Tela `OUT_OF_HOURS` na UI web

### 8.3 Média prioridade (polimento)

- [ ] Controle do Tapo buck via GPIO + PWM
- [ ] Hash de senha com salt no NVS (segurança da v7 era plain text)
- [ ] Relatórios e analytics no portal
- [ ] OTA de firmware via Orange Pi
- [ ] Multi-usuário no portal

### 8.4 Baixa prioridade (luxo)

- [ ] Controle de breathing do LED via HA (ledcWrite dinâmico)
- [ ] Silenciar buzzer fora do horário
- [ ] Modo Claro/Escuro adaptativo na UI (já tem base)
- [ ] API pública documentada com Swagger

---

## 9. Configurações que Faltaram na Sua Lista

Além do que você listou, estas são configurações que elevam o projeto ao nível profissional:

### 9.1 Notificações push configuráveis

```yaml
switch: "Notificar entrega"
switch: "Notificar porta aberta"
switch: "Notificar acesso negado"
switch: "Notificar loitering"
number: "Peso mínimo para notificar entrega (g)"  # evita spam de envelopes
select: "Canal de notificação"  # Push HA / Telegram / WhatsApp Business API
```

### 9.2 Modo "Férias"

```yaml
switch: "Modo férias"
text: "Mensagem da tela em férias"
switch: "Aceitar entregas em férias"  # QR ainda funciona, morador não notificado
number: "Peso máximo sem notificar (kg)"  # pequenos pacotes passam quietos
```

### 9.3 Automação por peso

```yaml
number: "Peso: alerta de item pesado (kg)"
# Se pacote > X kg: notificação prioritária "item pesado, talvez frágil ou valioso"
default: 10.0
```

### 9.4 Diagnóstico e manutenção preventiva

```yaml
number: "Ciclos do strike antes do alerta (unidades)"
switch: "Alertar sobre tendência de desgaste do strike (INA219 trend)"
number: "Score mínimo de saúde antes de entrar em modo degradado"
# Abaixo desse score: alerta no HA mas sistema continua operando
default: 60
```

### 9.5 Comportamento da câmera

```yaml
number: "Frames descartados para calibrar câmera"
# Ambientes com mudança brusca de luz (sol/sombra) precisam de mais frames
min: 3
max: 15
default: 5

switch: "Salvar foto em disco (SD) para cada entrega"
switch: "Enviar foto para Oracle Cloud"
# Permite desabilitar o upload se internet cara/limitada
```

### 9.6 Configuração do portão (novo em v8)

```yaml
switch: "Portão: fechar automaticamente após entrada"
number: "Portão: delay para fechar após entrada (s)"
switch: "Portão: alerta se aberto sozinho (sem acionamento pelo sistema)"
# Reed switch do portão detecta abertura não autorizada
number: "Portão: timeout de alerta (s)"
```

---

## 10. Resumo Executivo — O que a v8 entrega

| Dimensão | v7 (hoje) | v8 (target) |
|---|---|---|
| **Configurações via HA** | ~22 entidades | ~65 entidades |
| **Configurações via portal** | JWT, histórico | + Senhas, OTA, analytics, multi-user |
| **Estados FSM** | 16 | 19 (+ OUT_OF_HOURS, WAITING_PASS, LOCKOUT_KEYPAD) |
| **Comandos USB** | ~12 | ~22 |
| **Parâmetros NVS** | ~18 chaves | ~32 chaves |
| **Gestão de acesso** | NVS manual | HA + portal com hash SHA-256 |
| **Segurança de senhas** | Plain text NVS | SHA-256 com salt |
| **Horário de funcionamento** | Não existe | Configurável com estado dedicado |
| **Controle visual** | Fixo no firmware | Brightness, breathing, buzzer via HA |
| **mmWave** | Threshold único | Sensibilidade por gate (9 faixas) |
| **Portão** | Relé simples | Integração completa com reed switch e alertas |

---

*rlight.com.br · v8 Ideation Document · 2026*
*"A portaria que você nunca precisa abrir o código para ajustar."*