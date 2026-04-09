# rlight — Uma Portaria que Pensa (Firmware v6)

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

Enquanto o mercado persegue modelos complexos de inteligência artificial baseados puramente em câmeras, o sistema **rlight v6** adota a filosofia "*Physics-first*". Acreditamos que modelos comportamentais falham. Corrente elétrica diferencial e densidade atômica (peso), não.

* **Balança como Árbitro da Verdade (`Scale.cpp`):**
  Uma célula de carga monitorada por um ADC de precisão. Nós otimizamos o código (`RAM Tare only`) garantindo que as operações de autocorreção térmica (Auto-Zero) da balança jamais desgastem a vida útil da memória NVS flash.
* **Presença Inquestionável (`MmWave.cpp`):**
  A integração nativa de Radar 24GHz fornece detecção instantânea que independe de luximetria (iluminação). Nós criamos uma técnica de *Spatial Debouncing* no FSM assegurando que a transição de humano não cause falsos "vazios".
* **Buffer Circular para Entregadores (`QRReader.cpp`):**
  O escanner de código de barras jamais interrompe ou "trava" o microcontrolador esperando o entregador. Letra por letra é recebida de forma serial e analisada assincronamente.

---

## 3. Os Estados do Ambiente — A FSM em Ação

O estado do ambiente não é gerido por *flags* soltas booleanas. A classe modular `StateMachine.cpp` atua contendo **10 estados bem definidos**, e eles espelham as exatas diretivas da experiência humana descritas na visão de produto:

1. **`IDLE` (Dormência):** O sistema pisca suavemente seu LED, efetua calibrações termais na balança e checa falhas ativas.
2. **`AWAKE` (Despertar):** Botão pressionado. Resposta imediata, brilho total e inicialização de tela gráfica no ambiente host (Orange Pi).
3. **`WAITING_QR` (Leitura):** O leitor de QR code busca em modo não-bloqueante as 6 variações rastreáveis de transportadoras nacionais.
4. **`AUTHORIZED` (Autorização):** A trava de P1 inicia liberação paralela (`Strike::P1().open`).
5. **`INSIDE_WAIT` (Presença):** A física acusa que a mola encurtou e a porta P1 bateu. Entrega em andamento.
6. **`DELIVERING` (Depósito):** A balança varia além do fator de ruído ambiente e de variações termais.
7. **`DOOR_REOPENED` (Saída):** A mola aciona saída e a porta passa a se comportar como rota de evacuação.
8. **`CONFIRMING` (Confirmação):** Estabilização. O radar mmWave atesta que o corredor logístico está vázio de tecido humano denso.
9. **`RECEIPT` (Comprovante):** Geração criptográfica com as métricas geradas em silício.
10. **`DOOR_ALERT` / `ERROR`:** Interlock P1+P2, timeout de porta aberta e auto-proteção de curto-circuito nos strikes.

---

## 4. O Sistema Nervoso Distribuído (Edge e Segurança)

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
