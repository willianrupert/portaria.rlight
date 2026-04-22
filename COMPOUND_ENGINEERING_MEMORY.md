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

## 3. Diretrizes da Interface (Web UI)
- **Resolução:** 1024x600 Horizontal.
- **Ecossistema:** "Apple Style", moderno, minimalista, *glassmorphism*, tipografia limpa (Google Fonts: Inter ou Outfit).
- **Gerenciamento de Energia:** A tela IPS fica DESTRIGADA (apagada) no estado IDLE.
- **Editabilidade:** Os componentes e mensagens (ex: `dynamic_texts` ou templates de HA) devem ser passíveis de alteração pelo Home Assistant via MQTT e propagação para o front-end. O front-end usa HTLM/CSS limpos e otimizados para SBCs de baixa potência.

## 4. Histórico de Versões & Progresso
- **[ ] Migração de UI (Pygame -> Web):** 
  - Substituir o uso de `pygame` e `kmsdrm` do arquivo `host/main.py`.
  - Criar um frontend HTML/CSS/JS desacoplado, que se comunicará via WebSockets/API com o host Python FastAPI (`local_api.py`).
  - Executar o navegador Chromium em modo Kiosk apontado para a interface servida.
- **[ ] Atualização Firmware ESP32-S3 (v7):**
  - Integração do sensor Wiegand.
  - Lógica P1 + P2.
  - Rate limiting e cooldown.

---
*Regras do Agente:* Atualize este arquivo sempre que um novo marco arquitetural for superado ou uma nova restrição técnica descobrir-se.
