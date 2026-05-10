# RLight Portaria Autônoma — Compound Engineering Memory - RLight v8

## Decision Log (2026-04-23)
- **Security**: Migrated to HMAC-SHA256 (mbedtls) for receipt signing on ESP32-S3. Replaced mockups with real JWT verification.
- **Vision**: Replaced `pygame` with `OpenCV` on Orange Pi Host for faster, zero-write (RAM) camera snapshots.
- **Cloud**: Deployed Node.js validator on Oracle Cloud with NGINX reverse proxy and SSL (Certbot).
- **UX**: Implemented Apple-style Glassmorphism UI with adaptive theme support (Light/Dark) and full immersion for iPhone 16 Pro Max.
10: 
11: ## Decision Log (2026-05-05) - Industrial Physical Layer (v8-ready)
12: - **Connectors**: Total deprecation of RJ45 for physical sensors. Switched to 5x 6-pin Aviation GX16 connectors for high-vibration/dust immunity.
13: - **Switching**: Migrated Strikes and Cooler to MOSFET modules to prevent relay contact pitting and EMI from inductive kickback. Reserved 1x mechanical relay for the Gate Motor (dry contact).
14: - **Safety**: Hard-coded ban on GPIOs 33-37 for S3 modules with Octal PSRAM. Remapped sensors to safe pins (GPIOs 4, 5, 7, 12, 13, 14).
15: - **Documentation**: Established `MANUAL.md` and `generate_manual.js` as the source of truth for wiring, removing ambiguity between software definitions and physical assembly.
16: - **Monitoring**: Expanded from 2 to 3 Reed Switches (P1, P2, and Main Gate) and added Internal Release Button support.

## Architecture Highlights
1. **Edge Signing**: ESP32 signs a JSON payload with a shared secret.
2. **Local Capture**: Host captures image on trigger and generates a local link.
3. **Cloud Validation**: User scans QR, cloud server verifies signature and displays history/details.

Este documento serve como memória persistente ("Claude Code style") para o Agente de IA, mantendo o contexto arquitetural, regras de design e histórico de decisões entre as sessões.

## 1. Identidade e Papel do Agente
Você é o engenheiro principal focado na transição da V6 para a V7 do firmware/host do sistema **rlight**.
**Abordagem:** Compound Engineering — divida problemas complexos em etapas atômicas, validadas incrementalmente, mantendo o histórico de progresso.

## 2. Contexto do Projeto (v7)
- **Hardwares Envolvidos:** ESP32-S3 (Firmware AMP em C++) + Orange Pi Zero 3 (Host Linux Python 3).
- **Core Principle:** "Physics-first" -> hardware não bloqueante, FSM determinística.
- **Novidades V7:** 
  - Controle de Acesso via Teclado Matricial I2C (PCF8574).
  - Dois Strikes separando as lógicas (P1 para entrega/reversa, P2 exclusivo para morador).
  - Nova Interface Gráfica (1024x600 IPS).
- **Novidades V8 (EM DESENVOLVIMENTO):**
  - Integração de Motor de Portão Garen Niid via pulso de botoeira.
  - Monitoramento de 3 pontos via Reed Switches (Portão, P1, P2).
  - Botão Interno para liberação imediata da P2 (Morador saindo).
  - Substituição total de Micro Switches por Reed Switches magnéticos.

## 3. Diretrizes da Interface (Web UI v7)
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
  - [x] Push realizado com sucesso.
- **[x] Implementação V8 (Industrial Tier):**
  - [x] Adição de suporte ao Portão Garen (Relé/Pulso no GPIO 14).
  - [x] Implementação de lógica de Botão Interno P2 (GPIO 12).
  - [x] Migração de monitoramento de portas para 3x Reed Switches (GPIO 4, 5, 13).
  - [x] Saneamento de GPIOs (Remapeamento 33-37 -> Safe Pins).
  - [x] Criação do Manual de Engenharia Industrial (`MANUAL.md`).
  - [x] Implementação de script gerador de documentação DOCX/PDF profissional.
  - [x] Padronização de conectores: 1x 16-pin (Main) + 5x 6-pin (Peripherals) Aviation Connectors.
  - [x] Atualização do `README.md` e sincronização final de repositório.

---
*Regras do Agente:* Atualize este arquivo sempre que um novo marco arquitetural for superado ou uma nova restrição técnica descobrir-se.
