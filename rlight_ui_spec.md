# rlight — Especificação Completa de Telas Web
**Interface da Portaria Autônoma · Versão 7.0**
*Resolução alvo: 1024 × 600px · Sem scroll · Fullscreen · Kiosk mode*

---

## Filosofia de Design

A interface do rlight é tratada como um produto Apple — não um painel de automação. A premissa é que um iPad dedicado de última geração está embutido na parede, mas com propósito único e perfeito. Nenhum elemento excessivo. Cada pixel justificado. Cada animação com intenção.

**Tom:** Confiante, limpo, premium. Sem exageros. Sem exclamações desnecessárias.

**Referências visuais:** iOS Lock Screen, Apple Watch complications, iPhone Dynamic Island feedback, macOS Control Center.

---

## Sistema de Design Global

### Paleta de cores

```
--bg-base:        #080810      /* fundo base quase preto */
--bg-glass:       rgba(255,255,255,0.04)
--bg-glass-hover: rgba(255,255,255,0.07)
--border-glass:   rgba(255,255,255,0.08)

--accent-green:   #30D158      /* Apple green — sucesso, online, autorizado */
--accent-yellow:  #FFD60A      /* Apple yellow — atenção, instrução */
--accent-blue:    #0A84FF      /* Apple blue — informação, residente */
--accent-red:     #FF453A      /* Apple red — erro, alerta crítico */
--accent-orange:  #FF9F0A      /* Apple orange — aviso moderado */
--accent-purple:  #BF5AF2      /* Apple purple — reversa, especial */

--text-primary:   rgba(255,255,255,0.92)
--text-secondary: rgba(255,255,255,0.55)
--text-tertiary:  rgba(255,255,255,0.28)
```

### Tipografia

```
Fonte: SF Pro Display (fallback: -apple-system, BlinkMacSystemFont, "Inter", sans-serif)

--font-hero:    72px / weight 700 / letter-spacing -2px
--font-title:   36px / weight 600 / letter-spacing -1px
--font-body:    18px / weight 400 / letter-spacing -0.2px
--font-caption: 13px / weight 500 / letter-spacing 0.4px / uppercase
--font-mono:    14px / "SF Mono", "Menlo", monospace
--font-clock:   48px / weight 200 / letter-spacing -1px  (relógio, ultrafino)
```

### Background animado (global, compartilhado por todas as telas)

O fundo é uma composição de duas camadas:

**Camada 1 — Blobs de luz:**
Dois círculos com `filter: blur(120px)` e `opacity: 0.15`, animados com `@keyframes float` de 8s e 12s respectivamente (easing: ease-in-out, alternate). Posições iniciais aproximadas: verde no canto inferior-esquerdo, azul no canto superior-direito. Em estados de erro/alerta, os blobs transitam suavemente para vermelho/laranja com `transition: background 1.2s ease`.

**Camada 2 — Noise texture:**
Um `SVG feTurbulence` com opacity 0.025 sobreposto ao fundo, dando a textura "grain" característica do glassmorphism Apple. Estático.

**Camada 3 — Glass overlay:**
`backdrop-filter: blur(40px) saturate(180%)` em um div que cobre todo o viewport. Combinado com `background: rgba(8,8,16,0.6)`. Esta é a camada que recebe os componentes de UI.

### HUD Global — Header fixo (presente em TODAS as telas exceto IDLE)

O header é uma barra de `height: 52px` no topo da tela, com `backdrop-filter: blur(20px)` e `border-bottom: 1px solid var(--border-glass)`.

**Lado esquerdo:**
- Logotipo "rlight" em `--font-caption` com `letter-spacing: 3px`, cor `--text-tertiary`.

**Centro:**
- Nome do estado atual em `--font-caption`, cor `--text-tertiary`. Exemplos: "ENTREGA EM CURSO", "AGUARDANDO LEITURA", "MORADOR". Não exibir em IDLE.

**Lado direito (sempre visível, incluindo IDLE em versão reduzida):**
- **Bolinha de status online:** círculo de 8px de diâmetro. Verde `--accent-green` com `box-shadow: 0 0 8px var(--accent-green)` quando online. Laranja `--accent-orange` quando degradado (score < 80). Vermelho `--accent-red` quando offline (sem heartbeat > 90s). Animação `pulse` de 2s quando online.
- **Relógio:** `HH:MM:SS` em `--font-clock` com `font-weight: 200`. Horas e minutos em `--text-primary`, segundos em `--text-secondary` com tamanho reduzido (32px). Atualização a cada segundo via `setInterval`. Separadores `:` piscam em 50% de opacidade a cada meio segundo.
- **Data:** abaixo do relógio, em `--font-caption`, cor `--text-tertiary`. Formato: "QUA, 22 ABR" (dia da semana abreviado em português, dia, mês abreviado).

