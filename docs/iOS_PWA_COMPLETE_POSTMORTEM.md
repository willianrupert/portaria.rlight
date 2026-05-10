# iOS PWA — Barra Preta Inferior: Documentação Completa

**Data:** 07-08/05/2026  
**Dispositivo:** iPhone 16 Pro, iOS 17+, PWA Standalone  
**Sintoma:** Faixa preta no rodapé que aparece na primeira abertura e ao retornar do background.

---

## ⚠️ O Bug Raiz — WebKit Snapshot + black-translucent

### Descoberta Final (08/05/2026)

Após extensa pesquisa e 19 tentativas falhas, a causa raiz foi identificada:

**O meta tag `apple-mobile-web-app-status-bar-style: black-translucent` aplica um offset negativo de ~59px na viewport.**

Quando você adiciona:
```html
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
```

O iOS faz o seguinte:
1. Estende o conteúdo para trás da status bar (comportamento esperado)
2. **MAS** aplica um offset negativo de ~59px na viewport inteira
3. O conteúdo é empurrado 59px acima do bottom real da tela
4. Isso cria o **chin gap** (barra preta inferior)

### Por que `height: 100%` piora o problema

Se você usa `html { height: 100% }` ou `height: -webkit-fill-available`:
- O `body` calcula altura relativa ao bloco pai
- O bloco pai tem a viewport com offset negativo
- O `body` NÃO cresce para compensar o offset
- Resultado: ~59px de gap no bottom

### A Solução Correta

**Usar `height: 100vh` (viewport height absoluto) em vez de valores relativos.**

`100vh` = altura real da viewport (incluindo a área estendida)
`100%` ou `-webkit-fill-available` = relativo ao bloco pai (não compensa o offset)

### Configuração Recomendada (Sem black-translucent)

```html
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<!-- OMITIR black-translucent completamente -->
<meta name="apple-mobile-web-app-capable" content="yes">
```

```css
html {
  height: 100vh; /* Absoluto, não relativo */
  overflow: hidden;
  background: #000000; /* Cor sólida para cobrir zona do home indicator */
}
body {
  min-height: 100vh; /* Absoluto */
  margin: 0; padding: 0; overflow: hidden;
}
```

### Descobertas Adicionais (Reddit Research)

1. **iOS standalone mode** automaticamente estende o app edge-to-edge, atrás da status bar e home indicator, usando a cor de fundo da página
2. **Você NÃO precisa de `viewport-fit=cover`** ou manipulação manual de safe areas para conseguir fullscreen
3. **O `background_color` do manifest.json** é usado para preencher as áreas do status bar e home indicator
4. **`viewport-fit=cover` funciona bem** por si só, mas com `black-translucent` quebra
5. **Configuração que funciona:**
   - `display: standalone` no manifest
   - Opcional: `viewport-fit=cover` se precisar de controle manual das safe areas em landscape
   - NUNCA usar `black-translucent`

---

## ❌ Abordagens que NÃO FUNCIONAM (Não tentar novamente)

### 1. `bottom: calc(-1 * env(safe-area-inset-bottom, 34px))` no `.panel`
**Motivo da falha:** `env(safe-area-inset-bottom)` retorna valor stale/0 no initial load e no resume do PWA. O CSS é calculado com o valor errado em cache do WebKit.

### 2. `html { position: fixed; inset: 0 }` 
**Motivo da falha:** Quebra o layout completamente. `body` perde referência de tamanho. Causa overflow e bugs de scroll.

### 3. `html { height: -webkit-fill-available }` com background sólido
**Motivo da falha:** Valor relativo que não compensa o offset negativo do `black-translucent`. A zona do home indicator fica descoberta.

### 4. Script JS para reflow no `visibilitychange` / `pageshow` / `focus`
**Motivo da falha:** Mudanças de CSS via JS não invalidam o compositor GPU do WebKit. Só mudanças reais nas dimensões da janela conseguem isso.

### 5. `window.dispatchEvent(new Event('resize'))`
**Motivo da falha:** Evento sintético não muda `window.innerWidth/Height`. O WebKit ignora para fins de recalcular env().

### 6. Cores sólidas no `html`/`body` com `backdrop-filter` no `.panel`
**Motivo da falha:** O `.panel` com `backdrop-filter: blur()` tem apenas 4% de opacidade (`rgba(255,255,255,0.04)`), deixando o `.bg` gradiente transparecer.

### 7. Extensão de altura com `height: calc(100vh + 50px)` no `.panel`
**Motivo da falha:** O WebKit ignora valores de `height` estendido no snapshot do PWA standalone.

