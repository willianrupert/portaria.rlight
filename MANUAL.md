# Manual de Montagem Física e Hardware - RLight v7 (Padrão Industrial)

Este é o documento normativo de engenharia para a montagem e instalação do **RLight Portaria Autônoma v7**. Ele detalha todas as conexões, chicotes, chicotes de rede e esquemas de blindagem elétrica.

> [!WARNING]
> **Alterações Críticas (Maio/2026):** 
> 1. O protocolo **Wiegand foi totalmente descontinuado**. O acesso via teclado e RFID foi unificado através de um **Teclado Matricial I2C (PCF8574)**.
> 2. Pinos GPIO 33 ao 37 são **PROIBIDOS** no módulo ESP32-S3 WROOM-2 N8R8 (conflito fatal com a Octal PSRAM). Os botões foram remapeados para os pinos seguros 4, 5 e 7.

---

## 1. Topologia de Ambientes

O projeto físico é dividido em três ambientes principais, isolados para segurança e manutenção:

*   **Ambiente A (Fachada Externa):** Fica na rua. Contém a interface com o entregador/morador (Tela, Câmera, Botões, Teclado).
*   **Ambiente B (Porta/Grade):** A fronteira física. Contém as travas magnéticas (Strikes) e os sensores magnéticos (Reed Switches).
*   **Ambiente C (Central Interna):** Fica seguro dentro de casa. Contém o servidor Orange Pi, o ESP32-S3 e as Fontes de Alimentação.

---

## 2. Ambiente C: O Núcleo do Sistema (Painel Interno)

Alojado em um quadro de comando (ex: caixa hermética de 30x30cm), este ambiente concentra o processamento e a distribuição de energia.

### 2.1. Arquitetura de Energia (Vital)
Para evitar resets do ESP32 devido a *brownouts* (quedas de tensão) ao acionar fechaduras, use a topologia de **Fontes Separadas**:

*   **Fonte 1 (Lógica - 5V/3A):** Alimenta EXCLUSIVAMENTE a Orange Pi Zero 3 e o ESP32-S3.
*   **Fonte 2 (Potência - 12V/5A):** Alimenta as Travas Eletromagnéticas (Strikes) e o Cooler do quadro.
*   **Aterramento:** O pino GND (Terra) da Fonte 1 **DEVE** estar conectado ao GND da Fonte 2. Caso contrário, os MOSFETs/Relés não funcionarão corretamente.

### 2.2. O Cérebro: Orange Pi e ESP32
*   A comunicação entre Orange Pi e ESP32-S3 ocorre via **Native USB CDC**.
    *   `GPIO 19` do ESP32 -> `D-` da USB da Orange Pi.
    *   `GPIO 20` do ESP32 -> `D+` da USB da Orange Pi.
*   **Cabo Ethernet:** Conecte a Orange Pi diretamente ao roteador via cabo Cat5e. Não use Wi-Fi.

### 2.3. Painel de Conexões RJ45
O painel C deve ter saídas modulares usando conectores fêmea RJ45 (Keystones). Isso facilita manutenção e troca de módulos sem solda.

---

## 3. Especificação dos Chicotes RJ45 (Cabeamento Cat5e)

Cabos de rede Cat5e possuem 4 pares trançados (8 fios). Eles ligam a Central (C) aos periféricos isolados usando a seguinte padronização de pinos (T568B):

### Chicote 1: Radar mmWave (LD2410B)
*   **Branco/Laranja:** VCC (5V da Fonte Lógica)
*   **Laranja:** GND
*   **Branco/Verde:** TX do ESP32 (`GPIO 17`) -> RX do Radar
*   **Azul:** RX do ESP32 (`GPIO 18`) -> TX do Radar
*   *(Sobram 4 fios não utilizados)*

### Chicote 2: Balança e Célula de Carga
A balança requer cuidado extremo com interferências analógicas. O chip HX711 fica *junto* da balança, não no quadro.
*   **Cabo Central -> HX711 (Módulo na Balança):**
    *   **Branco/Laranja:** VCC (5V Lógica)
    *   **Laranja:** GND
    *   **Branco/Verde:** CLK do ESP32 (`GPIO 41`)
    *   **Azul:** DATA do ESP32 (`GPIO 42`)
*   **Fiação Interna da Balança (Célula -> HX711):**
    *   E+ (Vermelho), E- (Preto), A+ (Verde), A- (Branco).