> **Nota de implementação:** O relógio e o status online são os únicos elementos que existem também na tela IDLE (em versão reduzida, sem o header completo — ver especificação do IDLE abaixo).

---

## Telas por Estado

---

### TELA 1 — IDLE
**Estado FSM:** `IDLE`
**Objetivo:** Quase invisível. Indica vida sem chamar atenção. Tela pode estar com brilho reduzido a 20% via controle DPMS.

**Layout:**
- Fundo com blobs em intensidade mínima (`opacity: 0.06`). Praticamente preto.
- **Elemento central:** Logotipo "rlight" em `--font-hero`, `font-weight: 200` (ultrafino), `opacity: 0.12`. Sem animação de entrada — já está lá, sempre esteve.
- **Ponto de vida:** Logo abaixo do logotipo, um `pulse-dot` de 6px de diâmetro, cor `--accent-green`, com animação `pulse` de 3s e `opacity: 0.6`. É o único elemento animado.
- **Relógio e status:** Posicionados no canto superior direito, mas com `opacity: 0.4` (mais discretos que nas outras telas). O relógio usa `font-size: 28px` aqui, menor.

**O que NÃO tem:** Nenhuma instrução, nenhuma seta, nenhum texto explicativo. Silêncio visual.

**Transição de entrada:** Fade in de 0.8s de toda a tela ao vir de qualquer outro estado.
**Transição de saída:** Fade out de 0.3s antes de qualquer outra tela surgir.

---

### TELA 2 — AWAKE
**Estado FSM:** `AWAKE`
**Ativado por:** Botão 30mm inox pressionado pelo entregador.
**Objetivo:** Chamar atenção imediatamente. Orientar o entregador sobre o que fazer enquanto o sistema aguarda o handshake `SCREEN_READY` do Orange Pi.

**Layout — divisão vertical:**

**Terço superior:**
- Endereço completo do imóvel em `--font-caption`, cor `--text-secondary`. Ex: "Rua Hermes da Fonseca, 315 — CEP 54400-333".
- Abaixo, em `--font-body` com peso 500, cor `--text-tertiary`: "Recebedores autorizados:".
- Nomes dos moradores em `--font-title`, cor `--accent-green`, separados por `·`. Ex: "Willian Rupert  ·  Maria Aparecida".

**Terço central:**
- Texto principal: "LEIA O CÓDIGO DO PACOTE" em `--font-title`, `font-weight: 700`, `--text-primary`. Com `letter-spacing: 1px`.
- Subtexto: "Aproxime a etiqueta de rastreio abaixo" em `--font-body`, `--text-secondary`.

**Terço inferior:**
- **Seta direcional:** Uma seta grande (80px × 80px) apontando para **baixo e para a esquerda (45°)**, animada com `@keyframes pulse-arrow` (translação de 8px na direção da seta, 1.2s, ease-in-out, alternate, infinite). Cor `--accent-yellow`. Estilo iOS — arredondada, com terminação chevron duplo `⟪`.
- Abaixo da seta, o texto "Leitor aqui" em `--font-caption`, cor `--accent-yellow`, opacity 0.8.

**Rodapé:**
- Barra de progresso linear (`height: 2px`, `border-radius: 1px`) em `--accent-yellow`, animada de 100% para 0% ao longo de 30s (timeout para IDLE). Fica no bottom da tela, tocando as bordas laterais.
- Texto acima da barra: "Aguardando leitura · 30s" que atualiza para "Aguardando leitura · 29s"... em `--font-caption`, `--text-tertiary`.

**Animação de entrada:** Os três blocos surgem em sequência com `translateY(12px) → 0 + opacity 0→1`, staggered de 80ms entre cada bloco.

> **Nota:** Esta tela permanece visível durante `AWAKE` e transiciona suavemente para `WAITING_QR` quando o handshake for recebido. O usuário não percebe a diferença — é a mesma tela com uma atualização sutil.

---

### TELA 3 — WAITING_QR
**Estado FSM:** `WAITING_QR`
**Ativado por:** Recebimento de `SCREEN_READY` do OPi.
**Objetivo:** GM861S está ativamente lendo. Reforçar onde aproximar. Timer visual.

**Layout:** Herdado de AWAKE com as seguintes diferenças:

- A seta e o texto "Leitor aqui" recebem um halo de luz `--accent-yellow` ao redor (glow de `box-shadow: 0 0 40px rgba(255,214,10,0.3)`), aumentando a presença visual.
- **Indicador de leitura ativa:** Uma faixa `height: 3px` na borda inferior do painel central, com animação `shimmer` (gradiente que percorre da esquerda para a direita em loop de 1.5s), cor `--accent-yellow`. Indica que o sensor está lendo.
- **Timer countdown:** O rodapé de progresso começa do ponto onde estava em AWAKE — sem reset. Continuidade temporal.