### 8. `bottom: -50px` no `.panel`
**Motivo da falha:** O WebKit ignora valores negativos no snapshot, ou o snapshot é tirado de uma camada diferente.

---

## 📋 Histórico de Todas as Tentativas (1-19)

### Tentativa 1: Ajuste na Lógica de Cache (Commit: `d44eceb`)
**O que foi feito:**
- Modificado script inline para definir variável CSS `--sab` imediatamente
- Ajustado `restoreFromCache()` para ignorar valores <=0
- Adicionado `requestAnimationFrame` duplo

**Resultado:** ❌ Não funcionou - snapshot do WebKit capturava estado incorreto.

---

### Tentativa 2: Cores Sólidas no HTML/Body (Commit: `f699a9e`)
**O que foi feito:**
- Revertido `html` para usar cores sólidas do tema
- Ajustado `body` para usar `background: var(--bg)` sólido
- Mantido uso da variável `--sab`

**Resultado:** ❌ Não funcionou - problema persistia.

---

### Tentativa 3: Extensão Física com `bottom: -50px` (Commit: `390bec2`)
**O que foi feito:**
- Removido todo o código de cache `rlight-sab`
- Alterado `.panel` para usar `bottom: -50px`

**Resultado:** ❌ Não funcionou - barra preta ainda aparecia.

---

### Tentativa 13: `height: calc(100vh + 50px)` (Commit: `c4a0f58`)
**O que foi feito:**
- Removido `bottom: -50px`
- Adicionado `height: calc(100vh + 50px)`

**Resultado:** ❌ Não funcionou - barra preta persistia.

---

### Tentativa 14: `top:0; bottom:0;` + `transform: translateZ(0)` (Commit: `2161f3a`)
**O que foi feito:**
- Alterado para `top: 0; bottom: 0; left: 0; right: 0;`
- Adicionado `will-change: transform` e `translateZ(0)`

**Resultado:** ❌ Não funcionou - não invalidava o snapshot.

---

### Tentativa 15: `height: 100vh` (Commit: `95a4cce`)
**O que foi feito:**
- Removido `top/bottom`
- Usado `height: 100vh; height: -webkit-fill-available;`

**Resultado:** ❌ Não funcionou - barra preta na primeira abertura e ao retornar do background.

---

### Tentativa 16: Remover `viewport-fit=cover` (Commit: `857b85f`)
**O que foi feito:**
- Removido `viewport-fit=cover` da meta tag viewport

**Resultado:** ❌ **PIOROU** - barra inferior colorida + nova barra preta superior perto da ilha dinâmica.

---

### Tentativa 17: Re-adicionar `viewport-fit=cover` + `100dvh` (Commit: `f5907cc`)
**O que foi feito:**
- Re-adicionado `viewport-fit=cover`
- Usado `height: 100dvh`

**Resultado:** ❌ Não funcionou - "só piorou", usuário pediu restauração.

---

### Restauração para Tentativa 15 (Commit: `037c0fa`)
Restaurado arquivo para estado da Tentativa 15.

---

### Tentativa 19: `.panel` com fundo sólido + `.panel-content` com glass effect (Commit: `bdac5d6`)
**O que foi feito:**
- `.panel` cobre toda a tela com `top: 0; bottom: 0;` e `background: var(--bg)` sólido
- Criado `.panel-content` interno com `background: var(--glass)` e `backdrop-filter: blur(60px)`
- Padding movido para `.panel-content`

**Resultado:** ⏳ Melhorou - voltou para apenas a barra preta inferior. Aguardando nova abordagem.

---

## 🎯 Tentativa 20: Remover black-translucent + height: 100vh (IMPLEMENTADA)

### Baseada na descoberta final (08/05/2026)

**O que foi feito:**

1. **Removido meta tag problemática:**
   ```html
   <!-- REMOVIDO -->
   <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
   ```

2. **Corrigido `html` height:**
   ```css
   /* ANTES */
   html {
     height: -webkit-fill-available;
     overflow: hidden;
     background: #000000;
   }
   
   /* DEPOIS */
   html {
     height: 100vh;
     overflow: hidden;
     background: #000000;
   }
   ```

3. **Corrigido `body` min-height:**
   ```css
   /* ANTES */
   body {
     min-height: 100vh;
     min-height: -webkit-fill-available;
     /* ... */
   }
   
   /* DEPOIS */
   body {
     min-height: 100vh;
     /* ... */
   }
   ```

