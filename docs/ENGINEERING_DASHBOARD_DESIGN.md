# Projeto: Aba de Engenharia (Diagnostic Dashboard) - RLight Portaria v8

Este documento detalha a arquitetura, interface e fluxo de dados para a nova aba de diagnóstico no PWA da portaria.

## 1. Visão Geral
A Aba de Engenharia permite que técnicos e administradores monitorem a saúde física da portaria em tempo real, identifiquem falhas de hardware e validem o funcionamento de sensores/atuadores sem a necessidade de acesso físico ou cabos seriais.

## 2. Interface (UI Design)

### Estética
- **Tema**: Dark Mode High-Tech (Glassmorphism).
- **Cores**: Acento em `#00ff88` (OK), `#ff4444` (FALHA), `#3399ff` (INFO).
- **Tipografia**: Mono-espaçada para dados técnicos.

### Seções da Tela
#### A. Header de Saúde (Health Score)
- Um gauge circular grande mostrando o `systemScore` (0 a 100).
- Se score < 100, listar em texto quais componentes estão OFFLINE.

#### B. Grid de Periféricos (Status dos Sensores)
Cards individuais com indicadores de cor:
- **GM861S (QR)**: [ONLINE/OFFLINE] | Última leitura.
- **HX711 (Balança)**: [ONLINE/OFFLINE] | Peso atual (g).
- **LD2410 (mmWave)**: [PRESENÇA/VAZIO] | Nível de sinal.
- **PCF8574 (Teclado)**: [ALIVE/DEAD] | Última tecla.
- **INA219 P1/P2**: [V / mA] | Consumo em tempo real.

#### C. Estado das Entradas (Inputs Digitais)
Indicadores binários (Leds virtuais):
- **Porta P1**: 🚪 Aberta / Fechada
- **Porta P2**: 🚪 Aberta / Fechada
- **Portão Garen**: 🚧 Aberto / Fechado
- **Botão Portaria**: 🔘 Pressionado / Livre
- **Botão Interno**: 🔘 Pressionado / Livre

#### D. Console do Sistema (Terminal)
- Área de texto com scroll automático.
- Filtros por nível: `DEBUG`, `SYSTEM`, `AUTH`.
- Botão "Limpar" e "Pausar Scroll".

## 3. Fluxo de Dados (Arquitetura)

Para evitar tráfego desnecessário, a comunicação seguirá este fluxo:

1.  **ESP32 (C++)**: Envia um JSON de telemetria a cada 500ms via USB-Serial para o Orange Pi.
2.  **Orange Pi (Python)**: Recebe o JSON e mantém um "Estado Atual" na memória.
3.  **WebSocket (Socket.IO)**: 
    - O PWA só se conecta ao socket quando a Aba de Engenharia é aberta.
    - O backend só emite eventos de telemetria se houver clientes conectados na "sala de engenharia".
    - **Tráfego**: ~5kbps (desprezível para redes locais/Wi-Fi).

## 4. Periféricos Mapeados

O dashboard deve contemplar absolutamente todos os itens da `Config.h`:

| Periférico | Interface | Pinos (ESP32) | Dados a Exibir |
| :--- | :--- | :--- | :--- |
| **Balança** | HX711 | 41, 42 | Gramas, CalFactor |
| **QR Reader** | Serial0 | 43, 44 | String RAW, Handshake |
| **mmWave** | Serial1 | 17, 18 | Frame Rate, Presença |
| **Teclado** | I2C | 21, 38 | Status do Barramento |
| **INA219 P1** | I2C | 0x40 | Corrente da Trava 1 |
| **INA219 P2** | I2C | 0x41 | Corrente da Trava 2 |
| **Reeds** | Digital | 4, 5, 13 | Estado Real (High/Low) |
| **Atuadores** | Digital | 15, 16, 14 | Estado do Relé/MOSFET |
| **Resfriamento** | Digital | 47 | Cooler (On/Off) |

## 5. Próximos Passos Sugeridos
1.  Criar o endpoint WebSocket no `host/local_api.py`.
2.  Desenvolver o componente `EngineeringTab.vue` ou `.js` no Front-end.
3.  Testar a desconexão física e validar se o card muda para VERMELHO instantaneamente no celular.
