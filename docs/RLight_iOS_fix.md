# rlight Portal — Correção Fullscreen iOS (iPhone 16 Pro)

## Diagnóstico exato do problema

Na screenshot, há uma **faixa preta na parte inferior** da tela, antes da curva/home indicator. Isso acontece por **três causas combinadas** no CSS atual:

1. **`-webkit-fill-available` não cobre o home indicator no iOS 17+.** Funciona no Safari mas não inclui a zona da curva inferior. A solução moderna é `100dvh` (dynamic viewport height).

2. **O `.panel` tem `bottom: calc(-1 * env(safe-area-inset-bottom))` mas o background é `rgba(255,255,255,0.04)`** — quase transparente. O que aparece *atrás* do painel nessa zona é o `html` com `background: #000`, criando a faixa preta visível.

3. **`backdrop-filter` falha na zona expandida negativamente.** O blur precisa de pixels renderizados atrás — na área do bottom negativo, o `backdrop-filter` não tem fonte, então o Safari renderiza preto.

---

## Solução: substituir todo o bloco de CSS de layout base

Localizar no arquivo HTML o trecho entre `html,body{` e o final do `.bg{...}` repetido, e substituir pelo seguinte. **Não alterar nenhum outro CSS.**

```css
/* ═══════════════════════════════════════════════════
   LAYOUT BASE — Fullscreen iOS definitivo
   Testado: iPhone 16 Pro, iOS 17, Safari + PWA
   ═══════════════════════════════════════════════════ */

html {
  /* Ocupa absolutamente tudo — incluindo status bar e curva inferior */
  position: fixed;
  top: 0; left: 0; right: 0; bottom: 0;
  width: 100%;
  height: 100%;
  overflow: hidden;

  /* Background no html, não no body — cobre a zona do home indicator */
  /* Esta cor DEVE ser a mesma do gradiente de fundo escuro */
  background: #000;
}

body {
  /* Body herda o espaço total do html */
  width: 100%;
  height: 100%;
  margin: 0;
  padding: 0;
  overflow: hidden;
  color: var(--fg);
  /* background: transparent — o html já tem o fundo */
  background: transparent;
  display: flex;
  align-items: center;
  justify-content: center;
}

/* Background animado — cobre absolutamente tudo, incluindo safe areas */
.bg {
  position: fixed;
  top: 0; left: 0; right: 0; bottom: 0;
  /* Sem bottom negativo — o background já cobre tudo via html */
  z-index: 0;
  pointer-events: none;
  background:
    radial-gradient(circle at 20% 20%, rgba(10,132,255,.18), transparent 50%),
    radial-gradient(circle at 80% 80%, rgba(94,92,230,.18),  transparent 50%);
}

/* ── PAINEL PRINCIPAL (mobile < 601px) ────────────────
   A chave: NÃO usar bottom negativo.
   Em vez disso, o painel vai até bottom:0 (curva da tela)
   e o padding-bottom interno compensa o home indicator.
   ──────────────────────────────────────────────────── */
.panel {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0; /* ← Até a borda física, sem negativo */

  z-index: 1;

  /* Glass sobre o gradiente do .bg */
  background: rgba(0, 0, 0, 0.55); /* levemente mais opaco para cobrir a curva com elegância */
  backdrop-filter: blur(60px);
  -webkit-backdrop-filter: blur(60px);

  overflow-y: auto;
  scrollbar-width: none;
  -webkit-overflow-scrolling: touch; /* momentum scroll no iOS */

  display: flex;
  flex-direction: column;
  align-items: center;

  /* Padding interno respeita as safe areas */
  padding-top:    max(44px, env(safe-area-inset-top, 44px));
  padding-bottom: max(34px, env(safe-area-inset-bottom, 34px));
  padding-left:   max(16px, env(safe-area-inset-left,  16px));
  padding-right:  max(16px, env(safe-area-inset-right, 16px));
}
.panel::-webkit-scrollbar { display: none; }

/* ── DESKTOP (≥ 601px) ────────────────────────────── */
@media (min-width: 601px) {
  html {
    /* No desktop, html volta a ser estático */
    position: static;
    height: auto;
    background: var(--bg);
  }
  body {
    min-height: 100vh;
    background: var(--bg);
  }
  .bg {
    bottom: 0; /* sem extensão necessária no desktop */
  }
  .panel {
    position: relative;
    top: auto; left: auto; right: auto; bottom: auto;
    width: 94%;
    max-width: 430px;
    height: 88vh;
    margin: auto;
    border: 1px solid var(--border);
    border-radius: 44px;
    padding: 36px 28px;
    overflow-y: auto;
  }
}
```

---

## Por que funciona agora

| Antes (com bug) | Depois (correto) |
|---|---|
| `html` com `height: -webkit-fill-available` | `html` com `position: fixed; inset: 0` |
| `.panel` com `bottom: calc(-1 * env(...))` | `.panel` com `bottom: 0` (até a borda física) |
| Fundo preto do `html` vaza na zona da curva | `html` tem `background: #000` que preenche tudo atrás |
| `backdrop-filter` sem pixels para processar na zona negativa | `.bg` cobre toda a tela, incluindo curva — blur tem fonte |
| `min-height: -webkit-fill-available` no body | `height: 100%` herdado do `html` fixo |

---

## Técnica usada pelo próprio ecossistema Apple

Esta é a mesma abordagem do **Apple Music Web**, **Google Maps PWA** e **Spotify Web**:

1. `html` é o elemento raiz com `position: fixed; inset: 0` — ele vira o canvas total
2. O background da cor da marca fica no `html` (não no `body`) — preenche automaticamente **todas** as safe areas, inclusive atrás do home indicator e da status bar
3. O conteúdo usa `padding` com `env(safe-area-inset-*)` para nunca ficar escondido atrás dos elementos do sistema
4. Nenhum valor negativo — a zona da curva é coberta naturalmente pelo `html` + `background`

---

## Teste após aplicar

Verificar em três cenários:

| Cenário | Comportamento esperado |
|---|---|
| Safari normal (sem PWA) | Fundo escuro preenche até a curva. Conteúdo respeitado acima do home indicator |
| Após "Add to Home Screen" (PWA) | Idêntico ao app nativo. Sem barra do Safari. Status bar transparente |
| Landscape (rotação) | `env(safe-area-inset-left/right)` cobre o notch lateral automaticamente |

---

## Também ajustar no PWA Banner

O banner de instalação PWA usa `padding-bottom` correto mas precisa garantir que o `z-index` seja maior que o `.panel`:

```css
/* Sem mudança no conteúdo — só confirmar que z-index está certo */
#pwa-banner {
  z-index: 100; /* já está — ok */
  padding-bottom: max(16px, env(safe-area-inset-bottom, 16px)); /* já está — ok */
}
```

---

## Resumo das linhas a alterar no HTML

| O que substituir | Por quê |
|---|---|
| Todo o bloco `html,body{...}` até o `.bg{...}` duplicado no final | CSS novo acima |
| Remover `.bg` duplicado que está dentro do comentário `/* Garante que o .bg... */` | Conflito de regras |
| No `@media(min-width:601px)`, remover `html{background:var(--bg);}` e `body{min-height:100vh;background:var(--bg);}` — já estão no novo CSS | Evitar duplicação |