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

