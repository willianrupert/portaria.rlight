# RLight Portaria Autônoma — Compound Engineering Memory

Este documento serve como memória persistente ("Claude Code style") para o Agente de IA, mantendo o contexto arquitetural, regras de design e histórico de decisões entre as sessões.

## 1. Identidade e Papel do Agente
Você é o engenheiro principal focado na transição da V6 para a V7 do firmware/host do sistema **rlight**.
**Abordagem:** Compound Engineering — divida problemas complexos em etapas atômicas, validadas incrementalmente, mantendo o histórico de progresso.

## 2. Contexto do Projeto (v7)
- **Hardwares Envolvidos:** ESP32-S3 (Firmware AMP em C++) + Orange Pi Zero 3 (Host Linux Python 3).
- **Core Principle:** "Physics-first" -> hardware não bloqueante, FSM determinística.
- **Novidades V7:** 
  - Controle de Acesso via TF9S (Wiegand).
  - Dois Strikes separando as lógicas (P1 para entrega/reversa, P2 exclusivo para morador).
  - Nova Interface Gráfica (1024x600 IPS).

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
  - [x] Integração do sensor Wiegand/TF9S.
  - [x] Lógica de intertravamento P1 + P2 (Dois Strikes).
  - [x] Broadcast de telemetria estendida (Carrier, Resident Label).
- **[x] Deploy e Repositório:**
  - [x] Git inicializado e linkado ao repositório remoto oficial.
  - [x] Push realizado com sucesso.

---
*Regras do Agente:* Atualize este arquivo sempre que um novo marco arquitetural for superado ou uma nova restrição técnica descobrir-se.
