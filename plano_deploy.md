# RLIGHT v8 — Plano de Deploy-Ready
**Data:** Maio de 2026  
**Gerado por:** Análise completa do repositório (73 arquivos, 530KB)  
**Destino:** Lido por IA agente para execução sequencial

---

## STATUS GERAL DO PROJETO

Após três revisões consecutivas, o projeto está **~90% concluído**. A base de firmware (ESP32-S3), a camada Python (Host OPi) e a interface kiosk (Web SPA) estão funcionais. O que resta são:

1. **1 bug crítico no Android** (causa o loop "Verificando sessão")
2. **2 bugs médios no firmware** restantes de revisões anteriores
3. **1 problema de segurança** no cloud_backend
4. **Vários itens de polimento/produção** que o distinguem de "funciona no dev" para "está em produção"

---

## PARTE 1 — BUG CRÍTICO: ANDROID LOOP "VERIFICANDO SESSÃO"

### Diagnóstico

O `cloud_backend/public/app.js` usa **Firebase Authentication com `signInWithPopup` como primário e `signInWithRedirect` como fallback**. No iOS Safari isso funciona porque a sequência foi cuidadosamente tratada no postmortem documentado em `docs/iOS_PWA_COMPLETE_POSTMORTEM.md`.

**No Android Chrome/WebView, o problema é diferente:**

```javascript
// cloud_backend/public/app.js — Boot atual
(async () => {
  await setPersistence(auth, browserLocalPersistence);
  try { await getRedirectResult(auth); } catch(e) { err('Erro: ' + e.message); }
  
  onAuthStateChanged(auth, user => {
    if (user) { show('dashboard'); }
    else       { show('login'); }   // ← Android nunca chega aqui
  });

  setTimeout(() => {
    if (document.getElementById('view-loading').classList.contains('active'))
      show('login');
  }, 6000);
})();
```

**Causas do loop no Android:**

**Causa A — `getRedirectResult` nunca resolve no Android PWA:**  
No Android, quando o app está instalado como PWA (Add to Home Screen), o Chrome usa um processo separado. `getRedirectResult()` retorna `null` na primeira chamada mas pode **ficar pendente indefinidamente** em certas versões do Android WebView/Chrome quando há um redirect em andamento (estado intermediário persistido no `localStorage`). O resultado: o `await getRedirectResult(auth)` trava, `onAuthStateChanged` nunca dispara, e o `view-loading` permanece ativo. O `setTimeout` de 6 segundos deveria cobrir isso, **mas** se o redirect estiver pendente, o Firebase pode disparar `onAuthStateChanged` com `null` DEPOIS do timeout, causando um flash login → loading → login em loop.

**Causa B — `browserLocalPersistence` + redirect state corrompido no Android:**  
O Android Chrome salva o estado de redirect em `localStorage`. Se o usuário tentou logar, o popup foi bloqueado, o redirect foi iniciado, e o app foi fechado ou o Chrome matou a aba, o `localStorage` fica com um estado de redirect incompleto. Na próxima abertura, `getRedirectResult()` tenta processar esse estado corrompido, falha silenciosamente, `onAuthStateChanged` dispara com `null`, e o app mostra login. Se o usuário tenta de novo, o ciclo se repete.

**Causa C — `signInWithPopup` bloqueado no Android PWA:**  
Android Chrome em modo PWA **bloqueia popups por padrão** (considera não-user-gesture se houver qualquer `await` antes da chamada). O código atual tenta popup → captura erro → chama redirect. No Android, o redirect é iniciado mas a URL de callback do Firebase (`/__/auth/handler`) é processada pelo proxy NGINX, que **não tem regra para esse path** (`portaria_nginx.conf` só tem `location /`). O proxy funciona via `http-proxy-middleware` no Node.js server, não no NGINX — então em produção com NGINX na frente, o `/__/auth/` path pode não chegar ao Node.

### Correção Completa (Android + iOS)

**Arquivo:** `cloud_backend/public/app.js`

**Estratégia:** Substituir o fluxo popup+redirect por **redirect puro no Android, popup puro no iOS**. Adicionar detecção de estado corrompido e limpeza automática.