**Popup de fallback — após 15s sem leitura:**
> Esta é a funcionalidade solicitada. Aparece como um card flutuante `glassmorphism` que surge do bottom com `translateY(100%) → 0` em 0.4s cubic-bezier.

Conteúdo do popup:
- Linha 1: "Não está conseguindo?" em `--font-body`, `--text-secondary`.
- Linha 2: "Use o interfone ou ligue" em `--font-caption`, `--text-tertiary`.
- Número de telefone: em `--font-title`, `font-weight: 600`, `--accent-blue`. Configurável via HA.
- Ícone de telefone `☎` à esquerda do número.
- Botão "Chamar interfone" (ghost button, borda `--accent-blue`, texto `--accent-blue`) que pode acionar a campainha Tapo via MQTT quando tocado.

O popup **não substitui** a tela — fica sobreposto no bottom 20% da tela. A seta e o timer continuam visíveis acima dele.

**Fim de timeout (30s):** Fade out completo → transição para IDLE.

---

### TELA 4 — AUTHORIZED
**Estado FSM:** `AUTHORIZED`
**Ativado por:** QR Code válido lido / código Wiegand de reversa recebido.
**Objetivo:** Confirmação inequívoca de sucesso. Instruir ação imediata. Poucos segundos visível — o entregador deve empurrar a porta.

> **Nota arquitetural importante:** Este estado tem dois destinos possíveis — entrega normal (→ INSIDE_WAIT) ou coleta reversa (→ REVERSE_PICKUP). A tela não precisa diferenciar: o ESP32 já sabe qual é qual. A instrução é a mesma: empurre a porta.

**Layout — totalmente centralizado, máxima presença:**

**Elemento âncora:**
- Círculo de `160px` de diâmetro com `background: rgba(48,209,88,0.15)`, `border: 2px solid var(--accent-green)`, `border-radius: 50%`. No centro, ícone de check `✓` em `font-size: 72px`, `--accent-green`.
- Animação de entrada `pop-in`: o círculo escala de `0.3 → 1.05 → 1.0` em 0.5s com `cubic-bezier(0.34, 1.56, 0.64, 1)` (spring effect). Glow externo: `box-shadow: 0 0 60px rgba(48,209,88,0.25)`.

**Abaixo do ícone:**
- Texto "ACESSO LIBERADO" em `--font-title`, `--accent-green`, `font-weight: 600`. Surge com `opacity 0→1` em 0.3s, delay de 0.4s.
- Transportadora identificada (se disponível): "via CORREIOS" ou "via SHOPEE" em `--font-caption`, `--text-tertiary`. Surge com delay de 0.6s.

**Instrução de ação:**
- Seta larga apontando para a **direita** (direção da porta), `width: 120px`, `--text-primary`, animação `pulse-right` (translação de +10px no eixo X, 0.8s, ease-in-out, alternate).
- Texto: "Empurre a porta" em `--font-body`, `font-weight: 500`, `--text-primary`.
- Subtexto: "A fechadura está desbloqueada" em `--font-caption`, `--text-secondary`.

**Indicador de timeout:**
- Barra circular (`stroke-dasharray`) de 80px de diâmetro, branca com opacity 0.2, preenchida em verde, regredindo de 100% para 0% ao longo de `cfg.authorized_timeout_ms` (padrão 10s). Fica no canto superior direito do painel central.

---

### TELA 5 — INSIDE_WAIT
**Estado FSM:** `INSIDE_WAIT`
**Ativado por:** P1 fechou com pessoa detectada pelo mmWave dentro do corredor.
**Objetivo:** Entregador está dentro com a porta fechada e a balança ainda não detectou peso. Orientar onde colocar o pacote.

> **Esta tela estava faltando.** INSIDE_WAIT e DELIVERING são estados distintos. Aqui a pessoa ainda não depositou nada.

**Layout:**

**Área principal:**
- Ícone de caixa `📦` ou SVG de balança, `font-size: 64px`, animado com `bounce` suave (2px para cima e para baixo, 2s, ease-in-out).
- Texto principal: "Coloque o pacote na balança" em `--font-title`, `font-weight: 600`, `--text-primary`.
- Subtexto: "Qualquer posição da plataforma" em `--font-body`, `--text-secondary`.

