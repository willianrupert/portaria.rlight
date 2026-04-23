# rlight — Histórico Completo de Versões
**Portaria Autônoma Inteligente · Jaboatão dos Guararapes, PE, Brasil**

> *"A entrega não termina quando o entregador vai embora. Ela termina quando o morador tem a certeza — criptograficamente verificável — de que o pacote está seguro."*

---

## Convenções deste documento

Este changelog segue o padrão [Keep a Changelog](https://keepachangelog.com/pt-BR/1.0.0/) adaptado para projetos de hardware+firmware+software. As seções de cada versão são:

- **Added** — novos recursos, módulos, sensores ou integrações
- **Changed** — modificações em comportamento, arquitetura ou hardware existente
- **Fixed** — correções de bugs, falhas de hardware ou lógica incorreta
- **Removed** — componentes, estados ou funcionalidades deliberadamente removidos
- **Security** — mudanças com impacto direto em segurança física ou criptográfica
- **Hardware** — decisões de componentes físicos, pinagem e eletrônica
- **Notes** — contexto de engenharia, raciocínio por trás de decisões

Versões de firmware do ESP32-S3, software do Orange Pi e interface web são numeradas em conjunto (o sistema é indivisível — uma versão engloba todas as camadas).

---

## [v8.0.0] — Portaria na Nuvem, Criptografia e Design Premium
**Data:** 23 de Abril de 2026
**Codinome:** *Ilha do Leite*
**Foco:** Segurança de ponta a ponta, experiência visual Apple-grade, e infraestrutura de nuvem.

### Added
- **Assinatura criptográfica JWT real com mbedTLS:** `JwtSigner.cpp` migrado de stub simulado para implementação completa usando `mbedtls_md_hmac` com `MBEDTLS_MD_SHA256`. O acelerador de hardware criptográfico nativo do ESP32-S3 é usado — sem biblioteca externa. O segredo reside no NVS com proteção eFuse. Zero trânsito do segredo fora do chip.
- **Rotação de segredo JWT sem downtime (S19):** Dois slots NVS simultâneos (`secret_current` e `secret_previous`). Comando `CMD_ROTATE_SECRET` via HA promove o slot atual para o anterior e gera novo segredo via `esp_random()` (TRNG de hardware). O servidor verifica com `current` primeiro, depois `previous`. Morador pode rotar anualmente sem abrir a caixa.
- **Backend Oracle Cloud Always Free:** Servidor Node.js/Express com NGINX reverse proxy e SSL/TLS via Let's Encrypt. Endpoint `POST /api/deliveries` recebe JWT + foto + metadados. Endpoint `GET /api/verify/:token` permite verificação pública de comprovantes. Custo: R$0.
- **Interface Web SPA completa — Glassmorphism Apple-style:** Migração total de pygame para aplicação React com Framer Motion. Resolução alvo 1024×600px, fullscreen kiosk mode. 16 telas mapeadas para os 16 estados FSM, cada uma com animações de entrada/saída e feedback visual específico. Fundo com blobs animados e `backdrop-filter: blur(40px)`. Tipografia SF Pro Display.
- **HUD global persistente:** Header fixo em todas as telas com relógio `HH:MM:SS` (atualização por segundo, separadores `:` piscando), data formatada em português, e bolinha de status online (verde `#30D158` com glow = online, laranja = degradado score < 80, vermelho = offline > 90s sem heartbeat).
- **Suporte iPhone 16 Pro Max:** Safe areas, viewport meta tags, splash screen adaptativo para acesso móvel à interface de monitoramento remoto.
- **Modo Claro/Escuro adaptativo:** A interface detecta `prefers-color-scheme` e ajusta a paleta automaticamente. No modo claro, os blobs de fundo ganham intensidade reduzida e o texto inverte para `#080810`.
- **Snapshot de webcam via OpenCV em RAM:** `camera_usb.py` migrado de `fswebcam` subprocess para `cv2.VideoCapture`. Captura 1080p descartando 5 frames iniciais de calibração do sensor. Frame JPEG e Surface pygame/canvas carregados em memória — zero write no MicroSD para fotos temporárias.
- **Foto vinculada ao token JWT por UUID (S8):** O arquivo de foto é nomeado `{token}.jpg` onde `token` é o UUID gerado pelo `esp_random()` do ESP32-S3. Elimina o risco de associação incorreta de foto e comprovante em ciclos rápidos consecutivos.
- **API local FastAPI porta 8080 (S10):**
  - `GET /entregas?limit=10` — últimas entregas do SD
  - `GET /ml/features?limit=1000` — features estruturadas para ML
  - `GET /status` — heartbeat mais recente da RAM
  - `GET /alerts?limit=20` — alertas críticos do SD
  - `GET /photos/{token}` — servir fotos para a interface web
- **Tabela `delivery_features` para ML/IA (S15):** Campos calculados em cada entrega para treinar modelos futuros: `ts`, `carrier`, `weight_g`, `duration_inside_s`, `door_open_count`, `mmwave_present_pct`, `scale_settle_variance`, `unconventional`, `time_of_day_h`, `day_of_week`. Dados acumulados ao longo de meses permitem: prever janela de entrega por transportadora, detectar anomalias de comportamento, otimizar thresholds da balança por sazonalidade.
- **Rate limiting de tentativas de QR (S20):** 5 leituras de código `INVALID` em 10 minutos → cooldown de 5 minutos + alerta MQTT + Tapo em modo sensível. Previne tentativas sistemáticas de brute-force com códigos de barras aleatórios.
- **Popup de fallback em WAITING_QR:** Após 15s sem leitura, popup glassmorphism sobe do rodapé com número de telefone configurável via HA e botão de acionamento da campainha Tapo. Permanece sobreposto sem substituir a tela — o timer e o sensor continuam ativos.
- **Telas para estados anteriormente sem representação visual:** `WAITING_QR` (separada de AWAKE), `INSIDE_WAIT` (com indicador de peso em tempo real), `DOOR_REOPENED` (indicador mmWave presença/vazio), `ABORTED` (3s neutro), `MAINTENANCE` (grid de saúde dos sensores), `DOOR_ALERT` (separada de ERROR com resolução automática).
- **Componentes React reutilizáveis:** `<StatusHeader>`, `<ProgressBar type="linear|circular">`, `<ActionArrow direction="...">`, `<WeightDisplay live>`, `<GlassCard>`, `<Pill color="...">`, `<FallbackPopup>`.
- **Watchdog Python via sdnotify (S9):** `WatchdogSec=10` no arquivo `.service` do systemd. O loop principal notifica `WATCHDOG=1` a cada ciclo. Processo travado é detectado e reiniciado em 3s automaticamente.
- **Sync worker com backoff exponencial (S7):** Thread dedicada processa fila `synced=0` do banco de entregas. Backoff: 1s → 2s → 4s → ... → 1800s (30min). Reseta ao primeiro upload bem-sucedido. Nenhuma entrega é perdida mesmo com dias de internet instável.
- **Abertura remota P1 com confirmação em duas etapas no HA (S12):** Primeiro toque ativa `input_boolean.unlock_pending`. Segundo toque dentro de 10s confirma e envia `CMD_UNLOCK_P1`. Sem confirmação em 10s → cancela automaticamente. Previne abertura acidental por toque em dispositivo móvel.

### Changed
- **Interface completamente reescrita:** De pygame (`kmsdrm`, sem X11) para React SPA servida pelo OPi via servidor local. O pygame era funcional mas limitava animações, layouts responsivos e manutenção de código.
- **`enable_strike_p2` default alterado para `true`:** Hardware P2 instalado fisicamente. O toggle continua disponível via HA para manutenção preventiva sem intervenção física.
- **Blobs de fundo adaptam cor ao estado:** Verde/azul em operação normal. Vermelho/laranja em DOOR_ALERT. Vermelho escuro em ERROR. Âmbar em MAINTENANCE. Transição suave de 1.2s.

### Security
- **JWT com timestamp extrapolado:** `ts = unix_time + ((millis() - millis_at_sync) / 1000)`. Entregas dentro da mesma janela de 10min entre sincronizações DS3231 têm timestamps precisos, não o momento da última sincronização.
- **Sanity check de timestamp (correção #17):** `CMD_SYNC_TIME` com `ts < 1700000000UL` (Novembro de 2023) descartado silenciosamente. Flag `system_time_invalid` ativada. Previne JWTs com data de ano 2000 quando bateria CR2032 do DS3231 morre.

### Hardware
- **Webcam USB 1080p:** Migração de `fswebcam` para OpenCV direto. Acesso ao `/dev/video0` via `cv2.VideoCapture`. Descarte dos primeiros 5 frames de calibração automática de brilho do sensor CMOS.

---

## [v7.0.0] — Controle de Acesso Wiegand, AMP FreeRTOS e Arquitetura Mission-Critical
**Data:** Março–Abril de 2026
**Codinome:** *Pina*
**Foco:** Arquitectura de dois núcleos, acesso de moradores via hardware dedicado, coleta reversa.

### Added
- **Módulo Wiegand completo (`Wiegand.h/cpp`):** Decodificação de frames via ISRs `IRAM_ATTR` nos pinos GPIO 6 (D0) e GPIO 7 (D1). Protocolo Wiegand 26 bits (padrão) com suporte a 34/37 bits alternativos. Captura atômica com `noInterrupts()`/`interrupts()`. Timeout de 25ms para detecção de fim de frame. Zero bloqueio do Core 1.
- **`AccessController` com banco de códigos na NVS:** Namespace `wieg_db`. Chave `wieg_{codigo_decimal}` → valor `"TIPO:Label"` (ex: `"RESIDENT:Willian Rupert"` ou `"REVERSE_ML:Coleta ML"`). Comandos `CMD_ADD_WIEGAND_CODE`, `CMD_REMOVE_WIEGAND_CODE`, `CMD_LIST_WIEGAND_CODES` via HA. Rate limiting: 5 tentativas inválidas em 5 minutos → cooldown de 10 minutos.
- **Controle de acesso TF9S integrado:** Módulo com teclado de senha, leitor de cartão IC/RFID e digital. Interface Wiegand D0/D1. Moradores acessam a casa via senha, cartão ou digital — sem app, sem celular, sem dependência de rede.
- **Tipos de acesso com semântica distinta:**
  - `RESIDENT` → abre P1 + delay de 2s + abre P2
  - `REVERSE_CORREIOS`, `REVERSE_ML`, `REVERSE_AMAZON`, `REVERSE_GENERIC` → apenas P1, fluxo de coleta reversa
- **Dois novos estados FSM: `RESIDENT_P1` e `RESIDENT_P2`:**
  - `RESIDENT_P1`: P1 aberta, morador deve entrar e deixar fechar. Exibe nome do morador na tela.
  - `RESIDENT_P2`: P1 fechou, delay de `cfg.p2_delay_ms` (padrão 2000ms) antes de P2 abrir — evita queda de tensão por dois strikes simultâneos. Progresso circular animado.
- **Novo estado FSM: `REVERSE_PICKUP`:** Coletor de reversa dentro do corredor. Balança monitora variação negativa (objeto sendo retirado). JWT com `"direction": "OUTBOUND"` e transportadora identificada. Notificação push ao morador.
- **Delay configurável entre P1 e P2 (`p2_delay_ms`):** Padrão 2000ms. Configurável via HA de 500ms a 5000ms. Previne queda de tensão do barramento 12V ao acionar dois solenoides em sequência rápida.
- **`CMD_RESIDENT_UNLOCK` via HA:** Sequência idêntica ao TF9S — P1 → delay 2s → P2. Substituição do antigo `CMD_UNLOCK_P2` direto por uma sequência segura e com interbloqueio verificado.
- **Proteção contra duplo acionamento do strike (S1):** Flag `_strike_armed` na classe `Strike`. `open()` retorna imediatamente se `_strike_armed = true`. Reseta em `tick()` quando o pino vai LOW. Previne sobrecarga da bobina e desgaste prematuro do MOSFET.
- **Fallback graceful quando mmWave degradado (S2):** Se `require_mmwave_empty = true` e o LD2410B está `DEGRADED`, `_handleDoorReopened()` usa timer de 3× `cfg.radar_debounce_ms` como proxy de ausência. FSM avança para `CONFIRMING` com flag `mmwave_fallback_used: true` no JWT. Sem fallback: sistema trava permanentemente em `DOOR_REOPENED` — o único cenário que bloqueava todos os entregadores seguintes sem intervenção manual.
- **Persistência de contexto de entrega na NVS durante o ciclo (S3):** Ao primeiro sinal de variação de balança em `INSIDE_WAIT`, ESP32 grava no namespace `dlv_ctx`: `qr_code`, `carrier`, `ts`. Se houver queda de energia entre `INSIDE_WAIT` e `CONFIRMING`, o boot recupera esses dados e gera o JWT com flag `recovery: true`. Sem isso: queda de energia perdia o comprovante mesmo com o pacote no corredor.
- **Watchdog de coerência FSM × hardware (S4):** Função `checkCoherence()` executada a cada 5s no `taskLogicBrain`. Verifica combinações impossíveis: FSM em `IDLE` com corrente no INA219 > 200mA por mais de 5s → `COHERENCE_STRIKE_IDLE`. P1 + P2 abertas simultaneamente em qualquer estado → `DOOR_ALERT`. Detecta falhas silenciosas de hardware.
- **Log de duração de estados (S5):** Cada `STATE_TRANSITION` inclui `prev_state` e `prev_duration_ms`. Dados para diagnóstico operacional: 47s em `WAITING_QR` antes de timeout indica etiqueta ilegível; 2s em `AUTHORIZED` confirma mola aérea funcionando.
- **Contador de ciclos do strike na NVS (S6):** `Preferences` no namespace `strike_p1`. Incrementado a cada `open()` confirmado pelo INA219. Publicado no heartbeat. Alerta configurável via HA quando ultrapassar threshold (padrão: 80.000 ciclos). Vida útil do strike documentada em tempo operacional real.
- **Spatial debouncing do LD2410B:** Micro-drops de presença (entregador imóvel, ponto cego momentâneo do radar) causavam falsa transição `DOOR_REOPENED → CONFIRMING` com pessoa ainda no corredor. Agora `person_present` só vai a `false` após `cfg.radar_debounce_ms` (padrão 1500ms) contínuos de ausência. Configurável via HA como `radar_dbc`.
- **I²C Bus Recovery por EMI do strike:** O pulso da bobina do strike gera pico eletromagnético que pode travar o INA219 com SDA em LOW. `PowerMonitor::readWithRecovery()` detecta a travada e executa 9 pulsos de clock no SCL por bit-bang (I²C Spec §3.1.16), libera o barramento e reinicializa o driver. Sem recovery: toda a task do Core 1 trava e o TWDT reinicia o chip perdendo o contexto.
- **Auto-zero tracking da balança em RAM:** Drift térmico em Jaboatão (30°C a 45°C, 80%+ de umidade relativa) causa variação de ~±300g ao longo do dia. Em `IDLE` com P1 e P2 fechadas, se a leitura da balança variar menos que `cfg.auto_zero_max_drift_g` (padrão 50g), o `_zero_offset` é recalibrado silenciosamente na RAM. **Nunca toca a NVS.** Compensa drift sem intervenção do morador.
- **Stale Data Protection (correção #16):** `PhysicalState` inclui `sample_age_ms` calculado no `getSnapshot()`. Se `sample_age_ms > cfg.stale_data_max_ms` (padrão 150ms), o tick da FSM é pulado naquele ciclo. Previne ações de hardware com dados de sensores desatualizados durante recuperação de EMI ou congestionamento de I²C.
- **`AUTHORIZED` timeout (correção #14):** Se P1 não abrir em `cfg.authorized_timeout_ms` (padrão 10s) após o strike ser acionado, FSM retorna ao `IDLE` com evento `AUTHORIZATION_TIMED_OUT`. Previne deadlock quando entregador não empurra a porta (mãos ocupadas, porta pesada, hesitação).
- **`DELIVERING` timeout (correção #15):** Se P1 não abrir em `cfg.delivering_timeout_ms` (padrão 180s) após objeto detectado na balança, FSM gera JWT com flag `unconventional_exit: true`, envia push alert ao morador e volta ao `IDLE`. Previne deadlock por calço acidental ou saída pela P2 (emergência).
- **`MAINTENANCE` como estado FSM próprio (não flag booleana):** Estado `MAINTENANCE` no enum. FSM entra por via do ConfigManager (`maintenance_mode = true` via HA) com prioridade absoluta sobre qualquer outro estado. Impossível transitar acidentalmente para `AUTHORIZED` ou abrir strikes durante manutenção.
- **udev rule `/dev/rlight_esp`:** Regra em `/etc/udev/rules.d/99-rlight-esp32.rules` vincula o symlink ao Vendor ID `303a` e Product ID `1001` do ESP32-S3. Surtos elétricos que resetam a porta USB e trocam `/dev/ttyACM0` para `/dev/ttyACM1` são invisíveis ao software — o path nunca muda.

### Changed
- **Botão interno de P2 eliminado:** Anteriormente planejado um botão momentâneo dentro do corredor para acionar P2. Removido. Acesso de moradores é exclusivamente via TF9S (D0/D1 Wiegand) ou HA. Simplifica o hardware e elimina superfície de ataque física.
- **`enable_strike_p2` migrado de `false` para `true` como default:** Strike P2 fisicamente instalado nesta versão.
- **Fluxo de acesso de moradores redesenhado:** A sequência anterior (código → P2 direta) é substituída por: TF9S válido → P1 abre por `door_open_ms` → P1 fecha → delay `p2_delay_ms` → P2 abre por `door_open_ms`. O airlock é sempre respeitado — P1 sempre fecha antes de P2 abrir.
- **`taskLogicBrain` verifica Wiegand fora do switch de estados:** O processamento do sinal TF9S ocorre antes do `switch(_ctx.state)`, com guarda explícita que impede interromper um ciclo de entrega ativo. Morador pode usar o TF9S enquanto sistema está em `IDLE` ou `MAINTENANCE`, mas não durante `WAITING_QR` a `RECEIPT`.

### Fixed
- **Colisão I²C entre Python e kernel no DS3231:** `rtc_sync.py` usava `smbus2` para ler o DS3231 diretamente, colidiendocom o driver nativo do kernel Armbian (`rtc_ds3231`). Corrigido: o script usa `time.time()` — o SO já sincronizou o relógio via `hwclock --hctosys` no boot.
- **Bloqueio permanente em `DOOR_REOPENED` com mmWave degradado (S2):** Sem o fallback por timer, um LD2410B falhando deixava o sistema preso indefinidamente neste estado, bloqueando todos os entregadores seguintes.

### Removed
- **Botão momentâneo interno (GPIO 8):** Ver "Changed" acima. Removido antes de ser fisicamente instalado.
- **PIN_ACCESS_CTRL_RX/TX (UART para TF9S):** Eliminado quando confirmado que o TF9S usa Wiegand, não UART ASCII.

### Security
- **Rate limiting do Wiegand:** 5 tentativas de códigos inválidos em 5 minutos → cooldown de 10 minutos. Previne brute-force físico no teclado ou com cartões clonados.
- **Interbloqueio em três camadas:** Em `tick()` antes de abrir P1, em `_handleResidentP2()` antes de abrir P2, e em `CMD_RESIDENT_UNLOCK` no UsbBridge. Impossível abrir P1 e P2 simultaneamente por qualquer caminho de código.

### Hardware
- **TF9S instalado:** Controle de acesso com teclado 4×3, leitor RFID IC, leitor de digital. Interface Wiegand D0 (GPIO 6) / D1 (GPIO 7). Pull-up externo 2,2kΩ recomendado em ambos os pinos.
- **Strike P2 (GPIO 46) instalado:** MOSFET LR7843, flyback 1N4007. High sólido nunca PWM.

---

## [v6.0.0] — Firmware Definitivo AMP FreeRTOS e Zero NVS Wear
**Data:** Março de 2026
**Codinome:** *Capibaribe*
**Foco:** Arquitetura de produção industrial, longevidade de hardware, código que dura anos sem manutenção.

### Added
- **Arquitetura AMP (Asymmetric Multiprocessing) com FreeRTOS:**
  - `taskSensorHub` (Core 1, 20Hz): todos os sensores físicos. Nunca executa lógica de negócio.
  - `taskLogicBrain` (Core 0, 100Hz): FSM, JWT, USB, cockpit HA. Nunca acessa hardware diretamente.
  - `loop()` do Arduino deletado com `vTaskDelete(NULL)`. Sem task ociosa.
- **`SharedMemory` com `SemaphoreHandle_t` Mutex:** Único canal de dados entre os núcleos. `update()` do Core 1, `getSnapshot()` do Core 0. Timeout de 10ms no `xSemaphoreTake` — Core 1 nunca bloqueia por Core 0 lento. `sample_age_ms` calculado fora do mutex para não aumentar tempo de lock.
- **`ConfigManager` data-driven com wear-leveling:** Todos os thresholds operacionais em NVS `Preferences`. Macro `UPD_UINT/FLOAT/BOOL` compara valor antes de qualquer `putX()`. Somente escreve na Flash quando o valor realmente muda. Contador teórico da Flash: ~100.000 ciclos de apagamento por célula — com wear-leveling, o ConfigManager pode ser usado diariamente por décadas.
- **Timestamp extrapolado entre sincronizações:** `ts = unix_time + ((millis() - millis_at_sync) / 1000)`. Aplicado em `CONFIRMING` e `DELIVERING` (unconventional exit). JWTs com horário preciso mesmo durante os 10 minutos entre sincronizações DS3231.
- **TWDT (Task Watchdog Timer) em ambos os núcleos:** `esp_task_wdt_init(5, true)` — panic se qualquer task não resetar em 5s. `esp_task_wdt_add(NULL)` em `taskSensorHub` (Core 1). `esp_task_wdt_reset()` em cada ciclo de ambas as tasks. HX711 travado por fio solto é detectado em 5s e causa reboot limpo.
- **Telemetria em RAM, entregas no SD (correção #13):** `/dev/shm/rlight_telemetry.db` (RAM disk nativo do Linux) para heartbeats, state transitions e sensor events. `/var/lib/rlight/deliveries.db` (MicroSD com SQLite WAL) apenas para entregas confirmadas e alertas críticos. Heartbeat a cada 60s = 1440 escritas/dia — com RAM disk, zero desgaste do SD para telemetria.
- **Feature toggles via HA sem reflash:** `maintenance_mode`, `enable_loitering_alarm`, `require_mmwave_empty`, `enable_auto_cooler`, `enable_strike_p2`. Todos em NVS, alteráveis via `CMD_UPDATE_CFG`. O sistema se comporta diferente sem nenhuma linha de código alterada.
- **Boot com leitura raw da balança antes da tare:** `Scale::initRaw()` lê o peso bruto antes de qualquer tare. Se `weight > min_delivery_weight_g` e P1 está fechada, o sistema detecta pacote residual de ciclo anterior interrompido por queda de energia. Evento `BOOT_RESIDUAL_WEIGHT` enviado ao OPi. Sem isso: tare zeraria o peso de um pacote que já está lá.

### Changed
- **`tare()` opera exclusivamente em RAM (correção #12):** A versão anterior da tare gravava `_zero_offset` na NVS a cada chamada. `tare()` é chamada no boot e pode ser chamada manualmente via `CMD_TARE_SCALE`. Com uso moderado, isso consumiria centenas de ciclos de Flash por mês. Corrigido: `_zero_offset` vive exclusivamente em RAM. `cal_factor` (calibração com peso conhecido) é o único valor que vai para a NVS, e apenas quando muda.
- **`QRReader::poll()` com buffer circular não-bloqueante (correção análoga #3):** Mesmo padrão do `UsbBridge`. Lê um `char` por ciclo, sem `Serial.readString()` bloqueante. Zero interferência no timing do HX711 e do mmWave no Core 1.

### Fixed
- **`StaticJsonDocument` incompatível com ArduinoJson v7 (correção #9):** A API v7 baniu `StaticJsonDocument<N>` e `DynamicJsonDocument`. Substituído por `JsonDocument doc` sem template em todo o `UsbBridge`. O pool de memória é gerenciado internamente pela biblioteca sem fragmentação.
- **millis() overflow a cada 49,7 dias (correção #10):** Toda aritmética de temporização migrada de comparação com soma (`millis() >= _open_until`) para subtração de `uint32_t` (`millis() - _opened_at >= _duration`). Por propriedade da aritmética modular de inteiros não-assinados, a subtração é sempre correta mesmo após rollover. Afetava o Strike, todos os timers da FSM e o auto-zero da balança.
- **TWDT cego no Core 1 (correção #11):** A versão anterior registrava o watchdog apenas na task principal do Arduino (Core 0). A `scalePollerTask` no Core 1 ficava fora do radar — HX711 travado por fio solto nunca causava reboot. Corrigido: `esp_task_wdt_add(NULL)` e `reset` dentro da task do Core 1.

### Removed
- **ZW111 biométrico:** Removido definitivamente. Leitor biométrico de superficie plana em ambiente externo tropical (80%+ umidade, poeira, luz solar direta) apresenta taxa de falha inaceitável. Acesso de moradores via TF9S (v7) ou chave física.
- **Estado `THEFT_ALERT`:** Lógica de vigilância de furto pós-entrega removida. O morador acessa o corredor para buscar a encomenda, causando falso positivo inevitável da balança em IDLE. A câmera Intelbras com DVR independente é a evidência forense — mais confiável que lógica de peso.
- **Estado `RECOVERY_WAIT` como estado navegável:** Simplificado para detecção no `setup()` + evento assíncrono `BOOT_RESIDUAL_WEIGHT` ao OPi. Sem estado adicional na FSM.
- **`loop()` do Arduino:** Substituído por `vTaskDelete(NULL)`. Em arquitetura FreeRTOS pura, a task do Arduino fica ociosa consumindo recursos. Agora é deletada no boot.

### Security
- **RF desligado permanentemente no nível de silício (correção #8 consolidada):** `esp_wifi_stop()`, `esp_wifi_deinit()`, `esp_bt_controller_disable()` são as três primeiras linhas do `setup()`. Air-gapped por design desde o primeiro ciclo de clock. Superfície de ataque remoto: zero.
- **NVS encryption via eFuse:** O segredo JWT é protegido pela Flash Encryption nativa do ESP32-S3. Extração física requer decapagem do chip e microscopia eletrônica.

### Hardware
- **Plataforma balança de piso (80×70cm):** HX711 + 4 células de carga 50kg (200kg nominal). Substituiu a balança de prateleira. Resolve: pacotes grandes que não cabem na prateleira, detecção de coleta reversa, suporta pessoa andando (pico ~100kg com fator de impacto — dentro da margem). `FLOOR_TRIGGER_S = 3s` discrimina passagem de pessoa de depósito estático.

---

## [v5.0.0] — Resiliência Industrial e Proteções Avançadas
**Data:** Fevereiro–Março de 2026
**Codinome:** *Boa Viagem*
**Foco:** Casos de borda que só aparecem em produção real.

### Added
- **Sanity check de timestamp (correção #17):** `TS_SANITY_MIN = 1700000000UL` (Novembro de 2023). `CMD_SYNC_TIME` com valor abaixo deste limiar é descartado silenciosamente. Flag `system_time_invalid` ativada. Evento `SYSTEM_TIME_INVALID_RTC_CHECK` enviado ao OPi. Previne JWTs com data de ano 2000 quando bateria CR2032 morre após queda conjunta de internet e energia.
- **`millis_at_sync` para extrapolação de timestamp:** Campo adicionado ao `FsmContext`. Ao receber `CMD_SYNC_TIME` válido, armazena `millis()` do momento da sincronização. A FSM usa `ts = unix_time + ((millis() - millis_at_sync) / 1000)` em vez do `unix_time` bruto. Sem isso: todas as entregas dentro da mesma janela de 10min teriam o mesmo timestamp.
- **Separação de bancos de dados por criticidade:** `TELEM_DB = /dev/shm/rlight_telemetry.db` (RAM), `DELIVERY_DB = /var/lib/rlight/deliveries.db` (SD com WAL). A separação é arquitetural — telemetria descartável não deve competir por ciclos de Flash com dados de entrega permanentes.
- **`authorized_timeout_ms` (correção #14):** Padrão 10s. Entregador com mãos ocupadas ou hesitante não bloqueia o sistema indefinidamente em `AUTHORIZED`.
- **`delivering_timeout_ms` (correção #15):** Padrão 180s. Calço acidental bloqueando P1 mecanicamente não deixa FSM presa em `DELIVERING` para sempre.
- **Spatial debouncing do LD2410B (1,5s):** Configurável via HA como `radar_dbc`. Valor ajustável para ambientes onde vegetação ou chuva forte causam eco esporádico.
- **I²C Bus Recovery (9 pulsos de clock):** Implementado em `PowerMonitor::readWithRecovery()`. Conforme I²C Specification §3.1.16.
- **Auto-zero tracking da balança:** `Scale::autoZeroTick(bool idle_safe)` chamado pelo `taskSensorHub` com `idle_safe = true` apenas quando FSM em `IDLE` e P1 + P2 fechadas. Ajuste máximo por ciclo limitado por `cfg.auto_zero_max_drift_g`. Sem escrita na NVS.
- **FSM preemptiva em `CONFIRMING` e `RECEIPT`:** Se P1 abre durante estes estados, FSM retorna imediatamente a `DELIVERING` sem gerar JWT com dados incompletos. Suporta entregador que volta com segundo pacote.
- **Feature toggles completos:** `maintenance_mode`, `enable_theft_alarm` (v5, depois removido na v6), `enable_loitering_alarm`, `require_mmwave_empty`, `enable_auto_cooler`.
- **udev rule `/dev/rlight_esp`:** Estabilidade do path USB independente de resets.

### Fixed
- **`Scale::tare()` gravando na NVS a cada chamada (correção #12):** `tare()` pode ser chamada no boot e manualmente. Com a implementação anterior, cada chamada consumia um ciclo de apagamento da Flash. Corrigido para RAM-only.
- **HX711 bloqueando o loop principal (correção #7 — reestruturação):** Migrado para task FreeRTOS dedicada no Core 1.

---

## [v4.0.0] — Arquitetura AMP, FreeRTOS e Criptografia de Borda
**Data:** Janeiro–Fevereiro de 2026
**Codinome:** *Tejipió*
**Foco:** Separação de responsabilidades entre núcleos, JWT real, configuração dinâmica.

### Added
- **Arquitetura AMP inicial:** `scalePollerTask` no Core 1 como primeira task isolada. `g_scale_weight` como variável global volatile lida pelo Core 0. Precursor do `SharedMemory` com Mutex da v6.
- **`ConfigManager` com NVS:** Primeira versão data-driven. Todos os thresholds saem do `#define` para NVS carregada no boot. `CMD_UPDATE_CFG` via HA.
- **`SharedMemory` com Mutex FreeRTOS:** `PhysicalState` protegida por `SemaphoreHandle_t`. Zero race conditions entre núcleos.
- **DS3231 via Orange Pi:** Arquitetura de sincronização de tempo definitiva. OPi lê o DS3231 via hwclock do kernel e envia `CMD_SYNC_TIME` via USB CDC a cada 10 minutos. ESP32 mantém `unix_time` no `FsmContext`. Sem módulo RTC de hardware no ESP32.
- **`rtc_sync.py` sem smbus2:** Usa `time.time()` — o Armbian já gerencia o DS3231 como driver de kernel. Zero colisão I²C entre userspace e kernel.

### Fixed
- **`StaticJsonDocument` banido pelo ArduinoJson v7 (correção #9):** `JsonDocument doc` sem template.
- **millis() overflow a 49,7 dias (correção #10):** Subtração de uint32_t em todos os timers.
- **TWDT cego no Core 1 (correção #11):** `esp_task_wdt_add(NULL)` na task do Core 1.

### Security
- **RF desligado explicitamente:** `esp_wifi_stop()` e `esp_bt_controller_disable()` no `setup()`.

---

## [v3.0.0] — Firmware Industrial, Self-Healing e Eliminação de Heap
**Data:** Dezembro de 2025–Janeiro de 2026
**Codinome:** *Olinda*
**Foco:** Código que compila, não trava e funciona por anos sem atenção.

Esta versão foi o resultado de duas revisões independentes de código que identificaram 8 falhas críticas na implementação anterior. Todas foram corrigidas.

### Fixed — Batch de correções críticas

- **Correção #1 — Fragmentação de heap (classe `String`):** A classe `String` do Arduino aloca memória dinamicamente no heap. Após meses de uso contínuo, a RAM do ESP32 vira "queijo suíço" (fragmentação). Um dia, na geração do JWT, não existe bloco contíguo disponível → `std::bad_alloc` → panic/reboot. Corrigido: `String` proibida em todos os paths críticos. `char[]` + `snprintf` + `strlcpy` em todos os módulos (`JwtSigner`, `UsbBridge`, `HealthMonitor`, `QRReader`). Tamanhos de buffer definidos como constantes em `Config.h`.

- **Correção #2 — Strike bloqueante com `delay()`:** O acionamento do strike usava `delay(500)` para manter o pino HIGH. Durante 500ms, o ESP32 não escutava USB, ignorava botões e os LEDs PWM por software paravam. Pior: na v5, com o pulso de 3s para a mola aérea, o sistema ficaria congelado por 3 segundos a cada abertura. Corrigido: `Strike::open()` apenas seta `_opened_at = millis()` e `_duration`. `Strike::tick()` no loop desliga o pino quando `millis() - _opened_at >= _duration`. Zero bloqueio.

- **Correção #3 — `Serial.readStringUntil('\n')` bloqueante:** Esta função do framework Arduino aguarda até 1000ms se o terminador não chegar (interferência eletromagnética do strike, cabo USB sem blindagem). Durante 1 segundo, a balança perde amostras e a FSM atrasa. Corrigido: buffer circular caractere a caractere. `Serial.available()` verificado no loop, um `char` lido por ciclo, JSON processado apenas quando `\n` chega. Zero espera.

- **Correção #4 — Ausência de Task Watchdog (TWDT):** Se o barramento I²C travar (notoriamente comum com ruídos elétricos afetando os pull-ups do SDA/SCL durante o pulso do strike), `Wire.h` entra em loop infinito. O sistema trava com a tela ligada e o OPi não pode fazer nada — sem watchdog, nunca reinicia. Corrigido: `esp_task_wdt_init(5, true)` no `setup()`. `esp_task_wdt_add(NULL)` registra a task principal. `esp_task_wdt_reset()` na primeira linha do `loop()`.

- **Correção #5 — ISR sem debounce (bouncing fantasma):** Micro switches mecânicos tremem em nível microscópico quando os contatos fecham, gerando dezenas de transições HIGH/LOW em menos de 2ms. A ISR disparava 50 vezes por fechamento de porta. Corrigido: filtro de 50ms por timestamp dentro da `IRAM_ATTR`. `volatile uint32_t last_isr_p1` persiste entre chamadas e descarta transições dentro da janela de bouncing.

- **Correção #6 — `delay()` na FSM (`_handleConfirming`):** `delay(SCALE_SETTLE_MS)` de 2 segundos antes de ler a balança. Durante esses 2s: sem USB, sem botões, sem atualização de LEDs. Corrigido: flag `scale_settle_start`. Fase 1: registra `millis()` e retorna. Fase 2 (próximos ciclos): verifica elapsed. Apenas avança quando 2s passaram sem bloquear.

- **Correção #7 — HX711 bloqueando o loop (300ms por leitura):** O HX711 opera a 10 samples/segundo. `read_average(3)` = 300ms de bloqueio. A FSM rodava a ~3 "frames" por segundo. Corrigido: `FreeRTOS Task` no Core 1 com `vTaskDelay(100ms)` de yield. A FSM no Core 0 lê `g_scale_weight` (variável volatile) em tempo zero.

- **Correção #8 — RF não desligado explicitamente:** A biblioteca Arduino inicializa o rádio WiFi passivamente. Sem desligamento explícito, o ESP32 pode ser escaneado por ferramentas WiFi e consome energia de rádio desnecessária. Corrigido: `esp_wifi_stop()`, `esp_wifi_deinit()`, `esp_bt_controller_disable()` antes de qualquer outra inicialização.

### Added
- **`HealthMonitor` com `SensorHealth` struct:** `online`, `degraded`, `fail_count`, `total_fails`, `last_ok_ms`, `last_fail_ms`, `last_error[48]`, `auto_recovered`. Cada sensor tem score ponderado em `systemScore()` (0–100). `tryRecover()` tenta reinicializar sensores degradados a cada 30s.
- **Operação degradada por sensor:** Sensor com 3 falhas consecutivas → `degraded = true`. FSM adapta o fluxo: HX711 degradado → peso `null` no JWT; mmWave degradado → fallback por timer; GM861S degradado → fallback interfone imediato.
- **`UsbBridge` com buffer circular completo:** Toda serialização usa `snprintf` com `char[]`. `_dispatch()` processa JSON completo somente quando `\n` chega. CRC8 em todos os pacotes (Dallas/Maxim polinômio 0x07).
- **`QRReader` com identificação de transportadora:** `CarrierPattern` struct com validador opcional. Correios: validação S10 completa (2 letras + 8 dígitos + check + BR). Shopee: prefixo SPX. Amazon: prefixo TBA. Mercado Livre: OI/LP/MLE. Loggi: LGI. Jadlog: J + comprimento mínimo. `isAllowedCarrier()` em modo lista de bloqueio: INVALID não passa, UNKNOWN passa.
- **`JwtSigner` stub funcional:** Implementação simulada com `snprintf` correto. Substituída pela implementação real com mbedTLS na v8.
- **Pinout definitivo da v3:** Todos os 21 GPIOs documentados com função, interface, direção e observação crítica.

### Hardware
- **Migração de balança de prateleira para plataforma de piso:** Decisão tomada nesta versão baseada em análise de casos de borda. A plataforma de piso (80×70cm, 200kg nominal) resolve simultaneamente: pacotes grandes, coleta reversa, detecção de passagem de pessoa por timer.
- **Remoção de MPU6050 e BME280:** Sensor de vibração e sensor de temperatura externa removidos. Temperatura monitorada pelo sensor nativo do ESP32-S3 (`temperatureRead()`).

---

## [v2.0.0] — Orange Pi, USB CDC e Primeira FSM
**Data:** Outubro–Novembro de 2025
**Codinome:** *Recife Antigo*
**Foco:** Computação distribuída, interface gráfica e máquina de estados estruturada.

### Added
- **Orange Pi Zero 3 como SBC de mídia e middleware:** Armbian, OverlayFS rootfs somente-leitura, systemd. Ethernet GbE como primário, WiFi como fallback. Separação de responsabilidades: ESP32 cuida do tempo real, OPi cuida de mídia, rede e interface.
- **Comunicação USB CDC (pinos 19/20):** Substituição de UART convencional por USB CDC nativo do ESP32-S3. Vantagens: velocidade superior, detecção de conexão/desconexão, elimina conversor de nível lógico, path estável via udev.
- **Primeira FSM estruturada:** Estados `IDLE`, `AWAKE`, `WAITING_QR`, `AUTHORIZED`, `INSIDE_WAIT`, `DELIVERING`, `CONFIRMING`, `RECEIPT`, `ABORTED`, `DOOR_ALERT`, `ERROR`. Transições auditáveis via `transition()` centralizado.
- **Interface pygame kmsdrm (sem X11):** Tela HDMI 7" IPS 1024×600px renderizada diretamente no framebuffer via KMS/DRM. Sem servidor gráfico, zero overhead de X11. `pygame.FULLSCREEN | pygame.DOUBLEBUF | pygame.HWSURFACE`.
- **Integração Home Assistant via MQTT:** MQTT Auto-Discovery para exposição automática de entidades. Primeiras entidades: `sensor.fsm_state`, `sensor.weight_g`, `switch.maintenance_mode`.
- **`MachineState` observer pattern:** Espelho Python da FSM do ESP32. `subscribe(callback)` para múltiplos listeners (UI, HA, câmera, Oracle). Thread-safe com `threading.Lock()`.
- **`SerialBridge` com reconexão elástica:** Loop de reconexão com `time.sleep(2)` em `SerialException`. Survives hot-plug do cabo USB.
- **GM861S leitor 2D UART:** Lê QR Code, Code 128, EAN, PDF417 de etiquetas de qualquer transportadora. Montado atrás de acrílico IR-pass. LED de iluminação 12V durante `WAITING_QR`.
- **LD2410B mmWave 24GHz:** Detecção de presença estacionária. Instalado no teto do corredor. Fundamental para confirmar saída do entregador antes de gerar comprovante.
- **HX711 + 4 células de carga:** Primeira implementação de balança. Prateleira 1m×20cm, 200kg nominal, resolução prática ~2g.
- **Electric strike 12V fail-secure:** MOSFET LR7843 com flyback 1N4007. HIGH sólido, nunca PWM. Fail-secure: fecha sem energia. P1 externa.
- **INA219 monitor de corrente:** Validação de acionamento do strike. Se corrente < `ina_strike_min_ma` após pulso → `STRIKE_P1_FAIL` + transição para `ERROR`.
- **Cooler 40mm com PWM por temperatura:** `temperatureRead()` nativo do ESP32-S3. Canal ledc. Liga se `> COOLER_TEMP_MIN_C`, interpola até 100% em `COOLER_TEMP_MAX_C`.
- **Buzzer ativo com padrões por estado:** `beep(1, 100)` em AWAKE, `beep(2, 150)` em AUTHORIZED, `melody_success()` em RECEIPT, `continuous(true)` em DOOR_ALERT.
- **LED azul 30mm com efeitos PWM:** Breathe 2s = IDLE, blink 300ms = WAITING_QR, solid = AWAKE/AUTHORIZED. Canal ledc com função matemática `exp(sin(millis * k))` para efeito de respiração realista.
- **ISRs nos limit switches:** `IRAM_ATTR` para latência mínima. Interrupção CHANGE em P1 (GPIO 38) e P2 (GPIO 39).
- **`OracleRESTClient` com retry infinito:** Upload de JWT + foto em background thread. Backoff `min(60, attempt * 5)` segundos. Nenhuma entrega perdida por falta de internet temporária.

### Changed
- **ESP32 → ESP32-S3 N16R8:** Upgrade para dual-core 240MHz, 16MB Flash, 8MB PSRAM, acelerador criptográfico mbedTLS nativo, USB OTG nativo (USB CDC sem chip adicional).

### Notes
- A decisão de usar USB CDC em vez de UART para comunicação ESP32↔OPi foi tomada após perceber que o conversor serial adicional introduzia um ponto de falha e um limite de velocidade desnecessários. O USB CDC do ESP32-S3 apresenta-se como dispositivo ACM ao Linux — driver `cdc_acm` nativo, sem instalação.

---

## [v1.0.0] — Proof of Concept e Origem do Projeto
**Data:** Setembro–Outubro de 2025
**Codinome:** *Guararapes*
**Foco:** Validar a hipótese central: é possível construir uma portaria autônoma de qualidade industrial com hardware acessível.

### Added
- **ESP32 (original) acionando relé:** Primeiro circuito funcional. Strike 12V controlado por relé de 5V via transistor NPN. Acionamento por botão físico e comando serial.
- **Validação do airlock de duas portas:** Confirmação física de que o conceito de dois portões — P1 externa abrindo para corredor, P2 interna separando corredor da casa — é construtivamente viável no espaço disponível.
- **Primeiro script Python de controle:** Envio de comandos por serial para o ESP32. `serial.Serial` + `readline()`. Sem estrutura de protocolo.
- **Definição da filosofia de design:** "Física determinística em vez de IA visual." Câmeras existem para registro, nunca para decisão. Esta premissa não mudou em nenhuma versão subsequente.

### Notes
- A inspiração para o nome *rlight* vem de um avião construído artesanalmente pelo pai do criador do projeto — um projeto de décadas que combina engenharia, paciência e amor pelo que se faz. A portaria é, em escala diferente, o mesmo tipo de projeto: algo construído com as próprias mãos, para durar.
- O local de instalação é Jaboatão dos Guararapes, PE — litoral pernambucano com temperatura de 30–45°C, umidade relativa de 70–90%, chuvas torrenciais sazonais. Todas as decisões de hardware consideram este ambiente.

---

## Registro de Decisões de Arquitetura (ADR)

### ADR-001: Física como dado, câmera como registro
**Status:** Ativo desde v1.0.0
A câmera é o único sensor que interpreta imagens. Todos os outros sensores medem grandezas físicas objetivas: peso (HX711), presença (mmWave), corrente (INA219), estado de porta (micro switch). Nenhuma decisão crítica — abrir porta, gerar JWT, detectar entrega — depende de visão computacional. Câmeras falham com iluminação, ângulo, oclusão e degradação de modelo. Física não mente.

### ADR-002: Air-gapped por design
**Status:** Ativo desde v3.0.0
O ESP32-S3 tem WiFi e Bluetooth desligados permanentemente no nível de silício. O firmware não inclui nenhum driver de rede. A superfície de ataque remoto é zero. Toda comunicação é via USB CDC físico para o Orange Pi, que é o único nó com acesso à rede.

### ADR-003: Offline-first, nuvem como espelho
**Status:** Ativo desde v2.0.0
Toda entrega é processada e o comprovante é gerado localmente, sem dependência de internet. O JWT é assinado no ESP32-S3 com segredo que nunca sai do chip. O servidor Oracle Cloud é um espelho de conveniência — não é autoridade. Um JWT válido gerado durante queda de internet é tão autêntico quanto um gerado com conexão plena.

### ADR-004: Wear-leveling ativo da NVS
**Status:** Ativo desde v5.0.0
A Flash do ESP32-S3 tem ~100.000 ciclos de apagamento por célula. Qualquer write sem necessidade é um desperdício de vida útil. Regras:
1. `tare()` da balança nunca escreve na NVS — RAM only.
2. `ConfigManager::updateParam()` compara o valor antes de qualquer `putX()`.
3. O único valor que vai para NVS na calibração é `cal_factor`, e apenas quando muda.
4. Telemetria (heartbeats, state transitions) em `/dev/shm` (RAM disk) — zero ciclos do MicroSD.

### ADR-005: Fail-secure em todos os atuadores
**Status:** Ativo desde v1.0.0
Queda de energia fecha P1 (strike fail-secure). P2 também fail-secure. Uma pessoa presa no corredor durante queda de energia é o pior cenário — o morador deve abrir manualmente. Este trade-off (conforto operacional vs. segurança física) está documentado e é intencional. Um botão de saída de emergência interno que abre P1 independentemente de energia é recomendado para instalação final.

### ADR-006: AMP (Asymmetric Multiprocessing) com FreeRTOS
**Status:** Ativo desde v4.0.0, consolidado em v6.0.0
Core 1 é o sistema nervoso (sensores, I/O, tempo real). Core 0 é o cérebro (lógica, criptografia, rede). Nenhum core conhece os detalhes de implementação do outro — apenas a `SharedMemory` com Mutex. Esta separação é arquitetural, não apenas de performance. Permite testar a FSM sem hardware físico (unit tests com mock de `PhysicalState`).

### ADR-007: JWT como prova, não como acesso
**Status:** Ativo desde v2.0.0
O JWT gerado pelo ESP32-S3 prova que uma entrega ocorreu. Não concede acesso a nada. A chave de assinatura nunca sai do chip. O servidor Oracle armazena apenas a chave pública para verificação — comprometimento do servidor não compromete tokens passados ou futuros.

---

## Mapa de Componentes por Versão

| Componente | v1 | v2 | v3 | v4 | v5 | v6 | v7 | v8 |
|---|---|---|---|---|---|---|---|---|
| ESP32 (original) | ✓ | — | — | — | — | — | — | — |
| ESP32-S3 N16R8 | — | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| Orange Pi Zero 3 | — | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| GM861S QR Reader | — | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| LD2410B mmWave | — | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| HX711 Balança | — | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| INA219 (P1) | — | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| INA219 (P2) | — | — | — | — | ✓ | ✓ | ✓ | ✓ |
| Strike P1 | — | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| Strike P2 | — | — | — | — | — | — | ✓ | ✓ |
| Micro switch P1 | — | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| Micro switch P2 | — | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| DS3231 RTC | — | — | — | ✓ | ✓ | ✓ | ✓ | ✓ |
| Webcam USB | — | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| Câmera Intelbras+DVR | — | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| Tapo D210 (fachada) | — | — | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| ZW111 Biometria | — | — | ✓ | ✓ | ✓ | — | — | — |
| TF9S + Wiegand | — | — | — | — | — | — | ✓ | ✓ |
| Balança prateleira | — | — | ✓ | ✓ | — | — | — | — |
| Balança piso 80×70cm | — | — | — | — | ✓ | ✓ | ✓ | ✓ |

---

## Glossário Técnico

| Termo | Definição no contexto rlight |
|---|---|
| **AMP** | Asymmetric Multiprocessing — ESP32-S3 dual-core com Core 0 (lógica) e Core 1 (sensores) com responsabilidades estritamente separadas |
| **Air-gapped** | WiFi e Bluetooth desligados permanentemente no nível de silício. Sem superfície de ataque remoto no ESP32-S3 |
| **Airlock** | Os dois portões em sequência (P1 externa → corredor → P2 interna). P1 e P2 nunca abrem simultaneamente |
| **Conformal coating** | Verniz eletrônico aplicado no HX711 para proteção contra umidade tropical (70-90% UR) e oxidação |
| **Fail-secure** | Strikes elétricos que fecham quando sem energia. Queda de energia nunca abre uma porta |
| **FSM** | Finite State Machine — máquina de estados finita. 16 estados na v7/v8 |
| **HX711** | Conversor analógico-digital de 24 bits para células de carga. Interface bit-bang, 10 samples/segundo |
| **INA219** | Sensor de corrente e tensão I²C. Usado para validar acionamento do strike e detectar desgaste |
| **JWT** | JSON Web Token — comprovante criptográfico de entrega assinado pelo ESP32-S3 com HMAC-SHA256 |
| **LD2410B** | Radar mmWave 24GHz. Detecta presença estacionária (pessoa parada no corredor) com alcance configurável por gates |
| **mbedTLS** | Biblioteca criptográfica embarcada da ARM, nativa no ESP-IDF do ESP32-S3. Usa acelerador de hardware |
| **NVS** | Non-Volatile Storage — armazenamento em Flash do ESP32. Namespace-based, ~100k ciclos de apagamento por célula |
| **OverlayFS** | Sistema de arquivos do Linux que sobrepõe uma camada de escrita em RAM sobre um rootfs somente-leitura. Previne corrupção do SD por queda de energia |
| **PhysicalState** | Struct C++ que representa o estado físico do mundo (pesos, portas, presença, correntes) em um dado momento. Único dado compartilhado entre núcleos |
| **SharedMemory** | Classe singleton que encapsula o PhysicalState com proteção por SemaphoreHandle_t (Mutex FreeRTOS) |
| **Spatial debouncing** | Técnica que exige N milissegundos contínuos de ausência antes de marcar `person_present = false`. Previne falsos "vazios" do mmWave |
| **Stale Data** | Dado do SharedMemory com `sample_age_ms > stale_data_max_ms`. FSM pula o tick quando dados estão velhos demais |
| **Strike** | Electric strike — fechadura elétrica solenóide. Abre quando energizada (12V). Fail-secure: fecha quando desligada |
| **TF9S** | Módulo de controle de acesso com teclado 4×3, leitor RFID IC e digital. Interface Wiegand 26-bit D0/D1 |
| **TWDT** | Task Watchdog Timer — mecanismo de hardware que causa panic/reboot se uma task não resetar o contador em N segundos |
| **USB CDC** | USB Communications Device Class — apresenta o ESP32-S3 como porta serial virtual ao Linux sem driver adicional |
| **Wear-leveling** | Técnica de preservação da Flash: só escreve quando o valor realmente muda, nunca por rotina |
| **Wiegand** | Protocolo de dois fios (D0/D1) para transmissão de dados de controle de acesso. Cada bit é um pulso de ~50µs em D0 (bit 0) ou D1 (bit 1) |

---

*rlight.com.br · Jaboatão dos Guararapes, Pernambuco, Brasil*
*Construído com as próprias mãos, para durar.*