```javascript
// ── Auth — Substituir a seção Boot e btn-login completa ──────────────

// Detecta se é Android (Chrome/WebView)
const isAndroid = /Android/i.test(navigator.userAgent);

// Limpa estado de redirect corrompido se existir por mais de 2 minutos
const REDIRECT_KEY = 'rlight_redirect_ts';
const redirectTs = parseInt(localStorage.getItem(REDIRECT_KEY) || '0');
if (redirectTs && Date.now() - redirectTs > 120000) {
  localStorage.removeItem(REDIRECT_KEY);
  // Força limpeza do estado Firebase de redirect corrompido
  sessionStorage.clear();
}

document.getElementById('btn-login').onclick = function() {
  err('');
  if (isAndroid) {
    // Android: sempre redirect (popup é bloqueado em PWA)
    localStorage.setItem(REDIRECT_KEY, Date.now().toString());
    signInWithRedirect(auth, provider);
  } else {
    // iOS/Desktop: popup primeiro, redirect como fallback
    signInWithPopup(auth, provider).then(result => {
      console.log('Popup login OK:', result.user.email);
    }).catch(e => {
      if (e.code !== 'auth/popup-closed-by-user') {
        localStorage.setItem(REDIRECT_KEY, Date.now().toString());
        signInWithRedirect(auth, provider);
      } else {
        err('Login cancelado.');
      }
    });
  }
};

// ── Boot ───────────────────────────────────────────────────────────────
(async () => {
  await setPersistence(auth, browserLocalPersistence);

  // Processa resultado de redirect com timeout de segurança
  try {
    const redirectResult = await Promise.race([
      getRedirectResult(auth),
      new Promise((_, reject) => setTimeout(() => reject(new Error('timeout')), 5000))
    ]);
    if (redirectResult?.user) {
      localStorage.removeItem(REDIRECT_KEY);
    }
  } catch(e) {
    if (e.message !== 'timeout') err('Erro no login: ' + e.message);
    // Timeout ou erro: continua, onAuthStateChanged vai resolver
  }

  // Auth state — com timeout de segurança contra loop
  let authResolved = false;
  onAuthStateChanged(auth, user => {
    authResolved = true;
    localStorage.removeItem(REDIRECT_KEY);
    if (user) {
      const first = user.displayName ? user.displayName.split(' ')[0] : 'Willian';
      document.getElementById('greeting').textContent = 'Olá, ' + first;
      show('dashboard');
      loadHistory();
    } else {
      show('login');
    }
  });

  // Failsafe: se Firebase não respondeu em 8s, mostra login
  setTimeout(() => {
    if (!authResolved) {
      console.warn('[Auth] Firebase timeout — mostrando login');
      show('login');
    }
  }, 8000);

  const p = new URLSearchParams(location.search);
  const tok = p.get('token') || p.get('id');
  if (tok) verifyReceipt(tok);
})();
```

**Arquivo:** `portaria_nginx.conf`

Adicionar regra para o callback do Firebase auth chegar ao Node.js:

```nginx
server {
    listen 80;
    server_name portaria.rlight.com.br;

    # Firebase Auth callback — DEVE chegar ao Node antes de qualquer rewrite
    location /__/ {
        proxy_pass http://127.0.0.1:3005;
        proxy_http_version 1.1;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
    }

    location / {
        proxy_pass http://127.0.0.1:3005;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection 'upgrade';
        proxy_set_header Host $host;
        proxy_cache_bypass $http_upgrade;
    }
}
```

**Arquivo:** `cloud_backend/public/index.html`

Adicionar meta tag para Android PWA (Chrome precisa de `mobile-web-app-capable`):

```html
<!-- Adicionar após as meta tags Apple existentes -->
<meta name="mobile-web-app-capable" content="yes">
<!-- Garante que o Chrome no Android trate como PWA standalone -->
```

**Arquivo:** `cloud_backend/public/manifest.json`

Adicionar `shortcuts` e corrigir `display_override` para Android:

```json
{
  "short_name": "RLight Portal",
  "name": "RLight Portaria Digital",
  "icons": [
    { "src": "/favicon_v2.png", "type": "image/png", "sizes": "512x512", "purpose": "any maskable" }
  ],
  "start_url": "/",
  "background_color": "#080810",
  "display": "standalone",
  "display_override": ["standalone", "minimal-ui"],
  "orientation": "portrait",
  "scope": "/",
  "theme_color": "#000000"
}
```

**Nota:** `"purpose": "any maskable"` é obrigatório para o Chrome no Android não mostrar ícone com fundo branco na tela inicial.

---

## PARTE 2 — BUGS RESTANTES DO FIRMWARE (ESP32-S3)

### 2.1 — `delay(50)` em `_handleIdle` (RESOLVIDO na revisão atual)

✅ **Já corrigido** — verificado em `StateMachine.cpp`. Usa `_ctx.btn_debounce_ms` com timestamp.