**Indicador de balança:**
- Um card `glassmorphism` abaixo do texto principal mostrando a leitura em tempo real da balança:
  - Label: "PESO DETECTADO" em `--font-caption`, `--text-tertiary`.
  - Valor: "—" (traço) enquanto não há variação, substitui por "X.X kg" quando balança varia. Em `--font-hero`, `font-weight: 200`, `--text-primary`.
  - Barra de progresso horizontal preenchida à medida que o peso aumenta, de 0 a um max visual de 30kg, cor `--accent-green`.

**Rodapé:**
- Timer de segurança: barra de progresso linear (mesma do AWAKE), cor `--accent-orange`, regredindo ao longo de `cfg.inside_timeout_ms` (padrão 180s).
- Texto: "Tempo restante para depósito" em `--font-caption`, `--text-tertiary`.

---

### TELA 6 — DELIVERING
**Estado FSM:** `DELIVERING`
**Ativado por:** Balança detectou variação > `min_delivery_weight_g`.
**Objetivo:** Pacote está sobre a balança. Orientar o entregador a sair.

**Layout:**

**Confirmação de peso:**
- Card centralizado com `border: 1px solid rgba(48,209,88,0.3)`, `background: rgba(48,209,88,0.06)`:
  - Label: "PACOTE REGISTRADO" em `--font-caption`, `--accent-green`, `letter-spacing: 2px`.
  - Peso em tempo real: "1.24 kg" em `--font-hero`, `--accent-green`. Atualiza em tempo real a cada heartbeat.
  - Animação de entrada: o número "conta" de 0 até o valor real em 0.8s.

**Instrução de saída:**
- Seta larga apontando para a **esquerda** (direção de saída), `--accent-yellow`, mesma animação `pulse` mas na direção oposta.
- Texto: "Saia e feche a porta" em `--font-title`, `--text-primary`.
- Subtexto: "O recibo será gerado automaticamente" em `--font-body`, `--text-secondary`.

**Aviso importante:**
- Um pill `height: 32px`, `border-radius: 16px`, `background: rgba(255,159,10,0.15)`, `border: 1px solid rgba(255,159,10,0.3)`:
  - Ícone `⚠` + texto "Não feche antes de sair do corredor" em `--font-caption`, `--accent-orange`.

---

### TELA 7 — DOOR_REOPENED
**Estado FSM:** `DOOR_REOPENED`
**Ativado por:** P1 abriu novamente após DELIVERING — entregador saindo.
**Objetivo:** Aguardar P1 fechar e mmWave confirmar corredor vazio. O entregador pode ainda estar de costas para a porta.

> **Esta tela estava faltando.**

**Layout — mínima, transitória:**

- Ícone de seta saindo por uma porta, `font-size: 72px`, `--text-secondary`.
- Texto: "Aguardando saída..." em `--font-title`, `font-weight: 400`, `--text-secondary`.
- Subtexto: "Feche a porta completamente" em `--font-body`, `--text-tertiary`.
- **Indicador mmWave:** Pill no centro: `● Presença detectada` em `--accent-orange` enquanto mmWave = presente. Muda para `● Corredor livre` em `--accent-green` quando vazio. A mudança dispara a transição para CONFIRMING.

**Sem timer visível** — esta transição é rápida e determinística. Um spinner mínimo (20px, `--text-tertiary`) no canto inferior central indica que o sistema está ativo.

---

### TELA 8 — CONFIRMING
**Estado FSM:** `CONFIRMING`
**Ativado por:** P1 fechou + mmWave = vazio. Sistema estabilizando balança e gerando JWT.
**Objetivo:** Feedback durante os 2s de wait de estabilização + geração criptográfica.

**Layout:**

**Elemento central:**
- Spinner de carregamento `40px × 40px`, estilo iOS (`border: 3px solid rgba(255,255,255,0.15); border-top-color: --text-primary; border-radius: 50%; animation: spin 0.8s linear infinite`).
- Texto abaixo: "Gerando comprovante" em `--font-body`, `--text-primary`.
- Subtexto: "Registrando peso e assinatura criptográfica" em `--font-caption`, `--text-secondary`.

**Etapas visuais** (sequência animada, cada uma dura ~0.7s):
1. `○ Estabilizando balança...` → `● Peso confirmado` (verde)
2. `○ Capturando imagem...` → `● Foto registrada` (verde)
3. `○ Assinando token...` → `● Comprovante gerado` (verde)

As etapas surgem em sequência com stagger de 0.5s. Quando todas completam, transição para RECEIPT.

**Nota:** Se P1 abrir durante CONFIRMING (segundo entregador), a tela transiciona abruptamente para DELIVERING sem animação de conclusão.

---

### TELA 9 — RECEIPT
**Estado FSM:** `RECEIPT`
**Ativado por:** JWT gerado e enviado ao OPi.
**Objetivo:** Comprovante completo visível. Entregador fotografa o QR Code para sua própria segurança. 120 segundos de exibição.

