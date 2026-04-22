# rlight — Uma Portaria que Pensa (Firmware v7)

Bem-vindo ao repositório do **rlight-firmware-v6**. 
Esta documentação é uma adaptação técnica da [Visão de Produto Original](https://docs.google.com/document/d/16W1DhlyzR_M756VwtquH-GJW7is5pEe3DV4iOpLja9g/edit), refletindo **exatamente** o que foi implementado na arquitetura de código C++ / PlatformIO deste projeto para o microcontrolador ESP32-S3.

A entrega não termina quando o entregador vai embora. Ela termina quando o morador tem a certeza — criptograficamente verificável — de que o pacote está seguro. Este firmware é o sistema nervoso autônomo que torna isso possível.

---

## 1. Visão Arquitetural Implementada (AMP)
Para lidar com a complexidade de múltiplos sensores lidos em tempo real e de atuadores acionados concomitantemente sem bloqueios de CPU, o firmware foi modelado sob o paradigma de **Processamento Assimétrico (AMP)** nativo do FreeRTOS no ESP32-S3.

O fluxo de trabalho abandonou o `loop()` tradicional do Arduino e foi dividido em dois núcleos:
* **Core 1 (`taskSensorHub`):** Roda a 20Hz. Responsável exclusivamente pela leitura do mundo físico. Ele interroga balança (HX711), radar de presença (LD2410B), leitor de QR codes GM861S em buffer circular (não bloqueante) e monitores de corrente INA219. Os dados coletados são embalados em um struct `PhysicalState` e copiados para uma memória compartilhada.
* **Core 0 (`taskLogicBrain`):** Roda a 100Hz. Responsável pelas decisões. Avalia a idade estrutural dos dados (Stale Data Protection), roda os cálculos matemáticos da `StateMachine` preemptiva, lida com criptografia (JWT) e controla a comunicação USB com a Orange Pi (`UsbBridge`).

### 1.1. Inclusão da Trava P2 Flexível
Foi solicitado que a **Strike P2** (Fechadura eletromagnética interna da residência) estivesse presente na arquitetura de software, mas fisicamente desativada por padrão visando máxima segurança. 
Nós implementamos o `Strike::P2()` assíncrono exatamente como o P1, atrelado ao `GPIO 46` com a classe `PowerMonitor::P2()`. Todavia, toda a lógica da FSM a ignora a menos que o morador altere a NVS (Flash) de configuração remotamente (`strike_p2 = true`) ou que envie expressamente o comando digital json `CMD_UNLOCK_P2` via rede.

---

## 2. A Física como Dado: Os Sensores

Enquanto o mercado persegue modelos complexos de inteligência artificial baseados puramente em câmeras,## 3. Diretrizes da Interface (Web UI v7)
- **Orientação:** Vertical (Portrait) — otimizada para 600x1024.
- **Ecossistema:** "Apple Style", moderno, minimalista, *glassmorphism*, tipografia SF Pro/Inter.
- **HUD Global:** Cabeçalho persistente com Relógio, Data, Status de Rede e Nome do Estado.
- **Gerenciamento de Energia:** A tela IPS fica DESTRIGADA (apagada) no estado IDLE e MAINTENANCE via DPMS.
- **Editabilidade:** Mensagens dinâmicas (`ENDERECO_TEXT`, `NOMES_RECEBEDORES`, `MSG_ERRO`, `MSG_DOOR_ALERT`) injetadas via WebSocket, permitindo controle total pelo Home Assistant sem re-deploy.

## 4. Histórico de Versões & Progresso
- **[x] Migração de UI (Pygame -> Web):** 
  - [x] Substituído `pygame` por SPA Web moderna (HTML/CSS/JS).
  - [x] Implementadas as 16 telas baseadas no `rlight_ui_spec.md`.
  - [x] Comunicação reativa via WebSockets (`/ui/stream`) com o host Python.
  - [x] Design refeito para orientação Vertical (Portrait).
- **[x] Atualização Firmware ESP32-S3 (v7):**
  - [x] Integração do Teclado Matricial I2C (PCF8574).
  - [x] Lógica de intertravamento P1 + P2 (Dois Strikes).
  - [x] Broadcast de telemetria estendida (Carrier, Resident Label).
- **[x] Deploy e Repositório:**
  - [x] Git inicializado e linkado ao repositório remoto oficial.
  - [x] Push realizado com sucesso.rma serial e analisada assincronamente.

---

## 3. Os Estados do Ambiente — A FSM em Ação

O estado do ambiente não é gerido por *flags* soltas booleanas. A classe modular `StateMachine.cpp` atua contendo **16 estados bem definidos** (v7), espelhando as exatas diretivas da experiência humana:

1.  **`IDLE`**: Silêncio visual. Apenas um ponto pulsante e logo discreta.
2.  **`AWAKE`**: Despertar imediato. Instrução clara de leitura com seta Apple-style.
3.  **`WAITING_QR`**: Leitura ativa com efeito shimmer e fallback de interfone/telefone após 15s.
4.  **`AUTHORIZED`**: Confirmação visual "ACESSO LIBERADO" com checkmark animado.
5.  **`INSIDE_WAIT`**: Orientação de depósito com visualização de peso em tempo real.
6.  **`DELIVERING`**: Feedback de depósito confirmado e instrução de saída.
7.  **`DOOR_REOPENED`**: Monitoramento de saída segura via radar mmWave.
8.  **`CONFIRMING`**: Processamento criptográfico e estabilização de carga.
9.  **`RECEIPT`**: Exibição do snapshot da webcam e QR Code de validação.
10. **`ABORTED`**: Ciclo encerrado sem depósito (neutro).
11. **`RESIDENT_P1`**: Boas-vindas personalizada para moradores (via Senha no Teclado).
12. **`RESIDENT_P2`**: Intertravamento de segurança P1/P2 com timer visual.
13. **`REVERSE_PICKUP`**: Fluxo de coleta com indicação de variação de peso negativa.
14. **`DOOR_ALERT`**: Alerta vermelho crítico de porta esquecida aberta.
15. **`MAINTENANCE`**: Diagnóstico de saúde dos sensores em tempo real.
16. **`ERROR`**: Bloqueio por falha de hardware com instrução física de campainha.

---

## 4. Interface Web Moderníssima (Glassmorphism)

A v7 abandona o renderizador Pygame em favor de uma **Single Page Application (SPA)** rodando em Chromium Kiosk Mode.

*   **Estética Apple-like**: Uso extensivo de `backdrop-filter: blur()`, tipografia SF Pro e animações suaves de 60fps.
*   **Orientação Vertical**: Otimizada para telas de 7 polegadas montadas em portrait (600x1024).
*   **HUD Global**: Cabeçalho inteligente com relógio, status de conexão e score de saúde do sistema.
*   **Reatividade Total**: O host Python injeta estados via WebSocket (`/ui/stream`), fazendo a UI reagir instantaneamente a cada evento físico do ESP32.

---

## 5. O Sistema Nervoso Distribuído (Edge e Segurança)

### Criptografia de Borda: JWT Simulado e Concretizado
Para garantir a confiança na prova fotográfica tirada pela Orange Pi, o microcontrolador ESP32-S3 garante a chancela do peso lido injetando as chaves. Dentro do estado `CONFIRMING`, o ESP32 (`JwtSigner.cpp`) é instigado a criar uma string atestando localmente que um objeto pesando, por exemplo, `1890.3g` foi depositado lá por um código logístico verificável de uma transportadora homologada.

### O Clock Extrapolado (`rtc_sync.py`)
No mundo dos SBCs Linux locais, disputas entre I2C do kernel e do usuário sobre o módulo relógio de tempo real (`DS3231`) geram instabilidade severa. Na nossa configuração final implementada dentro de `/scripts`, resolvemos isso com maestria. O `rtc_sync.py` gerido pelo host confia na camada abstrata do SO Linux para obter dados de ponta do hardware com `time.time()`. Em seguida, as informações são injetadas no barramento USB CDC (`UsbBridge.cpp` captando pelo `CMD_SYNC_TIME`) para dentro do ESP32-S3, que realiza suas extrapolações em tempo de voo com os submilisegundos nativos, evitando congestionar qualquer pino local.

---

## Instalação e Compilação

Toda a arquitetura proposta está construída sob o capricho da padronização **PlatformIO**.

1. Instale a extensão do [PlatformIO IDE](https://platformio.org/install/ide?install=vscode) no VSCode.
2. Abra esta raiz de diretório no VSCode.
3. Clique em "Build" e veja o gerenciador de pacotes baixar autonomamente o *Arduino Core*, o *ArduinoJson v7* e as bibliotecas essenciais para o S3.
4. Conecte o cabo USB para uso direto do USB CDC das portas configuradas 19/20 com a placa `esp32-s3-devkitc-1`.

> **AVISO IMPORTANTE:** Os pinos GPIO 43/44 são canais seriais nativos U0TXD e U0RXD, usados no momento do FLash primário. Lembre-se, conforme as boas práticas estipuladas no rlight v6, **devem ser desconectados do GM861S antes da primeira compilação física.**

---
*rlight.com.br - Jaboatão dos Guararapes, Brasil.*