### 2.2 — `vTaskDelay(200ms)` em `_handleAuthorized` (RESOLVIDO)

✅ **Já corrigido** — verificado. Usa split não-bloqueante com `_ctx.ina_checked`.

### 2.3 — `String` em `_handleWaitingPass` (RESOLVIDO)

✅ **Já corrigido** — verificado. Usa `const char* code = KeypadHandler::instance().getCode()`.

### 2.4 — `_persistDeliveryContext()` usa `p.putString()` (PENDENTE)

**Arquivo:** `src/fsm/StateMachine.cpp`

```cpp
// ATUAL — usa String do Arduino internamente via Preferences.putString()
void StateMachine::_persistDeliveryContext() {
  Preferences p;
  p.begin("dlv_ctx", false);
  p.putString("qr",      _ctx.qr_code);   // putString aceita const char* — OK
  p.putString("carrier", _ctx.carrier);   // idem — OK
  p.putULong ("ts",      _ctx.unix_time);
  p.putBool  ("active",  true);
  p.end();
}
```

**Avaliação:** `Preferences::putString(key, const char*)` é uma overload que **não usa `String` do Arduino** — usa `const char*` diretamente no NVS. **Não é um bug.** Pode ser deixado como está.

### 2.5 — `_handleResidentP2` abre P2 múltiplas vezes (BUG REAL)

**Arquivo:** `src/fsm/StateMachine.cpp`

```cpp
void StateMachine::_handleResidentP2(const PhysicalState& w) {
  // ...
  if (millis() - _ctx.resident_p2_timer < cfg.p2_delay_ms) return;

  if (cfg.enable_strike_p2) {
    if (!w.p1_open) {
      Strike::P2().open(cfg.p2_open_ms);  // ← CHAMADO A CADA TICK APÓS O DELAY!
      Buzzer::beep(1, 200);               // ← Bipe repetido a cada 50ms
      Led::btn().solid(255);
    }
  }
  // aguarda p2_open...
```

**Problema:** Após o delay de 2000ms, a cada tick de 10ms (100Hz), o handler chama `Strike::P2().open()` e `Buzzer::beep()` repetidamente até que `w.p2_open` seja true. O strike é acionado dezenas de vezes.

**Correção:**

```cpp
void StateMachine::_handleResidentP2(const PhysicalState& w) {
  auto& cfg = ConfigManager::instance().cfg;

  if (w.p1_open) {
    UsbBridge::instance().sendAlert("RESIDENT_P1_REOPENED_BEFORE_P2", _ctx);
    _ctx.resident_p2_timer = 0;
    _ctx.p2_opened = false; // Reset da flag
    transition(State::IDLE);
    return;
  }

  if (millis() - _ctx.resident_p2_timer < cfg.p2_delay_ms) return;

  // Abre P2 uma única vez (flag p2_opened no FsmContext)
  if (cfg.enable_strike_p2 && !_ctx.p2_opened && !w.p1_open) {
    Strike::P2().open(cfg.p2_open_ms);
    Buzzer::beep(1, 200);
    Led::btn().solid(255);
    _ctx.p2_opened = true; // Garante acionamento único
  }

  if (w.p2_open) {
    UsbBridge::instance().sendEvent("RESIDENT_ACCESS_COMPLETE", _ctx);
    _ctx.resident_p2_timer = 0;
    _ctx.p2_opened = false;
    transition(State::IDLE);
    return;
  }

  if (millis() - _ctx.resident_p2_timer > cfg.door_open_ms + 3000) {
    UsbBridge::instance().sendAlert("RESIDENT_P2_TIMEOUT", _ctx);
    _ctx.resident_p2_timer = 0;
    _ctx.p2_opened = false;
    transition(State::IDLE);
  }
}
```

**Adicionar em `StateMachine.h` (FsmContext):**
```cpp
bool p2_opened = false;
```

**Resetar em `transition()`:**
```cpp
void StateMachine::transition(State next) {
  _ctx.prev_duration_ms = millis() - _ctx.state_enter;
  _onExit(_ctx.state);
  _ctx.state       = next;
  _ctx.state_enter = millis();
  _ctx.ina_checked = false;
  _ctx.p2_opened   = false; // Adicionar
  _onEnter(next);
  UsbBridge::instance().sendState(next, _ctx);
}
```

---

## PARTE 3 — CLOUD BACKEND: PROBLEMAS DE SEGURANÇA E PRODUÇÃO

### 3.1 — JWT_SECRET com valor padrão inseguro (CRÍTICO)