**Layout — dois painéis laterais:**

**Painel esquerdo (45% da largura):**
- Foto capturada da webcam (snapshot do momento da entrega). `border-radius: 12px`, `border: 1px solid var(--border-glass)`.
- Abaixo da foto:
  - Transportadora: pill com cor por transportadora (Correios = amarelo, Shopee = laranja, Amazon = azul, ML = verde). Label: "via CORREIOS" em `--font-caption`.
  - Peso: "1.24 kg" em `--font-body`, `--text-secondary`.
  - Timestamp: "Hoje, 14:32" em `--font-caption`, `--text-tertiary`.

**Painel direito (45% da largura):**
- QR Code gerado dinamicamente, `width: 180px`, `border-radius: 12px`, fundo branco `padding: 12px`.
- Abaixo do QR:
  - "COMPROVANTE DIGITAL" em `--font-caption`, `--text-tertiary`, `letter-spacing: 2px`.
  - URL parcial: "portaria.rlight.com.br" em `--font-caption`, `--accent-blue`.
  - Token (primeiros 8 chars): `a3f9c2b1` em `--font-mono`, `--text-tertiary`.

**Mensagem central (entre os dois painéis):**
- Separador vertical `1px solid var(--border-glass)`, `height: 70%`.

**Topo da tela (acima dos painéis):**
- Título: "Entrega registrada" em `--font-title`, `--text-primary`. Sem exaltação, sem exclamação.
- Subtítulo: "Fotografe o código ao lado para o seu comprovante" em `--font-body`, `--text-secondary`.

**Rodapé:**
- Barra de progresso regredindo ao longo de 120s (`--accent-blue`).
- Texto: "Esta tela fecha automaticamente" em `--font-caption`, `--text-tertiary`.

**Animação de entrada:** Os dois painéis surgem de lados opostos (`translateX(-20px)` e `translateX(+20px)`) com `opacity 0→1` em 0.5s.

---

### TELA 10 — ABORTED
**Estado FSM:** `ABORTED`
**Ativado por:** Entregador entrou e saiu sem depositar nada na balança.
**Objetivo:** Informar de forma neutra que o ciclo foi encerrado sem entrega. 3 segundos e vai para IDLE.

> **Esta tela estava faltando.**

**Layout — minimalista, 3s de exibição:**

- Ícone `○` (círculo vazio) em `font-size: 48px`, `--text-tertiary`.
- Texto: "Nenhum pacote registrado" em `--font-title`, `font-weight: 400`, `--text-secondary`.
- Subtexto: "O ciclo foi encerrado sem depósito" em `--font-body`, `--text-tertiary`.

**Tom:** Neutro. Não é erro. Não é alerta. É informação.

**Animação de saída:** Após 3s, fade out rápido (0.3s) → IDLE.

---

### TELA 11 — RESIDENT_P1
**Estado FSM:** `RESIDENT_P1`
**Ativado por:** TF9S concedeu acesso de morador / HA enviou CMD_RESIDENT_UNLOCK.
**Objetivo:** P1 está aberta. Morador deve entrar e deixar P1 fechar. P2 vai abrir automaticamente.

**Layout:**

**Saudação personalizada:**
- "Bem-vindo" em `--font-title`, `font-weight: 300`, `--text-primary`.
- Nome do morador (vindo do label do código Wiegand): "Willian" em `--font-hero`, `font-weight: 700`, `--accent-green`.
- Ambos centralizados verticalmente no terço superior.

**Instrução:**
- Seta larga apontando para a **direita**, `--text-primary`, animação `pulse-right`.
- "Entre e deixe a porta fechar" em `--font-body`, `--text-primary`.

**Informativo sobre P2:**
- Card `glassmorphism` com ícone de cadeado aberto `🔓` e texto:
  - "A porta interna abrirá automaticamente" em `--font-body`, `--text-secondary`.
  - "Aguarde dentro do corredor" em `--font-caption`, `--text-tertiary`.

**Indicador de timeout:**
- Barra de progresso circular (como em AUTHORIZED), regredindo ao longo de `cfg.door_alert_ms` (90s). Cor `--accent-green`.

---

### TELA 12 — RESIDENT_P2
**Estado FSM:** `RESIDENT_P2`
**Ativado por:** P1 fechou com morador dentro. Delay de 2s antes de P2 abrir.
**Objetivo:** 2 segundos de espera para evitar queda de tensão por dois strikes simultâneos. Feedback visual para o morador não tentar forçar P2.

**Layout — foco total no feedback de progresso:**

