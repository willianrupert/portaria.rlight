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

### Fixed — Batch de correções críticas
- **Correção #1 — Fragmentação de heap (classe `String`):** Corrigido: `String` proibida em todos os paths críticos.
- **Correção #2 — Strike bloqueante com `delay()`:** Corrigido: Uso de timers não-bloqueantes.
- **Correção #3 — `Serial.readStringUntil('\\n')` bloqueante:** Corrigido: buffer circular caractere a caractere.
- **Correção #4 — Ausência de Task Watchdog (TWDT):** Corrigido: `esp_task_wdt_init(5, true)`.
- **Correção #5 — ISR sem debounce:** Corrigido: filtro de 50ms por timestamp.

---

## [v2.0.0] — Orange Pi, USB CDC e Primeira FSM
**Data:** Outubro–Novembro de 2025
**Codinome:** *Recife Antigo*

### Added
- **Orange Pi Zero 3 como SBC de mídia e middleware.**
- **Comunicação USB CDC nativa.**
- **Interface pygame kmsdrm.**

---

## [v1.0.0] — Proof of Concept e Origem do Projeto
**Data:** Setembro–Outubro de 2025
**Codinome:** *Guararapes*

### Added
- **ESP32 (original) acionando relé.**
- **Validação do airlock de duas portas.**

---

## Registro de Decisões de Arquitetura (ADR)

### ADR-001: Física como dado, câmera como registro
**Status:** Ativo desde v1.0.0
Câmeras falham com iluminação. Física (peso, presença, corrente) não mente.

### ADR-002: Air-gapped por design
**Status:** Ativo desde v3.0.0
ESP32-S3 tem WiFi/BT desligados permanentemente. Superfície de ataque zero.

### ADR-003: Offline-first, nuvem como espelho
**Status:** Ativo desde v2.0.0
Assinatura JWT local. Nuvem Oracle é espelho de conveniência, não autoridade.

### ADR-004: Wear-leveling ativo da NVS
**Status:** Ativo desde v5.0.0
Preservação da Flash: zero writes desnecessários na NVS/SD.

---

## Mapa de Componentes por Versão

| Componente | v1 | v2 | v3 | v4 | v5 | v6 | v7 | v8 |
|---|---|---|---|---|---|---|---|---|
| ESP32-S3 N16R8 | — | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| Orange Pi Zero 3 | — | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| Strike P1/P2 | ✓/— | ✓/— | ✓/— | ✓/— | ✓/— | ✓/— | ✓/✓ | ✓/✓ |

---

## Glossário Técnico

| Termo | Definição no contexto rlight |
|---|---|
| **AMP** | Asymmetric Multiprocessing — Core 0 (lógica) e Core 1 (sensores) |
| **JWT** | JSON Web Token — comprovante criptográfico de entrega HMAC-SHA256 |
| **NVS** | Non-Volatile Storage — armazenamento em Flash do ESP32 |

---

*rlight.com.br · Jaboatão dos Guararapes, Pernambuco, Brasil*
*Construído com as próprias mãos, para durar.*