**Arquivo:** `cloud_backend/server.js`

```javascript
// ATUAL — fallback hardcoded inseguro
const JWT_SECRET = process.env.JWT_SECRET || 'rlight_v6_base_secret';
const JWT_PREV_SECRET = process.env.JWT_PREV_SECRET || 'rlight_v6_base_secret_old';
```

**Problema:** Em produção, se `JWT_SECRET` não estiver no ambiente (`.env` não carregado, systemd sem `EnvironmentFile`), o servidor usa o segredo padrão que está publicado no repositório. Qualquer JWT forjado com esse segredo seria aceito.

**Correção:**

```javascript
const JWT_SECRET = process.env.JWT_SECRET;
const JWT_PREV_SECRET = process.env.JWT_PREV_SECRET;

if (!JWT_SECRET) {
  console.error('[FATAL] JWT_SECRET não definido. Defina no .env ou variável de ambiente.');
  process.exit(1);
}
```

**Adicionar ao deploy service** `deploy/rlight-cloud.service`:
```ini
[Service]
EnvironmentFile=/opt/rlight/cloud_backend/.env
```

### 3.2 — `/api/history` retorna dados mock hardcoded (ALTO)

**Arquivo:** `cloud_backend/server.js`

```javascript
// ATUAL — dados fake hardcoded
app.get('/api/history', (req, res) => {
    res.json([
        { id: 'RL-9281', carrier: 'Mercado Livre', weight: '1240g', ts: Date.now() - 3600000 },
        ...
    ]);
});
```

**Problema:** O dashboard do usuário no Android/iOS sempre mostra as mesmas 3 entregas fake. As entregas reais estão no SQLite do OPi (`/var/lib/rlight/deliveries.db`) e precisam chegar ao cloud via `oracle_api.py`. Enquanto o sync não está completo, o cloud_backend deveria pelo menos ter um endpoint que lê do SQLite ou retorna dados reais via proxy do OPi.

**Opções (escolher uma):**

**Opção A — Proxy para o OPi (recomendado para começar):**
```javascript
const { createProxyMiddleware } = require('http-proxy-middleware');
const OPI_HOST = process.env.OPI_HOST || 'http://192.168.1.100:8080';

app.get('/api/history', createProxyMiddleware({
  target: OPI_HOST,
  changeOrigin: true,
  pathRewrite: { '^/api/history': '/entregas' }
}));
```

**Opção B — Banco SQLite no cloud (se Oracle/sync estiver funcionando):**
```javascript
const Database = require('better-sqlite3');
const db = new Database(process.env.DB_PATH || '/var/lib/rlight/deliveries.db', { readonly: true });

app.get('/api/history', (req, res) => {
  const rows = db.prepare('SELECT * FROM deliveries ORDER BY ts DESC LIMIT 20').all();
  res.json(rows.map(r => ({
    id: r.token_uuid,
    carrier: r.carrier,
    weight: r.weight_g + 'g',
    ts: r.ts * 1000,
    photo_path: r.photo_path
  })));
});
```

### 3.3 — `/api/doors` retorna estado mock (ALTO)

**Arquivo:** `cloud_backend/server.js`

O estado das portas (`doorState`) é um objeto em memória que nunca é atualizado com dados reais do ESP32. O controle remoto no app mostra sempre "Sem sinal" ou "Fechado" — nunca o estado real.

**Correção:** O cloud_backend deve receber o estado real via proxy para o endpoint `/api/doors` do OPi, ou o host Python deve fazer push via WebSocket/MQTT para o cloud.

**Solução mais simples (proxy):**
```javascript
app.get('/api/doors', createProxyMiddleware({
  target: OPI_HOST,
  changeOrigin: true,
  pathRewrite: { '^/api/doors': '/doors' }
}));
```

**Adicionar em `host/local_api.py`:**
```python
@app.get("/doors")
def get_door_state():
    return {
        "p1":   {"reed_open": host_fsm.get_p1_open(),   "strike_active": False},
        "p2":   {"reed_open": host_fsm.get_p2_open(),   "strike_active": False},
        "gate": {"reed_open": host_fsm.get_gate_open(), "relay_active":  False},
        "ts":   int(time.time() * 1000),
        "age_ms": 0
    }
```

### 3.4 — `/api/actuate` aciona mock, não o hardware real (ALTO)

Mesmo problema: `actuate` apenas muda `doorState` em memória. Não envia nenhum comando ao ESP32.