**Elemento central:**
- Círculo de progresso (`SVG stroke-dasharray`) de `120px` de diâmetro, `stroke: --accent-blue`, `stroke-width: 3px`. Fundo do círculo: `rgba(10,132,255,0.1)`. O preenchimento vai de 0% para 100% ao longo de `cfg.p2_delay_ms` (2000ms).
- Dentro do círculo: ícone de cadeado `🔒` que muda para `🔓` quando o progresso completa.

**Texto abaixo:**
- Durante delay: "Aguarde..." em `--font-title`, `font-weight: 300`, `--text-secondary`.
- Quando completa (P2 abrindo): "Entrando" em `--font-title`, `--accent-blue` com fade-in de 0.2s.

**Subtexto:**
- "Verificando segurança da passagem" em `--font-caption`, `--text-tertiary`.

**Nota:** Tela dura exatamente `p2_delay_ms + door_open_ms` milissegundos. Não tem interação possível.

---

### TELA 13 — REVERSE_PICKUP
**Estado FSM:** `REVERSE_PICKUP`
**Ativado por:** TF9S concedeu acesso de coleta reversa.
**Objetivo:** Coletor está dentro do corredor. Deve retirar o pacote e sair.

**Layout:**

**Identificação da operação:**
- Pill no topo do conteúdo: `COLETA REVERSA` em `--font-caption`, `--accent-purple`, `border: 1px solid rgba(191,90,242,0.4)`, `background: rgba(191,90,242,0.1)`.
- Transportadora identificada: "Mercado Livre" em `--font-title`, `--text-primary`.

**Instrução:**
- Seta apontando para a **esquerda** (saída), `--text-primary`, animação `pulse-left`.
- "Retire o pacote e saia" em `--font-body`, `--text-primary`.

**Indicador de balança em tempo real:**
- Card com leitura: `Δ −1.24 kg` (variação negativa indica que algo foi retirado), em `--font-title`, `--accent-purple`.
- Label: "Variação detectada" em `--font-caption`, `--text-tertiary`.
- Quando variação ultrapassar o threshold: pill confirmatório "Coleta registrada ✓" em `--accent-green`.

**Timer:**
- Barra de progresso linear, `--accent-orange`, regredindo ao longo de `cfg.delivering_timeout_ms` (180s).

---

### TELA 14 — DOOR_ALERT
**Estado FSM:** `DOOR_ALERT`
**Ativado por:** P1 ficou aberta por mais de `cfg.door_alert_ms` (padrão 90s).
**Objetivo:** Alerta urgente de segurança. Porta aberta é risco físico. Requer ação imediata.

> **Estado distinto de ERROR.** DOOR_ALERT é situacional e se resolve sozinho (quando P1 fechar). ERROR é uma falha de hardware que requer intervenção técnica.

**Layout — máximo impacto visual:**

**Background:** Os blobs de luz transitam para `--accent-red` e `--accent-orange` ao longo de 1.2s. O fundo inteiro fica com um toque avermelhado sem ser completamente vermelho.

**Elemento pulsante:**
- Círculo de `200px` com `background: rgba(255,69,58,0.1)` e `border: 2px solid var(--accent-red)`, com animação `@keyframes ring-pulse` (escala 1.0→1.08→1.0 + opacity 1→0.6→1, 1.5s, infinite). Dentro: ícone `⚠` em `font-size: 80px`, `--accent-red`.

**Textos:**
- "PORTA ABERTA" em `--font-hero`, `font-weight: 700`, `--accent-red`, `letter-spacing: 2px`.
- "Feche a porta imediatamente" em `--font-title`, `--text-primary`.

**Indicador de tempo:**
- "Aberta há: 01:32" — contador crescente em `--font-title`, `--accent-orange`, `font-weight: 200`. Atualiza a cada segundo.

**Mensagem customizável** (vinda do HA via `dynamic_texts.MSG_DOOR_ALERT`):
- Card `glassmorphism` no rodapé com o texto configurável. Ex: "Se você não está no corredor, acesse o Home Assistant" em `--font-body`, `--text-secondary`.

**Resolução automática:** Quando P1 fechar, a tela faz um flash verde rápido (0.15s) e transiciona para IDLE com fade.

---

### TELA 15 — MAINTENANCE
**Estado FSM:** `MAINTENANCE`
**Ativado por:** HA toggle `maintenance_mode = true`.
**Objetivo:** Indicar que o sistema está em modo de manutenção. Sensores continuam funcionando, nenhum hardware é acionado.

> **Esta tela estava faltando.**

**Layout:**

**Background:** Blobs transitam para amarelo/âmbar (`--accent-yellow`), intensidade baixa.