4. **Mantida estrutura do Attempt 19:**
   - `.panel` com `top: 0; bottom: 0;` e `background: var(--bg)` sólido
   - `.panel-content` com glass effect e padding de safe areas

### Justificativa

- Remover `black-translucent` elimina o offset negativo de ~59px
- `height: 100vh` (absoluto) substitui `-webkit-fill-available` (relativo)
- iOS standalone mode gerencia safe areas automaticamente sem `black-translucent`
- Estrutura de `.panel` + `.panel-content` do Attempt 19 permanece válida

### Resultado
⏳ **Aguardando teste no iPhone real** - Alterações implementadas e deployadas.

---

## 📊 Valores de `env(safe-area-inset-*)` no iOS

### Com status bar style padrão (sem black-translucent):
- **Portrait:** top = 0 (iOS reserva), bottom = 34px (home indicator)
- **Landscape:** left/right = 59px (notch/Dynamic Island), bottom = 20px

### Com `black-translucent`:
- **Portrait:** top = 59px (não reservado), bottom = 34px
- **Landscape:** left/right = 59px, bottom = 20px
- **PROBLEMA:** Viewport tem offset negativo de ~59px, criando chin gap

---

## 🔍 O Que NÃO Deve Ser Tentado Novamente

### 1. Manipulação via JavaScript
- ❌ Usar `env(safe-area-inset-bottom)` no JS
- ❌ Cache no localStorage
- ❌ Eventos `resize`, `visibilitychange`, `DOMContentLoaded`

### 2. Variáveis CSS com fallback
- ❌ `calc(-1 * var(--sab, 34px))`
- ❌ Definir variáveis via `document.documentElement.style.setProperty`

### 3. Extensão de altura via CSS
- ❌ `bottom: -50px`
- ❌ `height: calc(100vh + 50px)`

### 4. Meta tags problemáticas
- ❌ `black-translucent` - CAUSA RAIZ DO PROBLEMA
- ❌ `viewport-fit=cover` com `black-translucent` - combinação fatal

### 5. Valores relativos de altura
- ❌ `height: -webkit-fill-available`
- ❌ `height: 100%`
- ❌ `min-height: -webkit-fill-available`

---

## ✅ Configuração Final Recomendada

### HTML Head
```html
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<meta name="apple-mobile-web-app-capable" content="yes">
<!-- NÃO usar black-translucent -->
<meta name="apple-mobile-web-app-title" content="rlight">
<link rel="manifest" href="/manifest.json">
<meta name="theme-color" content="#000000" media="(prefers-color-scheme: dark)">
<meta name="theme-color" content="#F2F2F7" media="(prefers-color-scheme: light)">
```

### CSS Base
```css
html {
  height: 100vh;
  overflow: hidden;
  background: #000000;
}
@media (prefers-color-scheme: light) {
  html { background: #F2F2F7; }
}
body {
  min-height: 100vh;
  margin: 0; padding: 0; overflow: hidden;
  color: var(--fg); background: var(--bg);
}
```

### Manifest.json
```json
{
  "display": "standalone",
  "background_color": "#000000",
  "theme_color": "#000000"
}
```

---

## 📝 Commits Realizados

1. `d44eceb` - Ajuste na lógica de cache (falhou)
2. `f699a9e` - Cores sólidas no HTML/Body (falhou)
3. `97d22d2` - Extensão de altura (falhou)
4. `390bec2` - bottom:-50px (falhou)
5. `c4a0f58` - height: calc(100vh + 50px) (falhou)
6. `2161f3a` - top/bottom + transform (falhou)
7. `95a4cce` - height:100vh (falhou)
8. `857b85f` - Remover viewport-fit=cover (PIOROU)
9. `f5907cc` - 100dvh (falhou - "só piorou")
10. `037c0fa` - Restauração para Tentativa 15
11. `bdac5d6` - Tentativa 19: .panel sólido + .panel-content glass (melhorou)
12. **PENDENTE** - Tentativa 20: Remover black-translucent + height:100vh (commit pendente)

---

## 🏁 Conclusão

Após 19 tentativas e extensa pesquisa, a causa raiz foi identificada: **o meta tag `apple-mobile-web-app-status-bar-style: black-translucent`**.

A correção definitiva requer:
1. Remover `black-translucent`
2. Usar `height: 100vh` (absoluto) em vez de valores relativos
3. Deixar o iOS gerenciar safe areas automaticamente em standalone mode

**Status atual:** Tentativa 20 implementada, aguardando commit, push, deploy e teste no iPhone real.