**Correção em `cloud_backend/server.js`:**
```javascript
app.post('/api/actuate', async (req, res) => {
    const { device } = req.body;
    if (!['p1','p2','gate'].includes(device))
        return res.status(400).json({ ok: false, error: 'Dispositivo inválido' });
    
    try {
        const r = await fetch(`${OPI_HOST}/actuate`, {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({ device }),
            signal: AbortSignal.timeout(5000)
        });
        const data = await r.json();
        res.json(data);
    } catch(e) {
        res.status(503).json({ ok: false, error: 'OPi inacessível: ' + e.message });
    }
});
```

**Adicionar em `host/local_api.py`:**
```python
from core.serial_bridge import esp32_bridge

@app.post("/actuate")
def actuate_device(body: dict):
    device = body.get("device")
    cmd_map = {"p1": "CMD_OPEN_P1", "p2": "CMD_OPEN_P2", "gate": "CMD_OPEN_GATE"}
    if device not in cmd_map:
        return {"ok": False, "error": "Dispositivo inválido"}
    esp32_bridge.send_cmd(cmd_map[device])
    return {"ok": True}
```

**Adicionar os handlers em `UsbBridge.cpp`** para CMD_OPEN_P1, CMD_OPEN_P2, CMD_OPEN_GATE.

### 3.5 — `/api/settings` usa mock em memória, não persiste (MÉDIO)

As configurações salvas pelo app são perdidas ao reiniciar o Node.js. O correto é encaminhar para o OPi que encaminha ao ESP32 via `CMD_UPDATE_CFG`.

**Correção:**
```javascript
app.post('/api/settings', async (req, res) => {
    try {
        await fetch(`${OPI_HOST}/settings`, {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify(req.body)
        });
        res.json({ ok: true });
    } catch(e) {
        res.status(503).json({ ok: false, error: 'OPi inacessível' });
    }
});
```

**Adicionar em `host/local_api.py`:**
```python
@app.post("/settings")
def update_settings(body: dict):
    for key, val in body.items():
        esp32_bridge.send_cmd("CMD_UPDATE_CFG", {key: val})
    return {"ok": True}
```

---

## PARTE 4 — INTERFACE KIOSK (host/ui/web)

### 4.1 — Fotos no `det-photo` são URLs de Unsplash hardcoded (ALTO)

**Arquivo:** `cloud_backend/public/app.js` — função `showDetail()`

```javascript
// ATUAL — imagens fake de placeholder
const imgs = [
  'https://images.unsplash.com/photo-...',
  'https://images.unsplash.com/photo-...',
  'https://images.unsplash.com/photo-...'
];
document.getElementById('det-photo').src = imgs[i % 3];
```

**Correção:** Usar o `photo_path` do item de entrega. Como as fotos ficam em `/tmp/rlight_photos/` no OPi e são servidas via `/photos/{token}.jpg` pelo local_api.py, o cloud_backend deve fazer proxy desse endpoint também:

```javascript
// No cloud_backend/server.js
app.get('/photos/:token', createProxyMiddleware({
  target: OPI_HOST,
  changeOrigin: true
}));
```

```javascript
// No showDetail() — usar photo real
function showDetail(i) {
  const item = deliveries[i];
  // ...
  const photoUrl = item.photo_path 
    ? `/photos/${item.token_uuid}.jpg` 
    : '';
  const photoEl = document.getElementById('det-photo');
  if (photoUrl) {
    photoEl.src = photoUrl;
    photoEl.onerror = () => { photoEl.src = ''; photoEl.parentElement.style.display = 'none'; };
  } else {
    photoEl.parentElement.style.display = 'none';
  }
  show('details');
}
```

### 4.2 — Kiosk: `show(state)` usa `view-WAITING_PASS` mas index.html tem `view-WAITING_PASS` (VERIFICAR)

**Status verificado:** `cloud_backend/public/index.html` tem `id="view-WAITING_PASS"` e `id="view-LOCKOUT_KEYPAD"` já criados com telas completas. `app.js` tem `show(state)` que monta `view-${state}`. ✅ **Já funciona.**

O `handleMessage` no `cloud_backend/public/app.js` trata `KEY_PRESSED` e atualiza os dots. ✅ **Já funciona.**

**PORÉM:** Esse é o app do cloud (Oracle), não o kiosk local. O kiosk local está em `host/ui/web/app.js`. Os dois são apps separados e **ambos precisam ter os mesmos handlers**. Verificar se `host/ui/web/app.js` também tem os mesmos tratamentos (ver seção 4.3).

### 4.3 — host/ui/web/app.js: verificar paridade com cloud app.js