**Elemento central:**
- Ícone de chave inglesa `🔧` em `font-size: 64px`.
- Texto: "Modo Manutenção" em `--font-title`, `--accent-yellow`.
- Subtexto: "Operações desabilitadas temporariamente" em `--font-body`, `--text-secondary`.

**Dados de saúde em tempo real** (o único estado onde o morador pode querer ver isso):
- Grid 2×3 com status de cada sensor:
  - `QR ●`, `Balança ●`, `Radar ●`, `Corrente P1 ●`, `Corrente P2 ●`, `Acesso ●`
  - Bolinha verde = OK, laranja = degradado, vermelha = offline.
  - Em `--font-caption`.
- Score total: "Score: 95/100" em `--font-body`, `--text-primary`.

**Rodapé:**
- "Para sair do modo manutenção, desative no Home Assistant" em `--font-caption`, `--text-tertiary`.

---

### TELA 16 — ERROR
**Estado FSM:** `ERROR`
**Ativado por:** Falha crítica bloqueante — strike sem corrente no INA219 após acionamento, interbloqueio violado, falha de hardware confirmada.
**Objetivo:** Bloqueio total. Alerta de que intervenção técnica é necessária. Não se resolve sozinho.

> **Distinto de DOOR_ALERT.** ERROR indica falha de hardware, não situação operacional.

**Layout:**

**Background:** Blobs em vermelho escuro (`#8B0000`), intensidade média.

**Elemento central:**
- Hexágono com borda `--accent-red` e ícone `×` centralizado (estilo iOS Critical Alert). Animação: shake de 0.5s ao entrar, depois estático.
- Texto: "Falha no sistema" em `--font-title`, `--accent-red`.
- Código do erro (vindo do payload de ERROR do ESP32): ex: `STRIKE_P1_FAIL` em `--font-mono`, `--text-tertiary`, `background: rgba(0,0,0,0.4)`, `border-radius: 4px`, `padding: 4px 8px`.

**Mensagem customizável** (HA → `dynamic_texts.MSG_ERRO`):
- "Chame no WhatsApp (81) 9999-9999" em `--font-body`, `--text-primary`.

**Instrução física:**
- Seta pulsante apontando para **baixo** (em direção à campainha física Tapo, que fica abaixo da tela):
  - Seta `⌄⌄` dupla, `font-size: 48px`, `--accent-yellow`, animação `bounce-down` (translação de 0 a +8px, 1s, ease-in-out, alternate, infinite).
  - "Aperte a campainha" em `--font-caption`, `--accent-yellow`.

**Nota:** Esta tela persiste até `CMD_UNLOCK_P1` ou reset via HA. Sem auto-resolução.

---

## Tabela de Correspondência Estado → Tela

| # | Estado FSM | Tela | Duração típica | Resolução |
|---|-----------|------|----------------|-----------|
| 1 | `IDLE` | Tela Standby | Indefinida | Automática |
| 2 | `AWAKE` | Tela Entregador | Até SCREEN_READY ou 60s | Timer |
| 3 | `WAITING_QR` | Tela Leitura QR | Até leitura ou 30s | Timer + fallback |
| 4 | `AUTHORIZED` | Tela Autorizado | Até P1 abrir ou 10s | Timer |
| 5 | `INSIDE_WAIT` | Tela Depósito | Até balança ou 180s | Balança |
| 6 | `DELIVERING` | Tela Saída | Até P1 abrir ou 180s | P1 micro switch |
| 7 | `DOOR_REOPENED` | Tela Aguardando Saída | Até P1 fechar + mmWave | mmWave |
| 8 | `CONFIRMING` | Tela Processando | ~2-4s | JWT gerado |
| 9 | `RECEIPT` | Tela Comprovante | 120s | Timer |
| 10 | `ABORTED` | Tela Sem Depósito | 3s | Timer fixo |
| 11 | `RESIDENT_P1` | Tela Morador P1 | Até P1 fechar ou 90s | P1 micro switch |
| 12 | `RESIDENT_P2` | Tela Morador P2 | p2_delay_ms + door_ms | Timer fixo |
| 13 | `REVERSE_PICKUP` | Tela Coleta Reversa | Até P1 abrir ou 180s | P1 micro switch |
| 14 | `DOOR_ALERT` | Tela Porta Aberta | Até P1 fechar | P1 micro switch |
| 15 | `MAINTENANCE` | Tela Manutenção | Indefinida | HA toggle |
| 16 | `ERROR` | Tela Falha Crítica | Indefinida | HA reset |

---

## Componentes Reutilizáveis

### `<StatusHeader>` (global)
- Sempre presente e no topo
- Preenche o template com: label de estado, relógio HH:MM:SS, data, bolinha de status