### Chicote 3 e 4: Travas (Strikes P1 e P2) e Reed Switches
Para evitar ruídos, passe o sinal de 12V em pares separados do sinal lógico.
*   **Branco/Laranja:** VCC 12V (Fonte Potência) -> Bobina da Trava
*   **Laranja:** GND 12V (Dreno do MOSFET) -> Bobina da Trava
*   **Azul:** GND Lógico -> Um lado do Reed Switch
*   **Branco/Azul:** Sinal Reed Switch -> ESP32 `GPIO 4` (Para P1) ou `GPIO 5` (Para P2)
> [!CAUTION]
> **Diodo Flyback Obrigatório:**
> É mandatório soldar um diodo (ex: 1N4007) invertido nos terminais da fechadura magnética. O anodo no fio GND (Laranja) e o catodo no fio 12V (Branco/Laranja). Isso impede que a energia reversa da bobina queime o ESP32.

---

## 4. O Umbilical da Fachada: Conector Aviação de 16 Pinos

Toda a comunicação entre a Central (C) e a Fachada Externa (A) passa por um único cabo super-manga com conector tipo aviação de 16 pinos.

**Mapeamento Absoluto dos 16 Pinos:**
1.  **USB D+** -> ESP32 `GPIO 20` (Compartilhado/Câmera Opcional)
2.  **USB D-** -> ESP32 `GPIO 19`
3.  **UART0 RX** -> ESP32 `GPIO 44` (Leitor GM861S TX)
4.  **UART0 TX** -> ESP32 `GPIO 43` (Leitor GM861S RX)
5.  **I2C SDA** -> ESP32 `GPIO 21` (PCF8574)
6.  **I2C SCL** -> ESP32 `GPIO 38` (PCF8574)
7.  **KEYPAD INT** -> ESP32 `GPIO 6` (Sinaliza que tecla foi apertada)
8.  **LED BTN** -> ESP32 `GPIO 39` (Feedback botão)
9.  **LED QR** -> ESP32 `GPIO 40` (Iluminador noturno GM861S)
10. **BUZZER** -> ESP32 `GPIO 48` (Som de confirmação/erro)
11. **BUTTON** -> ESP32 `GPIO 7` (Botão de iniciar atendimento)
12. **GND (Sinais Lógicos)**
13. **GND (Par de retorno para reforço)**
14. **VCC 5V** (Alimentação do Painel, Leitor QR, PCF8574)
15. **VCC 5V** (Par de retorno para reforço de corrente)
16. **RESERVA**

> [!NOTE]
> O cabo **HDMI da tela de 7 polegadas** requer alta largura de banda e não pode trafegar pelo conector de aviação. Ele deve ser um cabo micro-HDMI padrão, com fiação de blindagem própria, passado em paralelo na tubulação entre a Orange Pi e a tela.

---

## 5. Módulos Especiais: I2C e INA219

O barramento I2C opera em paralelo com o cabo de 16 pinos.

### Módulo Expansor de Teclado (PCF8574)
*   **Localização:** Ambiente A (Atrás do painel de acrílico).
*   **Endereço Fixo:** `0x20` (Pinos A0, A1 e A2 abertos/desconectados dependendo do módulo).
*   Transforma a leitura matriz de 7 fios do teclado físico em apenas 2 fios lógicos (SCL/SDA) e 1 interrupção (INT).

### Monitores de Corrente da Fechadura (INA219)
Estes sensores asseguram matematicamente se o MOSFET disparou a fechadura (anti-sabotagem).
*   **Localização:** Ambiente C (Dentro do quadro de comando, no fio positivo que vai para os Strikes).
*   **INA219 P1 (Porta Externa):** Endereço `0x40`.
*   **INA219 P2 (Porta Interna):** Endereço `0x41` (É necessário soldar o pad `A0` no módulo para alterar seu endereço de base).

> [!CAUTION]
> **Proteção I2C (Bypass e Pull-ups):**
> Sendo um cabo I2C longo, solde capacitores de bypass cerâmicos de 100nF entre VCC e GND nos pinos do PCF8574 e do INA219. Adicionalmente, coloque **resistores pull-up de 4.7kΩ** nos fios SDA e SCL puxando-os para o VCC de 3.3V, no lado do ESP32. Sem eles, o barramento sofrerá perdas de pacote.