Os dois apps (kiosk local e portal cloud) são mantidos separadamente. Garantir que `host/ui/web/app.js` tenha:
- `statesDict` com WAITING_PASS e LOCKOUT_KEYPAD ✅ (verificado)
- Handler `KEY_PRESSED` ✅ (verificado)
- Handler para `telemetry_update` com campos p1_open, p2_open, gate_open ✅

---

## PARTE 5 — PRODUÇÃO: ITENS OBRIGATÓRIOS ANTES DO DEPLOY

### 5.1 — SSL/HTTPS obrigatório para Firebase Auth no Android (CRÍTICO)

**Problema:** O Firebase Auth exige HTTPS para funcionar em domínios que não sejam localhost. O NGINX atual só tem `listen 80`. Em HTTP puro:
- O `signInWithPopup` falha silenciosamente
- O `signInWithRedirect` pode funcionar mas o callback falha
- O Chrome no Android bloqueia cookies de terceiros sem HTTPS

**Correção do `portaria_nginx.conf`:**
```nginx
server {
    listen 80;
    server_name portaria.rlight.com.br;
    return 301 https://$host$request_uri;
}

server {
    listen 443 ssl http2;
    server_name portaria.rlight.com.br;

    ssl_certificate     /etc/letsencrypt/live/portaria.rlight.com.br/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/portaria.rlight.com.br/privkey.pem;
    ssl_protocols TLSv1.2 TLSv1.3;
    ssl_ciphers ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256;
    ssl_prefer_server_ciphers off;

    add_header Strict-Transport-Security "max-age=63072000" always;
    add_header X-Content-Type-Options nosniff;
    add_header X-Frame-Options SAMEORIGIN;

    # Firebase Auth callback
    location /__/ {
        proxy_pass http://127.0.0.1:3005;
        proxy_http_version 1.1;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-Proto https;
    }

    location / {
        proxy_pass http://127.0.0.1:3005;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection 'upgrade';
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-Proto https;
        proxy_cache_bypass $http_upgrade;
    }
}
```

**Comando para obter certificado Let's Encrypt:**
```bash
certbot --nginx -d portaria.rlight.com.br --non-interactive --agree-tos -m admin@rlight.com.br
```

### 5.2 — Firebase: Domínio autorizado (CRÍTICO)

No Firebase Console → Authentication → Settings → Authorized domains:
- `localhost` ✅ (já adicionado por padrão)
- `portaria.rlight.com.br` ← **Adicionar manualmente**

Sem isso, o Google OAuth bloqueia o redirect de volta para o domínio.

### 5.3 — `cloud_backend/package.json` não tem `node-fetch` (MÉDIO)

```json
// cloud_backend/package.json — verificar dependências atuais
```

Para as correções das Partes 3 (proxy fetch para OPi), adicionar se não existir:
```bash
npm install node-fetch@3
```
Ou usar o `fetch` nativo do Node 18+ (sem import extra).

### 5.4 — OPi_HOST configurável por variável de ambiente (MÉDIO)

```javascript
// cloud_backend/server.js
const OPI_HOST = process.env.OPI_HOST || 'http://192.168.1.100:8080';
```

Adicionar ao `.env.example`:
```
OPI_HOST=http://192.168.1.100:8080
```

### 5.5 — `deploy/rlight-cloud.service`: sem EnvironmentFile (MÉDIO)

**Arquivo:** `deploy/rlight-cloud.service`

Adicionar:
```ini
[Service]
EnvironmentFile=/opt/rlight/cloud_backend/.env
WorkingDirectory=/opt/rlight/cloud_backend
ExecStart=/usr/bin/node server.js
Restart=always
RestartSec=5
```

---

## PARTE 6 — POLIMENTO E UX ANDROID/PWA

### 6.1 — Banner de instalação PWA não funciona no Android

**Arquivo:** `cloud_backend/public/app.js`

```javascript
// ATUAL — só mostra banner no iOS
const isIOS = /iPad|iPhone|iPod/.test(navigator.userAgent);
if (isIOS && !isStandalone && !localStorage.getItem('pwa-dismissed')) {
  setTimeout(() => { document.getElementById('pwa-banner').style.display = 'block'; }, 3000);
}
```

**No Android**, o Chrome dispara o evento `beforeinstallprompt` nativamente. Capturá-lo:

```javascript
// cloud_backend/public/app.js — adicionar no topo
let deferredInstallPrompt = null;

window.addEventListener('beforeinstallprompt', (e) => {
  e.preventDefault();
  deferredInstallPrompt = e;
  // Mostra banner Android após 3s se não instalado
  if (!isStandalone && !localStorage.getItem('pwa-dismissed')) {
    setTimeout(() => {
      const banner = document.getElementById('pwa-banner');
      // Atualiza texto do banner para Android
      banner.querySelector('p:last-of-type').textContent = 
        'Toque em "Instalar" para adicionar à tela inicial';
      banner.style.display = 'block';
    }, 3000);
  }
});

// Botão de fechar banner — aproveitar para instalar no Android
document.querySelector('#pwa-banner button').addEventListener('click', () => {
  if (deferredInstallPrompt) {
    deferredInstallPrompt.prompt();
    deferredInstallPrompt.userChoice.then(() => { deferredInstallPrompt = null; });
  }
  document.getElementById('pwa-banner').style.display = 'none';
  localStorage.setItem('pwa-dismissed', '1');
});
```

### 6.2 — `weekly_report.py`: send_email() não implementado

**Arquivo:** `host/weekly_report.py`

```python
# Substituir o print() atual por SMTP real
import smtplib
import os
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText

def send_email(subject: str, html_body: str):
    smtp_host = os.getenv("SMTP_HOST", "smtp.gmail.com")
    smtp_port = int(os.getenv("SMTP_PORT", "587"))
    smtp_user = os.getenv("SMTP_USER")
    smtp_pass = os.getenv("SMTP_PASS")
    smtp_from = os.getenv("SMTP_FROM", smtp_user)
    smtp_to   = os.getenv("SMTP_TO")

    if not all([smtp_user, smtp_pass, smtp_to]):
        print("[Email] Credenciais SMTP não configuradas — relatório não enviado")
        return

    msg = MIMEMultipart("alternative")
    msg["Subject"] = subject
    msg["From"]    = smtp_from
    msg["To"]      = smtp_to
    msg.attach(MIMEText(html_body, "html"))

    try:
        with smtplib.SMTP(smtp_host, smtp_port, timeout=10) as server:
            server.starttls()
            server.login(smtp_user, smtp_pass)
            server.sendmail(smtp_from, smtp_to, msg.as_string())
        print(f"[Email] Relatório enviado para {smtp_to}")
    except Exception as e:
        print(f"[Email] Falha ao enviar: {e}")
```

### 6.3 — `AccessController::listCodes()` não implementada

**Arquivo:** `src/sensors/AccessController.cpp`

```cpp
// Implementação usando índice próprio em NVS (mais simples que nvs_entry_find)
// Mantém uma chave "index" com lista CSV de códigos conhecidos

void AccessController::addCode(const char* code, AccessType type, const char* label) {
  Preferences p;
  p.begin("access_db", false);
  
  // Salva o código
  char key[20];
  snprintf(key, sizeof(key), "c_%s", code);
  // Formato: "type:label"
  char val[48];
  snprintf(val, sizeof(val), "%d:%s", (int)type, label);
  p.putString(key, val);
  
  // Atualiza índice CSV
  char idx[512] = "";
  String existing = p.getString("index", "");
  if (existing.length() > 0 && existing.length() < 480) {
    snprintf(idx, sizeof(idx), "%s,%s", existing.c_str(), code);
  } else {
    strlcpy(idx, code, sizeof(idx));
  }
  p.putString("index", idx);
  p.end();
}

// listCodes retorna JSON array via UsbBridge
void AccessController::listCodes(char* out_json, size_t out_size) {
  Preferences p;
  p.begin("access_db", true); // read-only
  String idx = p.getString("index", "");
  
  // Constrói JSON manualmente para evitar heap
  size_t pos = 0;
  pos += snprintf(out_json + pos, out_size - pos, "[");
  
  if (idx.length() > 0) {
    char idx_buf[512];
    strlcpy(idx_buf, idx.c_str(), sizeof(idx_buf));
    char* token = strtok(idx_buf, ",");
    bool first = true;
    while (token && pos < out_size - 64) {
      char key[20];
      snprintf(key, sizeof(key), "c_%s", token);
      String val = p.getString(key, "");
      if (val.length() > 0) {
        if (!first) pos += snprintf(out_json + pos, out_size - pos, ",");
        pos += snprintf(out_json + pos, out_size - pos, 
          "{\"code\":\"%s\",\"type\":%c,\"label\":\"%s\"}",
          token, val[0], val.length() > 2 ? val.c_str() + 2 : "");
        first = false;
      }
      token = strtok(nullptr, ",");
    }
  }
  
  snprintf(out_json + pos, out_size - pos, "]");
  p.end();
}
```