### `<ProgressBar type="linear|circular">` 
- Linear: largura 100% → 0%, cor configurável, height 2px no bottom da área de conteúdo
- Circular: SVG stroke-dasharray, diâmetro configurável

### `<ActionArrow direction="left|right|down|diagonal-down-left">`
- SVG de seta estilizada, animação de pulsação direcional
- Cor e tamanho configuráveis via props

### `<WeightDisplay live={true|false}>`
- Exibe peso em tempo real ou valor estático
- Animação de count-up na entrada
- Unidade automática (g abaixo de 1000, kg acima)

### `<GlassCard>`
- `backdrop-filter: blur(16px)`
- `background: var(--bg-glass)`
- `border: 1px solid var(--border-glass)`
- `border-radius: 16px`

### `<Pill color="green|yellow|blue|red|orange|purple">`
- Inline chip com cor de fundo translúcida e borda colorida
- `height: 28px`, `border-radius: 14px`, `padding: 0 12px`
- Texto em `--font-caption`

### `<FallbackPopup>`
- Específico para WAITING_QR após 15s
- Desliza do bottom, glassmorphism
- Contém número de telefone e botão de interfone

---

## Animações CSS nomeadas

```css
@keyframes float {
  0%, 100% { transform: translate(0, 0) scale(1); }
  33%       { transform: translate(30px, -20px) scale(1.05); }
  66%       { transform: translate(-20px, 10px) scale(0.97); }
}

@keyframes pulse-dot {
  0%, 100% { opacity: 0.6; transform: scale(1); }
  50%       { opacity: 1;   transform: scale(1.3); }
}

@keyframes pop-in {
  0%   { transform: scale(0.3);  opacity: 0; }
  70%  { transform: scale(1.05); opacity: 1; }
  100% { transform: scale(1);    opacity: 1; }
}

@keyframes pulse-right {
  0%, 100% { transform: translateX(0); }
  50%       { transform: translateX(10px); }
}

@keyframes pulse-left {
  0%, 100% { transform: translateX(0); }
  50%       { transform: translateX(-10px); }
}

@keyframes bounce-down {
  0%, 100% { transform: translateY(0); }
  50%       { transform: translateY(8px); }
}

@keyframes ring-pulse {
  0%, 100% { transform: scale(1);    opacity: 1; }
  50%       { transform: scale(1.08); opacity: 0.6; }
}

@keyframes spin {
  to { transform: rotate(360deg); }
}

@keyframes shimmer {
  0%   { background-position: -200% center; }
  100% { background-position:  200% center; }
}

@keyframes shake {
  0%, 100% { transform: translateX(0); }
  20%       { transform: translateX(-8px); }
  40%       { transform: translateX(8px); }
  60%       { transform: translateX(-5px); }
  80%       { transform: translateX(5px); }
}
```

---

## Notas de Implementação Web

### Tecnologia sugerida
- **React** com hooks para gerenciamento de estado local de UI
- **Tailwind CSS** ou **CSS Modules** para estilização
- **Framer Motion** para transições entre telas e animações de entrada/saída
- WebSocket ou SSE para receber eventos do `host/local_api.py` do Orange Pi em tempo real
- O estado atual da FSM é consumido via `GET /status` (polling de 500ms) ou via WebSocket se implementado

### Transições entre telas
- Todas as transições usam `opacity: 0 → 1` com duração de 0.3s como base
- Telas de sucesso (AUTHORIZED, RECEIPT) têm entrada mais elaborada (ver especificações individuais)
- Telas de erro (DOOR_ALERT, ERROR) têm entrada imediata sem animação suave
- O componente `<StatusHeader>` não transiciona — persiste entre telas

### Responsividade
- A UI é fixada em 1024×600px via `width: 1024px; height: 600px; overflow: hidden`
- Kiosk mode via `<meta name="viewport" content="width=1024, initial-scale=1, user-scalable=no">`
- Nenhum elemento tem scroll — tudo cabe na tela ou usa truncamento elegante

### Dados dinâmicos que a UI precisa receber
```
Endereço do imóvel       → dynamic_texts.ENDERECO_TEXT
Nomes dos moradores      → dynamic_texts.NOMES_RECEBEDORES
Telefone de contato      → dynamic_texts.TELEFONE_CONTATO (novo campo)
Mensagem de erro custom  → dynamic_texts.MSG_ERRO
Estado FSM atual         → host_fsm.get_state()
Peso em tempo real       → host_fsm.get_weight()
JWT token                → host_fsm.get_jwt()
Foto da entrega          → /photos/{token}.jpg via API local
Score de saúde           → /status endpoint
Online status            → presença de heartbeat < 90s
Transportadora atual     → host_fsm.get_carrier() (novo)
Label do morador         → host_fsm.get_resident_label() (novo)
```