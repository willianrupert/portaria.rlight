# Manual de Engenharia e Montagem Física - RLight v7 (Padrão Industrial)

Este é o documento normativo de engenharia para a montagem e instalação do **RLight Portaria Autônoma v7**. Ele detalha os diagramas de fiação, lista de materiais, chicotes de rede e esquemas de proteção elétrica em nível profissional.

> [!WARNING]
> **Alterações Críticas (Maio/2026):** 
> 1. O protocolo **Wiegand foi totalmente descontinuado**. O acesso via teclado foi unificado através de um **Teclado Matricial I2C (PCF8574)**.
> 2. Pinos GPIO 33 ao 37 são **PROIBIDOS** no módulo ESP32-S3 WROOM-2 N8R8 (conflito fatal com a Octal PSRAM). Os botões foram remapeados para os pinos seguros 4, 5 e 7.

---

## 1. Bill of Materials (Lista de Materiais - BoM)

Abaixo estão os componentes técnicos exatos para a construção da portaria:

### Processamento e Lógica
*   **1x Single Board Computer:** Orange Pi Zero 3 (Allwinner H618, 1.5GHz, 2GB LPDDR4).
*   **1x Microcontrolador IO:** Módulo ESP32-S3 WROOM-2 N8R8 (8MB PSRAM Octal, 8MB Flash).
*   **1x Módulo RTC:** DS3231 I2C Precision RTC (Conectado ao Orange Pi).
*   **1x Placa de Expansão (Breakout Board):** Placa Base para ESP32 de 38 Pinos.
*   **1x Cartão de Memória:** MicroSD 32GB Classe 10 A1 (Industrial preferencialmente, ex: SanDisk High Endurance).

### Sensores e Módulos de Comunicação
*   **1x Leitor de Código de Barras/QR:** Módulo Scanner Hangzhou Grow Technology **GM861S** (UART TTL/USB).
*   **1x Radar de Onda Milimétrica (mmWave):** Hi-Link **LD2410B** 24GHz.
*   **1x Teclado Matricial:** Teclado de Membrana ou Plástico ABS 4x3 (12 Teclas).
*   **1x Expansor de I/O I2C:** Módulo **PCF8574** (Para o teclado matricial).
*   **1x Balança de Carga:** Módulo Amplificador A/D de 24 bits **HX711**.
*   **4x Células de Carga:** Strain Gauges de 50kg cada (Formando uma ponte de Wheatstone para 200kg totais).
*   **2x Sensores de Tensão/Corrente I2C:** Texas Instruments **INA219** (Monitoramento anti-sabotagem das travas).
*   **3x Sensores Magnéticos:** Reed Switch NA com imã (Sensores de porta P1, P2 e Portão Principal).

### Atuadores e Interface
*   **1x Display:** Tela LCD TFT 7" polegadas (1024x600) com interface micro-HDMI.
*   **1x Módulo Relé de 1 Canal:** Relé de contato seco isolado (Apenas para pulso na botoeira do motor do portão).
*   **Módulos MOSFET (Isolamento Óptico):** Placa de acionamento em estado sólido (PWM/DC) para controle das Travas Strike P1, P2, do Cooler 40mm e a modulação (PWM) do LED Halo do botão externo, sem desgaste mecânico.
*   **2x Travas Eletromagnéticas:** Fechadura "Strike" Fail-Secure 12V (Abre apenas com pulso elétrico).
*   **1x Botão Push-Button Metálico:** 16mm/19mm NA com LED Halo iluminado (5V/12V).
*   **1x Botão Push-Button (Saída):** Botão NA para liberação interna da porta P2.
*   **1x Buzzer Ativo:** 5V para alertas sonoros.
*   **1x Cooler com Filtro:** Micro-ventilador DC 12V 40x40x10mm com tela filtro de poeira (Para refrigeração do quadro).
*   **2x Diodos Retificadores:** 1N4007 (Para Diodo Flyback das fechaduras magnéticas).