---

## PARTE 7 — CHECKLIST FINAL DE DEPLOY

### 7.1 — Checklist de Infraestrutura

```
[ ] SSL/HTTPS configurado com certbot para portaria.rlight.com.br
[ ] portaria_nginx.conf atualizado com HTTPS e location /__/
[ ] Firebase Console: portaria.rlight.com.br adicionado em Authorized Domains
[ ] cloud_backend/.env criado com JWT_SECRET real (não o padrão do repositório)
[ ] deploy/rlight-cloud.service com EnvironmentFile apontando para .env
[ ] deploy/rlight-host.service verificado e habilitado (systemctl enable rlight-host)
[ ] /var/lib/rlight/ criado com permissões corretas (chown rlight:rlight)
[ ] /tmp/rlight_photos/ criado (tmpfs — recriado no boot)
[ ] /dev/rlight_esp rule de udev instalada
```

### 7.2 — Checklist de Código

```
[ ] PARTE 1: Bug Android — app.js substituído com isAndroid + redirect puro
[ ] PARTE 1: portaria_nginx.conf com location /__/
[ ] PARTE 1: manifest.json com purpose: "any maskable"
[ ] PARTE 1: meta mobile-web-app-capable no index.html
[ ] PARTE 2: _handleResidentP2 com flag p2_opened no FsmContext
[ ] PARTE 3.1: JWT_SECRET sem fallback padrão — process.exit(1) se ausente
[ ] PARTE 3.2: /api/history conectado ao OPi ou banco real
[ ] PARTE 3.3: /api/doors proxy para OPi + endpoint /doors no local_api.py
[ ] PARTE 3.4: /api/actuate proxy para OPi + endpoint /actuate no local_api.py
[ ] PARTE 3.5: /api/settings proxy para OPi + endpoint /settings no local_api.py
[ ] PARTE 4.1: fotos reais no showDetail() em vez de Unsplash
[ ] PARTE 4.1: /photos/:token proxy no cloud_backend
[ ] PARTE 5.4: OPI_HOST via variável de ambiente
[ ] PARTE 6.1: beforeinstallprompt para Android PWA install
[ ] PARTE 6.2: send_email() com SMTP real
[ ] PARTE 6.3: AccessController::listCodes() implementado
```

### 7.3 — Checklist de Teste

```
[ ] Android Chrome (não PWA): login com Google → dashboard carrega
[ ] Android Chrome (PWA instalado): login com Google → dashboard carrega (sem loop)
[ ] iOS Safari (não PWA): login com Google → dashboard carrega
[ ] iOS Safari (PWA instalado): login com Google → dashboard carrega (sem faixa preta)
[ ] Controle remoto: estado das portas atualiza em tempo real
[ ] Controle remoto: botão "Acionar" move o strike físico
[ ] Kiosk: digitar senha no teclado físico → asteriscos aparecem na tela
[ ] Kiosk: código correto → RESIDENT_P1 → P1 abre → P2 abre uma vez (não múltiplas)
[ ] Kiosk: código errado 5x → LOCKOUT_KEYPAD → countdown aparece
[ ] Entrega completa: QR → AUTHORIZED → DELIVERING → RECEIPT → foto vinculada ao JWT
[ ] JWT verify: link do comprovante abre a entrega correta no app
[ ] Home Assistant: entidades aparecem como v8 (não v6)
[ ] Relatório semanal: e-mail chega na caixa correta
```

---

## RESUMO EXECUTIVO

| Categoria | Itens | Prioridade |
|-----------|-------|-----------|
| Bug Android (loop sessão) | 4 arquivos | 🔴 BLOQUEADOR |
| Firmware _handleResidentP2 | 2 arquivos | 🔴 FUNCIONAL |
| Segurança JWT_SECRET | 1 arquivo | 🔴 SEGURANÇA |
| APIs mock → reais | 4 endpoints | 🟡 PRODUÇÃO |
| SSL/HTTPS + Firebase domínio | Infra | 🔴 BLOQUEADOR |
| PWA Android install | 1 arquivo | 🟡 UX |
| Fotos reais no detalhe | 2 arquivos | 🟡 UX |
| send_email SMTP | 1 arquivo | 🟢 COMPLETUDE |
| listCodes NVS | 1 arquivo | 🟢 COMPLETUDE |

**Tempo estimado de implementação:** 4-6 horas de desenvolvimento focado.  
**Estado após implementação:** Sistema pronto para produção real.