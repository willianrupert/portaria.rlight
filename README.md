# RLight — Uma Portaria que Pensa (v8.0)

Bem-vindo ao repositório oficial do **RLight Portaria Autônoma v8**.
Esta documentação reflete a evolução arquitetural definitiva do projeto, consolidando um ecossistema **Web SPA de Alta Performance** em conjunto com o poderoso microcontrolador **ESP32-S3 (Octal PSRAM)**.

A entrega não termina quando o entregador vai embora. Ela termina quando o morador tem a certeza — criptograficamente verificável — de que o pacote está seguro. Este projeto é o sistema nervoso autônomo que torna isso possível.

---

## 1. Visão Arquitetural Implementada (AMP)

Para lidar com a complexidade de múltiplos sensores em tempo real e atuadores mecânicos sem bloqueios de CPU, o firmware v8 utiliza o paradigma de **Processamento Assimétrico (AMP)** no FreeRTOS:

*   **Core 1 (`taskSensorHub`):** Roda a 20Hz. Gerencia o "sistema sensorial": Balança (HX711), Radar mmWave (LD2410B), Scanner QR (GM861S), Teclado I2C e Monitores de Corrente (INA219).
*   **Core 0 (`taskLogicBrain`):** Roda a 100Hz. O cérebro do sistema. Executa a **StateMachine não-bloqueante**, processa comandos USB CDC e gerencia a criptografia JWT em tempo real.

### 1.1. Inovações da Versão 8
- **Zero-Heap Strategy**: Eliminação total da classe `String` no firmware para garantir estabilidade infinita e prevenir fragmentação de memória.
- **System Health Score (0-100)**: Monitoramento proativo da integridade de hardware. Se um sensor falha (ex: balança desconectada), o sistema degrada o score e avisa o Dashboard via telemetria.
- **Interbloqueio Físico**: Lógica de segurança de interlock que impede a abertura da porta interna (P2) se a externa (P1) estiver aberta, garantindo integridade do perímetro.
- **Botão de Saída P2**: Novo input físico dedicado (`GPIO 12`) para liberação imediata de saída de moradores.

---

## 2. Interface SPA e Orquestração (Host OPi)

O host (Orange Pi Zero 3) atua como o terminal inteligente, rodando uma **SPA (Single Page Application)** moderna:

- **FastAPI + WebSockets**: Comunicação ultra-rápida entre o Hardware e a UI. Eventos como `KEY_PRESSED` são refletidos na tela instantaneamente.
- **Persistência de Produção**: Banco de dados SQLite padronizado em `/var/lib/rlight/deliveries.db` com sistema de logs e nomenclatura de fotos por UUID de token.
- **Segurança HMAC Rotativa**: Verificação de tokens JWT no Cloud Backend (`server.js`) com suporte a segredos ativos e anteriores, permitindo rotação de chaves sem invalidar entregas em curso.
- **Relatórios SMTP**: Sistema de notificação semanal automatizado via SMTP seguro para prestação de contas aos moradores.

---

## 3. Estados da FSM (V8)

A `StateMachine` v8 é robusta e resiliente a falhas:

1.  **`IDLE`**: Standby profundo com tela desligada (DPMS).
2.  **`AWAKE` / `WAITING_QR`**: Início do fluxo de entrega e leitura de pacotes.
3.  **`WAITING_PASS`**: Entrada de senha via teclado I2C com feedback visual de "dots" na UI.
4.  **`LOCKOUT_KEYPAD`**: Bloqueio temporário (5 min) após sucessivas falhas de senha.
5.  **`AUTHORIZED`**: Liberação de P1 com validação de corrente do Strike (INA219).
6.  **`DELIVERING`**: Monitoramento de peso e presença durante o depósito.
7.  **`CONFIRMING` / `RECEIPT`**: Assinatura criptográfica JWT e exibição do comprovante.
8.  **`DOOR_ALERT`**: Alerta sonoro e visual se portas forem deixadas abertas.
9.  **`MAINTENANCE`**: Modo de diagnóstico que permite testar atuadores via Home Assistant.

---

## 4. Deploy e Produção

### 4.1. Serviços Systemd
O sistema é gerido por serviços nativos do Linux, garantindo resiliência:
- `rlight-host.service`: Orquestrador Python e Ponte USB.
- `rlight-cloud.service`: Backend Node.js para gestão remota e validação.

### 4.2. Integração Home Assistant
Configuração automática (Discovery) de entidades v8:
- Sensores binários de porta (P1, P2, Portão).
- Sensor de saúde (System Health Score).
- Controles de configuração dinâmica (Timeouts, Brilho, Delays).

---

## 5. Compilação (PlatformIO)

O projeto v8 é otimizado para o ambiente `esp32-s3-devkitc-1` com Flash/PSRAM Octal:

1.  Configure as credenciais em `Config.h`.
2.  Compile e faça o upload via USB CDC.
3.  **Aviso**: Desconecte o scanner QR (`GPIO 43/44`) durante o primeiro flash para evitar conflitos no Bootloader.

Para diagramas elétricos e guia de montagem, consulte o [MANUAL.md](./MANUAL.md).

---
*rlight.com.br - "Feito para funcionar, não para incomodar."*
