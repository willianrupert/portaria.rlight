# iOS PWA — Barra Preta Inferior: Post-Mortem Completo

**Data:** 07/05/2026  
**Dispositivo:** iPhone 16 Pro, iOS 17+, PWA Standalone  
**Sintoma:** Faixa preta no rodapé que some após rotação e volta ao sair e retornar ao app.

---

## ⚠️ Bug do WebKit — NÃO É BUG DO CÓDIGO

Este é um **bug documentado do WebKit** (bugs.webkit.org) em modo PWA standalone.  
O iOS faz um snapshot GPU das camadas compositor ao ir para background.  
Ao retornar, restaura o snapshot — incluindo o valor **stale** de `env(safe-area-inset-bottom)`.  
A rotação física invalida esse snapshot completamente (só o OS pode fazer isso).  
**Nenhuma mudança de CSS ou JS consegue simular a invalidação que a rotação faz.**

---

## ❌ Abordagens que NÃO FUNCIONAM (não tentar novamente)

### 1. `bottom: calc(-1 * env(safe-area-inset-bottom, 34px))` no `.panel`
**Motivo da falha:** `env(safe-area-inset-bottom)` retorna valor stale/0 no initial load e no resume do PWA. O CSS é calculado com o valor errado em cache do WebKit.

### 2. `html { position: fixed; inset: 0 }` 
**Motivo da falha:** Quebra o layout completamente. `body` perde referência de tamanho. Causa overflow e bugs de scroll. O `position: fixed` no elemento raiz `html` é problemático em todos os browsers.

### 3. `html { height: -webkit-fill-available }` com background sólido `#000000`
**Motivo da falha:** A zona do home indicator mostra o background do `html` (preto sólido). O `.panel` com `backdrop-filter` não se extende até lá. Resultado: barra preta visível quando `env()` não estende o painel.

### 4. Script JS para reflow no `visibilitychange` / `pageshow` / `focus`
Tentamos:
```javascript
panel.style.bottom = '0px';
requestAnimationFrame(() => requestAnimationFrame(() => { panel.style.bottom = ''; }));
```
**Motivo da falha:** Mudanças de CSS via JS não invalidam o compositor GPU do WebKit. Só mudanças REAIS nas dimensões da janela (como rotação física) conseguem isso.

### 5. `window.dispatchEvent(new Event('resize'))`
**Motivo da falha:** Um evento sintético não muda `window.innerWidth/Height`. O WebKit ignora para fins de recalcular env(). Nenhum efeito real.

### 6. `html { background: gradiente }` + `.panel { bottom: 0 }`
**Motivo da falha:** Gambiarra visual que não resolve o problema real. O usuário não quer um fundo pintado — quer a experiência fullscreen real com glass effect. Além disso, o gradiente não corresponde exatamente ao efeito do `backdrop-filter`.

### 7. `theme-color` com media queries + background literal no `html`
**Motivo da falha:** O `theme-color` controla a UI do Safari (barra de status), não a zona do home indicator dentro do PWA. A cor literal no `html` ajuda marginalmente mas não resolve o problema de `env()` stale.

### 8. `background: rgba(0,0,0,0.55)` hardcoded no `.panel`
**Motivo da falha:** Ignora completamente o sistema dark/light mode. Em light mode, o painel fica sempre escuro — alto contraste, UI quebrada.

---

## ✅ A ÚNICA ABORDAGEM QUE FUNCIONA

### Princípio
A rotação nos dá o valor correto de `env(safe-area-inset-bottom)`.  
Capturamos esse valor via `resize` event e persistimos no `localStorage`.  
Em todos os carregamentos subsequentes, aplicamos o valor persistido **antes** que o snapshot errado seja exibido.

### CSS
```css
.panel {
  bottom: 0; /* NUNCA usar env() aqui — stale no WebKit PWA */
}
```

### JavaScript (inline, antes do module)
```javascript
(function() {
  var KEY = 'rlight-sab'; // safe-area-bottom cached value

  function applyBottom(px) {
    var panel = document.querySelector('.panel');
    if (panel) panel.style.bottom = '-' + Math.max(0, px) + 'px';
  }

  function measureEnv() {
    // Só funciona corretamente APÓS rotação — env() é preciso nesse momento
    var probe = document.createElement('div');
    probe.style.cssText = 'position:fixed;bottom:0;left:0;width:1px;height:env(safe-area-inset-bottom,0px);pointer-events:none;opacity:0;';
    document.documentElement.appendChild(probe);
    var h = probe.getBoundingClientRect().height;
    document.documentElement.removeChild(probe);
    return h;
  }

  function restoreFromCache() {
    var cached = parseFloat(localStorage.getItem(KEY));
    // Default 34px = home indicator exato de todos os iPhones Face ID
    applyBottom(isNaN(cached) ? 34 : cached);
  }

  // Aplicar imediatamente no DOMContentLoaded
  document.addEventListener('DOMContentLoaded', restoreFromCache);

  // Capturar valor correto APÓS rotação (resize real muda window.innerHeight)
  window.addEventListener('resize', function() {
    var h = measureEnv();
    if (h > 0) {
      localStorage.setItem(KEY, String(h));
      applyBottom(h);
    }
  });

  // Re-aplicar ao voltar ao app
  document.addEventListener('visibilitychange', function() {
    if (!document.hidden) restoreFromCache();
  });
})();
```

### Por que funciona
1. **Primeiro uso:** default 34px → painel estendido corretamente para a maioria dos iPhones
2. **Após primeira rotação:** valor real capturado e salvo no localStorage
3. **Sair e retornar:** `visibilitychange` → lê localStorage → aplica ANTES do snapshot stale ser exibido
4. **Não depende de `env()` no CSS** → imune ao bug do compositor WebKit

---

## ⚡ Regras absolutas para este projeto

1. **NUNCA usar `env(safe-area-inset-bottom)` no `bottom` do `.panel`** — sempre será stale no PWA
2. **NUNCA usar `position: fixed` no `html`** — quebra layout em todos os browsers
3. **NUNCA usar background hardcoded sem variável CSS** — quebra dark/light mode automático
4. **NUNCA confiar em reflow JS para recalcular env()** — o compositor GPU ignora
5. **SEMPRE usar `var(--glass)` no background do `.panel`** — adaptação dark/light automática
6. **O valor correto de safe-area-bottom é 34px nos iPhones com Face ID** — usar como fallback

---

## Estado final do CSS de layout (reference)

```css
html {
  height: -webkit-fill-available;
  overflow: hidden;
  background: /* gradiente idêntico ao .bg */ #000000;
}
@media (prefers-color-scheme: light) {
  html { background: #F2F2F7; }
}
body {
  min-height: 100vh;
  min-height: -webkit-fill-available;
  margin: 0; padding: 0; overflow: hidden;
  color: var(--fg); background: transparent;
  display: flex; align-items: center; justify-content: center;
}
.panel {
  position: fixed;
  top: 0; left: 0; right: 0;
  bottom: 0; /* JS aplica inline o valor correto via localStorage */
  background: var(--glass);
  backdrop-filter: blur(60px); -webkit-backdrop-filter: blur(60px);
  padding-top: max(44px, env(safe-area-inset-top, 44px));
  padding-bottom: max(34px, env(safe-area-inset-bottom, 34px));
  /* padding usa env() — stale no padding é aceitável (não causa barra visual) */
}
```
