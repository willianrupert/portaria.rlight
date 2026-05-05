# RLight — Uma Portaria que Pensa (v7 / v8-ready)

Bem-vindo ao repositório oficial do **RLight Portaria Autônoma**.
Esta documentação reflete a evolução arquitetural definitiva do projeto, migrando de uma interface gráfica nativa pesada para um ecossistema **Web SPA Moderno** rodando em conjunto com o poderoso microcontrolador **ESP32-S3**.

A entrega não termina quando o entregador vai embora. Ela termina quando o morador tem a certeza — criptograficamente verificável — de que o pacote está seguro. Este projeto é o sistema nervoso autônomo que torna isso possível.

---

## 1. Visão Arquitetural Implementada (AMP)

Para lidar com a complexidade de múltiplos sensores em tempo real e atuadores mecânicos sem bloqueios de CPU, o firmware foi modelado sob o paradigma de **Processamento Assimétrico (AMP)** nativo do FreeRTOS.

O fluxo de trabalho abandonou o `loop()` tradicional e foi segmentado:
* **Core 1 (`taskSensorHub`):** Roda a 50Hz. Lida exclusivamente com o mundo físico. Faz o *polling* contínuo da balança (HX711), radar de presença (LD2410B), leitor de QR Code (GM861S) e agora do **Teclado Matricial I2C (PCF8574)**. Os dados são estruturados e copiados para uma *Shared Memory*.
* **Core 0 (`taskLogicBrain`):** Roda a 100Hz. Toma as decisões. Executa a *StateMachine* preemptiva, controla a criptografia e processa os eventos WebSocket via **Native USB CDC** (Ponte UART ultra-rápida com a Orange Pi Zero 3).

### 1.1. Integração Segura e Isolamento de GPIO
Foi estabelecida a **compatibilidade 100% segura com o módulo WROOM-2 N8R8** (que utiliza Octal PSRAM). Todos os pinos do projeto foram meticulosamente mapeados fora da "zona de perigo" (GPIO 33-37), garantindo zero kernel panics.

A comunicação física agora faz uso extensivo do barramento **I2C**, incluindo:
- **PCF8574:** Para conversão dos 7 fios do Teclado Matricial 4x3 em apenas 2 fios lógicos + 1 interrupção (`GPIO 6`).
- **INA219 (x2):** Sensores de tensão/corrente independentes (`0x40` e `0x41`) para auditar as travas eletromagnéticas das portas P1 e P2 contra sabotagem de relés presos.

---

## 2. A Evolução da Interface (Web UI v7)

Abandonamos o motor *Pygame* em prol de uma **Single Page Application (SPA)** hospedada em FastAPI (Python 3.11) rodando no ambiente Linux da Orange Pi.

- **Estética "Apple Style":** Design premium *Glassmorphism*, tipografia SF Pro/Inter, e animações em 60fps.
- **Orientação Portrait:** Otimizada nativamente para displays de 7 polegadas montados na vertical (600x1024).
- **Feedback em Tempo Real:** Conexão contínua via **WebSockets**. Se uma tecla for pressionada na calçada, o ESP32 avisa a Orange Pi por USB, e o WebSocket atualiza a UI injetando os asteriscos (`*`) da senha instantaneamente na tela.
- **Gestão de Energia Automática:** O *DPMS* do Linux é acionado via sysfs para desligar fisicamente os pixels do LCD durante o estado de `IDLE`.

---

## 3. Os Estados do Ambiente (A FSM em Ação)

O estado da portaria não é gerido por *flags* soltas e confusas. A `StateMachine` espelha as exatas diretivas da experiência humana contendo estados rígidos de transição:

1.  **`IDLE`**: Silêncio visual e tela desligada para economia de energia.
2.  **`AWAKE`**: Despertar imediato. Instrução de leitura com loader.
3.  **`WAITING_QR`**: Leitura de códigos da transportadora com scanline animada.
4.  **`WAITING_PASS`**: Digitação de senha segura (Moradores e Logística Reversa) ocultando a quantidade de dígitos com `*`.
5.  **`AUTHORIZED`**: Confirmação visual verde "ACESSO LIBERADO".
6.  **`INSIDE_WAIT`**: Orientação de depósito na balança.
7.  **`DELIVERING`**: Feedback em tempo real do peso adicionado.
8.  **`CONFIRMING`**: Estabilização do peso e assinatura criptográfica JWT.
9.  **`RECEIPT`**: Exibição visual do código final do pacote.
10. **`DOOR_REOPENED`**: Monitoramento de saída do ambiente restrito via mmWave.
11. **`RESIDENT_P1` / `RESIDENT_P2`**: Intertravamento e saudações VIP para condôminos usando as chaves `P1` e `P2`.
12. **`REVERSE_PICKUP`**: Lógica condicional de coleta reversa (Amazon, MercadoLivre, Correios).
13. **`DOOR_ALERT`**: Bloqueio sonoro de porta fisicamente travada aberta.
14. **`ERROR` / `MAINTENANCE`**: Bloqueio integral do painel por detecção de falha vital em sensor (Health Monitor do ESP32).

---

## 4. O Sistema Nervoso Distribuído (Edge e Segurança)

### Orquestração por Systemd
A máquina host (Orange Pi) carrega duas threads principais do sistema (Serviço de Backend API + Serviço de Front Kiosk Mode). Tudo se inicia autonomamente, mesmo em apagões elétricos. O resfriamento do painel é garantido por um cooler interno (12V) filtrado, também gerenciado pelo ESP32.

### Criptografia e Prova Criptográfica
Para atestar a imutabilidade do depósito, o microcontrolador (`JwtSigner.cpp`) sela em JWT um manifesto indicando precisamente o peso, o selo e o payload do código transportador.

### Time-Sync USB e RTC Abstrato
A Orange Pi coordena sua rede de relógios via SNTP no backend Linux e descarrega a estampa de tempo unix (`CMD_SYNC_TIME`) para o ESP32 a cada 10 minutos. O ESP32, então, usa sub-milisegundos nativos para manter a hora local perfeita sem necessitar de chips RTC (DS3231) vulneráveis.

---

## 5. Compilação e Deploy

Toda a infraestrutura é controlada nativamente via **PlatformIO**.

1. Clone este repositório.
2. Abra a pasta raiz no seu **VS Code** com a extensão do *PlatformIO* ativada.
3. O `platformio.ini` está configurado para o ambiente `esp32-s3-devkitc-1` com `qio_opi` (Flash e PSRAM ativadas e otimizadas).
4. Basta clicar na seta de upload e conectar seu módulo.
5. Para o diagrama físico de montagem elétrica e de hardware detalhada dos quadros, confira o [MANUAL.md](./MANUAL.md) disponível neste repositório.

> **AVISO DE FLASHEO:** Os pinos GPIO 43 e 44 são canais seriais do RX/TX primário e são usados no processo de Bootloader inicial do ESP32. Desconecte o scanner *GM861S* fisicamente durante o primeiro flash da placa.

---
*rlight.com.br - "Feito para funcionar, não para incomodar."*