### Infraestrutura e Energia
*   **1x Fonte de Lógica:** Fonte Chaveada Mean Well RS-25-5 (5V / 5A) - Estabilidade absoluta.
*   **1x Fonte de Potência:** Fonte Chaveada Mean Well RS-50-12 (12V / 4.2A).
*   **1x Conector Multivias Principal:** Conector Circular Aviação GX16 de 16 Pinos (Macho e Fêmea).
*   **5x Conectores Multivias Periféricos:** Conector Circular Aviação de 6 Pinos (Macho e Fêmea).
*   **Vários:** Cabos de Rede Cat5e FTP, Fios cabinho AWG 22 (para jumper).

---

## 2. Topologia de Ambientes

*   **Ambiente A (Fachada Externa):** Painel da rua (Tela 7", GM861S, Teclado+PCF8574, Botão Metálico, Buzzer).
*   **Ambiente B (Porta/Grade):** Travas (Strikes) e os sensores magnéticos (Reed Switches).
*   **Ambiente C (Central Interna):** Quadro de comando hermético contendo Orange Pi, Breakout Board ESP32-S3, Fontes, Relés, INA219 e o Cooler de 40mm.

---

## 3. Ambiente C: O Núcleo do Sistema (Painel Interno)

Alojado em um quadro hermético na área interna da casa.

### 3.1. Arquitetura de Energia e Refrigeração
*   **Fonte 1 Lógica (5V):** Alimenta Orange Pi, Breakout do ESP32-S3 e periféricos do Ambiente A.
*   **Fonte 2 Potência (12V):** Alimenta as Travas e o **Cooler 12V 40mm**.
*   **Aterramento:** Conecte o cabo GND da Fonte 5V com o GND da Fonte 12V (Terra Comum da Breakout Board).
*   **Cooler de 40mm (Filtro de Poeira):** 
    *   Posicionado na lateral inferior do quadro "jogando ar frio para dentro".
    *   **Pino VCC (12V):** Ligado à saída NA (Normalmente Aberto) do Módulo Relé/MOSFET.
    *   **Controle (Relé):** Acionado pelo ESP32-S3 via **`GPIO 47`**.

### 3.2. Cérebro: Orange Pi e ESP32-S3 (Breakout Board)
A Breakout Board resolve problemas de soldagem, oferecendo bornes de parafuso. Use os GNDs da Breakout para aterrar todos os shields I2C.
*   **Comunicação (Native USB CDC):** Use um cabo USB com os condutores de dados descascados ligados à Breakout.
    *   Orange Pi porta USB -> ESP32-S3 `GPIO 19` (D-) e `GPIO 20` (D+).
*   **Conectividade Externa:** Cabo de rede rígido ligado ao RJ45 da Orange Pi. (Wi-Fi proibido).

---

## 4. O Umbilical da Fachada: Conector Aviação de 16 Pinos

Todo o sinal do **Ambiente A** para o **Ambiente C** passa pelo Conector GX16 (16 pinos). 
*(Nota: O cabo de vídeo micro-HDMI da tela LCD deve ser passado separadamente na tubulação, possuindo blindagem própria).*

**Mapeamento Absoluto (GX16):**
1.  **Livre / Reserva** (Antigo USB D+)
2.  **Livre / Reserva** (Antigo USB D-)
3.  **UART0 RX** -> ESP32 `GPIO 44` (Conecta ao TX do GM861S)
4.  **UART0 TX** -> ESP32 `GPIO 43` (Conecta ao RX do GM861S)
5.  **I2C SDA** -> ESP32 `GPIO 21` (Conecta ao SDA do PCF8574)
6.  **I2C SCL** -> ESP32 `GPIO 38` (Conecta ao SCL do PCF8574)
7.  **KEYPAD INT** -> ESP32 `GPIO 6` (Conecta ao pino INT do PCF8574)
8.  **LED BTN** -> ESP32 `GPIO 39` (Liga o Halo Ring do Botão Metálico via Resistor 220Ω)
9.  **LED QR** -> ESP32 `GPIO 40` (Iluminação extra do leitor, se usado pin auxiliar)
10. **BUZZER** -> ESP32 `GPIO 48` (Conecta ao I/O ou positivo do Buzzer Ativo 5V)
11. **BUTTON** -> ESP32 `GPIO 7` (Sinal do contato NA do Botão Metálico. Puxado pra GND quando apertado).
12. **GND Lógica (Terra)**
13. **GND Lógica (Reforço/Malha)**
14. **VCC 5V Lógica** (Alimentação do GM861S, PCF8574 e Tela de 7" caso alimentada via cabo DC).
15. **VCC 5V Lógica (Reforço)**
16. **Livre / Reserva**

---

## 5. Chicotes Periféricos (Conectores Aviação 6 Pinos)

Os periféricos do Ambiente B e os módulos isolados usam cabos Cat5e FTP, mas são ligados à Central C através de **5x Conectores tipo Aviação de 6 Pinos**, garantindo uma fixação mecânica imune a vibrações (RJ45 foi descontinuado).

### 5.1. Chicote 1 — Radar mmWave Hi-Link LD2410B
O LD2410B detecta presença humana por ondas milimétricas de 24GHz. Possui 5 pinos funcionais, mas usamos apenas 4.
*   **Pino 1:** LD2410B `VCC` (5V Lógica)
*   **Pino 2:** LD2410B `GND`
*   **Pino 3:** LD2410B `RX` -> ESP32 `GPIO 17` (UART1 TX)
*   **Pino 4:** LD2410B `TX` -> ESP32 `GPIO 18` (UART1 RX)
*   **Pino 5 e 6:** Não utilizados

O pino OUT do radar foi ignorado pois o sistema usa o protocolo UART serial completo.

### 5.2. Chicote 2 — Balança HX711 + Células de Carga
A balança usa quatro células de carga de 50kg. O amplificador HX711 DEVE ficar fisicamente junto das células para evitar ruído.
*   **Fiação na base da balança:** `E+` (Vermelho), `E-` (Preto), `A-` (Branco), `A+` (Verde).
*   **Conector Aviação 6 Pinos (Painel C):**
    *   **Pino 1:** HX711 `VCC` (5V Lógica)
    *   **Pino 2:** HX711 `GND`
    *   **Pino 3:** HX711 `SCK` -> ESP32 `GPIO 41`
    *   **Pino 4:** HX711 `DT` -> ESP32 `GPIO 42`
    *   **Pino 5 e 6:** Não utilizados

### 5.3. Chicote 3 — Porta Externa P1 (Trava + Reed P1)
*   **Pino 1:** Strike VCC (+12V da Fonte)
*   **Pino 2:** Strike GND (Sincronizado ao Dreno do MOSFET)
*   **Pino 3:** Reed Switch P1 GND (Terra Lógica 5V)
*   **Pino 4:** Reed Switch P1 Sinal -> ESP32 `GPIO 4`
*   **Pino 5 e 6:** Não utilizados

### 5.4. Chicote 4 — Porta Interna P2 (Trava + Reed P2 + Botão Interno)
*   **Pino 1:** Strike VCC (+12V da Fonte)
*   **Pino 2:** Strike GND (Sincronizado ao Dreno do MOSFET)
*   **Pino 3:** GND Comum Lógica 5V (Para o Reed e para o Botão Interno)
*   **Pino 4:** Reed Switch P2 Sinal -> ESP32 `GPIO 5`
*   **Pino 5:** Botão Interno P2 Sinal -> ESP32 `GPIO 12`
*   **Pino 6:** Não utilizado

### 5.5. Chicote 5 — Portão Principal (Garen)
O motor do portão requer apenas um pulso de contato seco (sem injetar tensão), acionado pelo módulo Relé de 1 Canal.
*   **Pino 1:** Relé NO (Normalmente Aberto) -> Entrada BOT do Motor Garen
*   **Pino 2:** Relé COM (Comum) -> Entrada GND do Motor Garen
*   **Pino 3:** Reed Switch Portão GND (Terra Lógica 5V)
*   **Pino 4:** Reed Switch Portão Sinal -> ESP32 `GPIO 13`
*   **Pino 5 e 6:** Não utilizados

> [!CAUTION]
> **Diodo Flyback Obrigatório:**
> É mandatório soldar o Diodo `1N4007` **junto à fechadura (na porta)**, em paralelo com a bobina, inversamente polarizado: Catodo (faixa prata) no fio de 12V, e Anodo no fio de GND. Isso absolve a descarga indutiva da bobina. Sem ele, o MOSFET pode entrar em curto-circuito (avalanche térmica) e a Fonte 12V joga ruído elétrico na Breakout Board.

---

## 6. Pinagem Interna Detalhada dos Módulos

### 6.1. Leitor QR Code (GM861S)
*   `VCC` -> +5V (Pino 14/15 da Aviação)
*   `GND` -> Terra (Pino 12/13 da Aviação)
*   `RXD` -> Fio UART0 TX (Pino 4 da Aviação)
*   `TXD` -> Fio UART0 RX (Pino 3 da Aviação)

### 6.2. Teclado Matricial e PCF8574
O PCF8574 converte os 7 fios da membrana 4x3 em um barramento I2C inteligente.
*   **Fiação Teclado -> PCF8574:**
    *   `Linha 1 a 4` -> PCF8574 `P0`, `P1`, `P2`, `P3`
    *   `Coluna 1 a 3` -> PCF8574 `P4`, `P5`, `P6`
*   **Fiação PCF8574 -> Aviação:**
    *   `VCC` -> +5V Lógica
    *   `GND` -> Terra
    *   `SDA` -> Pino 5 Aviação (Para GPIO 21)
    *   `SCL` -> Pino 6 Aviação (Para GPIO 38)
    *   `INT` -> Pino 7 Aviação (Para GPIO 6) - **Sinal de Active LOW quando a tecla for pressionada**.

### 6.3. Monitores de Corrente da Fechadura (INA219 x 2)
Ficam localizados no Painel C. Usados para confirmar via sensor de Hall/Shunt que a energia fluiu de fato para a trava, impedindo sabotagem por relé preso.
*   **Alimentação Lógica (Ambos):** `VCC` a 3.3V (do breakout do ESP32) e `GND` ao Terra.
*   **I2C (Ambos):** `SDA` e `SCL` vão em paralelo para o `GPIO 21` e `GPIO 38`.
*   **Circuito de Potência P1 (Endereço 0x40):** O fio positivo 12V da Fonte entra em `Vin+` e sai pelo `Vin-` rumo ao Strike P1.
*   **Circuito de Potência P2 (Endereço 0x41):** É **OBRIGATÓRIO** soldar a ponte no pad `A0` na placa do módulo para que o barramento assuma endereço alternativo. O fio positivo 12V entra em `Vin+` e sai pelo `Vin-` rumo ao Strike P2.

> [!CAUTION]
> **Integridade de Barramento (Bypass e Pull-ups):**
> Adicione **resistores pull-up de 4.7kΩ** nos terminais `SDA` e `SCL` puxando para `3.3V` nos bornes da placa Breakout Board do ESP32. Além disso, solde pequenos capacitores de bypass cerâmicos (`100nF`) perto dos pinos VCC e GND dos chips PCF8574 e INA219. Essas práticas suprimem EMI em chicotes longos e mantêm o I2C com 100% de confiabilidade.

### 5.4 Relógio de Tempo Real (RTC DS3231)
Para garantir que os logs e JWTs tenham o horário correto mesmo sem internet, o DS3231 deve ser ligado ao **Orange Pi Zero 3**:

| Pino DS3231 | Pino Orange Pi (Header 26p) | Função |
| :--- | :--- | :--- |
| **VCC** | Pin 1 | 3.3V |
| **GND** | Pin 9 | Ground |
| **SDA** | Pin 3 | I2C1 SDA |
| **SCL** | Pin 5 | I2C1 SCL |

> [!TIP]
> O Host (Python) lê a hora do sistema do Orange Pi e a envia periodicamente (a cada 10 min) para o ESP32 sincronizar seu clock interno.
