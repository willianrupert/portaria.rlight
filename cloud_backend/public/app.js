// iOS PWA black bar fix: panel uses height extension instead of bottom manipulation
(function() {
  // Keep any other initialization code here if needed
})();

import { initializeApp } from 'https://www.gstatic.com/firebasejs/10.7.1/firebase-app.js';
import {
  getAuth, onAuthStateChanged, GoogleAuthProvider,
  signInWithPopup, signInWithRedirect, getRedirectResult,
  signOut, setPersistence, browserLocalPersistence
} from 'https://www.gstatic.com/firebasejs/10.7.1/firebase-auth.js';

const cfg = {
  apiKey:'AIzaSyA2RurqtI_a6yXM1iPW-HByLJ18yZ0fy6c',
  authDomain:'portaria-rlight.firebaseapp.com',
  projectId:'portaria-rlight',
  storageBucket:'portaria-rlight.firebasestorage.app',
  messagingSenderId:'810258356796',
  appId:'1:810258356796:web:ad46f619642a22f06d0139',
  measurementId:'G-JGQTZE0W5B'
};

const app = initializeApp(cfg);
const auth = getAuth(app);
const provider = new GoogleAuthProvider();
let deliveries = [];
let keypadAsterisks = 0;
let lockoutTimer = null;

// Detecta se é Android (Chrome/WebView)
const isAndroid = /Android/i.test(navigator.userAgent);
const REDIRECT_KEY = 'rlight_redirect_ts';

// ── Utilities ──────────────────────────────────────
function show(id) {
  document.querySelectorAll('.view').forEach(v => v.classList.remove('active'));
  document.getElementById('view-' + id).classList.add('active');
}
function err(msg) { document.getElementById('err').textContent = msg || ''; }

// Expose for inline onclick
window._resetAuth = () => { sessionStorage.clear(); location.reload(); };

// ── WebSocket (Host OPi) ───────────────────────────
let socket = null;
const isLocal = location.hostname === 'localhost' || location.hostname === '127.0.0.1' || location.hostname.startsWith('192.168.');

if (isLocal) {
  try {
    socket = new WebSocket('ws://' + location.hostname + ':3006');
    socket.onmessage = (event) => {
      try {
        const data = JSON.parse(event.data);
        handleMessage(data);
      } catch (e) { console.error('WS parse error:', e); }
    };
    socket.onerror = () => console.warn('WebSocket error (OPi offline?)');
  } catch (e) {
    console.error('WebSocket initialization failed:', e);
  }
}

function handleMessage(data) {
  if (data.type === 'state_change') {
    const state = data.state;
    const statesDict = {
      'IDLE': 'Aguardando...',
      'AWAKE': 'Iniciando...',
      'WAITING_QR': 'Aguardando QR Code',
      'WAITING_PASS': 'Verificando Acesso',
      'LOCKOUT_KEYPAD': '⚠ Teclado Bloqueado',
      'AUTHORIZED': 'Acesso Autorizado',
      'INSIDE_WAIT': 'Pode entrar',
      'DELIVERING': 'Depositando...',
      'DOOR_REOPENED': 'Pode sair',
      'CONFIRMING': 'Confirmando...',
      'RECEIPT': 'Comprovante',
      'ABORTED': 'Entrega Cancelada',
      'ERROR': 'Erro no Sistema',
      'RESIDENT_P1': 'Bem-vindo!',
      'RESIDENT_P2': 'Saindo...',
      'REVERSE_PICKUP': 'Coleta Reversa'
    };
    
    const label = statesDict[state] || state;
    // Se estiver em uma tela de 'view-' (PWA Admin), não forçamos a mudança de tela do Kiosk
    // exceto se for um estado crítico ou se o usuário quiser.
    // Mas para o Kiosk (localhost), trocamos a tela:
    if (isLocal) {
       if (state === 'WAITING_PASS') {
         keypadAsterisks = 0;
         const dots = document.getElementById('keypad-dots');
         if (dots) dots.innerHTML = '';
       }
       if (state === 'LOCKOUT_KEYPAD') startLockoutCountdown(600); // 10 min default
       show(state);
    }
  } 
  else if (data.type === 'KEY_PRESSED') {
    handleKeyPressed();
  }
}

function handleKeyPressed() {
  keypadAsterisks++;
  const container = document.getElementById('keypad-dots');
  if (container) {
    container.innerHTML = '';
    for(let i=0; i<keypadAsterisks; i++) {
      container.innerHTML += '<div style="width:20px;height:20px;background:var(--blue);border-radius:50%;"></div>';
    }
  }
}

function startLockoutCountdown(seconds) {
  if (lockoutTimer) clearInterval(lockoutTimer);
  let remaining = seconds;
  const el = document.getElementById('lockout-countdown');
  lockoutTimer = setInterval(() => {
    remaining--;
    const m = Math.floor(remaining / 60);
    const s = remaining % 60;
    if (el) el.textContent = `${m}:${s < 10 ? '0' : ''}${s}`;
    if (remaining <= 0) {
      clearInterval(lockoutTimer);
      location.reload();
    }
  }, 1000);
}

// ── Navigation ─────────────────────────────────────
document.getElementById('btn-settings').onclick  = () => show('settings');
document.getElementById('btn-remote').onclick     = () => { show('remote'); startRemotePolling(); };
document.getElementById('btn-back-dash').onclick  = () => { stopRemotePolling(); show('dashboard'); };
document.getElementById('btn-back-remote').onclick= () => { stopRemotePolling(); show('dashboard'); };
document.getElementById('btn-back-det').onclick   = () => show('dashboard');
document.getElementById('btn-logout').onclick     = () =>
  signOut(auth).then(() => { sessionStorage.clear(); location.reload(); });

// ── Remote Control ──────────────────────────────────
let remoteInterval = null;
let lastPollTs = 0;

function resolveStatus(reedOpen, strikeActive, ageMs) {
  if (ageMs > 8000) return 'UNKNOWN';
  if (reedOpen) return 'OPEN';
  if (strikeActive) return 'ACTUATING';
  return 'CLOSED';
}

const STATUS_LABEL = { CLOSED:'Fechado', ACTUATING:'Acionando...', OPEN:'Aberto', UNKNOWN:'Sem sinal' };
const STATUS_ICON  = { CLOSED:'🔒', ACTUATING:'🔓', OPEN:'🚨', UNKNOWN:'❓' };

function updateCard(id, status, reedOpen, strikeActive) {
  const card = document.getElementById('rc-' + id);
  card.className = 'remote-card ' + status;
  card.querySelector('.rc-status').textContent = STATUS_LABEL[status];
  card.querySelector('.rc-icon').textContent   = STATUS_ICON[status];
  const reedDot   = document.getElementById('rc-' + id + '-reed');
  const strikeDot = document.getElementById('rc-' + id + (id === 'gate' ? '-strike' : '-strike'));
  if (reedDot)   { reedDot.className   = 'rc-dot ' + (reedOpen     ? 'on' : 'off'); }
  if (strikeDot) { strikeDot.className = 'rc-dot ' + (strikeActive ? 'on' : 'off'); }
  const btn = document.getElementById('rc-btn-' + id);
  btn.disabled = (status === 'ACTUATING');
  btn.textContent = status === 'OPEN' ? 'Fechar?' : status === 'ACTUATING' ? '⏳ Aguarde...' : 'Acionar';
}

async function pollDoors() {
  try {
    const r = await fetch('/api/doors');
    const d = await r.json();
    lastPollTs = Date.now();
    updateCard('p1',   resolveStatus(d.p1.reed_open,   d.p1.strike_active,  d.age_ms), d.p1.reed_open,   d.p1.strike_active);
    updateCard('p2',   resolveStatus(d.p2.reed_open,   d.p2.strike_active,  d.age_ms), d.p2.reed_open,   d.p2.strike_active);
    updateCard('gate', resolveStatus(d.gate.reed_open, d.gate.relay_active, d.age_ms), d.gate.reed_open, d.gate.relay_active);
  } catch(e) {
    updateCard('p1','UNKNOWN',false,false);
    updateCard('p2','UNKNOWN',false,false);
    updateCard('gate','UNKNOWN',false,false);
  }
}

function startRemotePolling() {
  pollDoors();
  remoteInterval = setInterval(pollDoors, 2000);
  // Age ticker
  setInterval(() => {
    if (!lastPollTs) return;
    const s = Math.round((Date.now() - lastPollTs) / 1000);
    const el = document.getElementById('rc-age');
    if (!el) return;
    el.textContent = s < 5 ? `Atualizado há ${s}s` : s < 30 ? `Há ${s}s` : `⚠️ Há ${s}s — verificar conexão`;
    el.style.color = s > 30 ? 'var(--red)' : s > 10 ? '#FFD60A' : 'var(--muted)';
  }, 1000);
}

function stopRemotePolling() {
  if (remoteInterval) { clearInterval(remoteInterval); remoteInterval = null; }
}

async function actuate(device) {
  const prev = document.getElementById('rc-' + device).className.split(' ')[1];
  if (prev === 'ACTUATING') return;
  updateCard(device, 'ACTUATING', false, true);
  try {
    const r = await fetch('/api/actuate', { method:'POST',
      headers:{'Content-Type':'application/json'}, body:JSON.stringify({device}),
      signal: AbortSignal.timeout(8000) });
    if (!r.ok) throw new Error('Erro servidor');
  } catch(e) {
    updateCard(device, prev, false, false);
  }
}

document.getElementById('rc-btn-p1').onclick    = () => actuate('p1');
document.getElementById('rc-btn-p2').onclick    = () => actuate('p2');
document.getElementById('rc-btn-gate').onclick  = () => actuate('gate');
document.getElementById('rc-refresh-btn').onclick = pollDoors;

// ── Send Command ───────────────────────────────────
async function sendCmd(cmd, val) {
  try {
    await fetch('/api/settings', { method:'POST',
      headers:{'Content-Type':'application/json'},
      body: JSON.stringify({ key: cmd, val }) });
    console.log(`[CFG] ${cmd} = ${val}`);
  } catch(e) { console.error('sendCmd error:', e); }
}

// Gera os 9 gates do radar mmWave dinamicamente
(function() {
  const container = document.getElementById('mmw-gates');
  if (!container) return;
  for (let n = 0; n < 9; n++) {
    container.innerHTML += `
      <div style="margin-bottom:8px;">
        <p style="font-size:11px;color:var(--muted);margin-bottom:4px;">Gate ${n} — ${(n*0.75).toFixed(2)}m até ${((n+1)*0.75).toFixed(2)}m</p>
        <div class="s-row" style="padding:4px 0;">
          <span class="s-label" style="font-size:12px;">Movimento</span>
          <div class="range-wrap"><input type="range" id="cfg-mmw-g${n}-mot" min="0" max="100" step="5" value="50" oninput="this.nextElementSibling.textContent=this.value"><span class="range-num">50</span></div>
        </div>
        <div class="s-row" style="padding:4px 0;border-bottom:${n<8?'1px solid var(--border)':'none'}">
          <span class="s-label" style="font-size:12px;">Estática</span>
          <div class="range-wrap"><input type="range" id="cfg-mmw-g${n}-sta" min="0" max="100" step="5" value="50" oninput="this.nextElementSibling.textContent=this.value"><span class="range-num">50</span></div>
        </div>
      </div>`;
  }
})();


// ── Save Settings ──────────────────────────────────
document.getElementById('btn-save').onclick = async function() {
  this.textContent = 'Salvando…'; this.disabled = true;
  // Collect all config fields
  const fields = [
    ['maint',          document.getElementById('cfg-maint')?.checked ? 1 : 0],
    ['always_open',    document.getElementById('cfg-always-open')?.checked ? 1 : 0],
    ['op_start_h',     document.getElementById('cfg-op-start')?.value],
    ['op_end_h',       document.getElementById('cfg-op-end')?.value],
    ['strike_p1_en',   document.getElementById('cfg-strike-p1-en')?.checked ? 1 : 0],
    ['door_open_ms',   document.getElementById('cfg-door-open-ms')?.value],
    ['auth_timeout_ms',document.getElementById('cfg-auth-timeout')?.value],
    ['door_alert_ms',  document.getElementById('cfg-door-alert-ms')?.value],
    ['strike_p2',      document.getElementById('cfg-strike-p2-en')?.checked ? 1 : 0],
    ['p2_open_ms',     document.getElementById('cfg-p2-open-ms')?.value],
    ['p2_delay_ms',    document.getElementById('cfg-p2-delay-ms')?.value],
    ['p2_access_window_ms', document.getElementById('cfg-p2-window-ms')?.value],
    ['gate_en',        document.getElementById('cfg-gate-en')?.checked ? 1 : 0],
    ['gate_relay_ms',  document.getElementById('cfg-gate-relay-ms')?.value],
    ['gate_alert_ms',  document.getElementById('cfg-gate-alert-ms')?.value],
    ['awake_timeout_ms',document.getElementById('cfg-awake-ms')?.value],
    ['qr_timeout_ms',  document.getElementById('cfg-qr-ms')?.value],
    ['min_w_g',        document.getElementById('cfg-min-w-g')?.value],
    ['az_max',         document.getElementById('cfg-az-max')?.value],
    ['ina_min',        document.getElementById('cfg-ina-min')?.value],
    ['ina_max',        document.getElementById('cfg-ina-max')?.value],
    ['ina_p2_min',     document.getElementById('cfg-ina-p2-min')?.value],
    ['ina_p2_max',     document.getElementById('cfg-ina-p2-max')?.value],
    ['led_btn_max',    document.getElementById('cfg-led-btn-max')?.value],
    ['led_breathe_ms', document.getElementById('cfg-led-breathe')?.value],
    ['buzzer_en',      document.getElementById('cfg-buzzer-en')?.checked ? 1 : 0],
    ['buzzer_beep_ms', document.getElementById('cfg-buzzer-ms')?.value],
    ['kp_max_fails',   document.getElementById('cfg-kp-max-fails')?.value],
    ['kp_lockout_s',   document.getElementById('cfg-kp-lockout-s')?.value],
  ];
  for (const [key, val] of fields) if (val !== undefined) await sendCmd(key, val);
  this.textContent = '✓ Salvo!';
  setTimeout(() => { this.textContent = 'Salvar Alterações'; this.disabled = false; show('dashboard'); }, 1400);
};

// ── PWA Install Banner ─────────────────────────────
const isStandalone = window.navigator.standalone === true
  || window.matchMedia('(display-mode: standalone)').matches;
let deferredInstallPrompt = null;

window.addEventListener('beforeinstallprompt', (e) => {
  e.preventDefault();
  deferredInstallPrompt = e;
  if (!isStandalone && !localStorage.getItem('pwa-dismissed')) {
    setTimeout(() => {
      const banner = document.getElementById('pwa-banner');
      if (banner) {
        const textEl = banner.querySelector('p:last-of-type');
        if (textEl) textEl.textContent = 'Toque em "Instalar" para adicionar à tela inicial';
        banner.style.display = 'block';
      }
    }, 3000);
  }
});

const bannerBtn = document.querySelector('#pwa-banner button');
if (bannerBtn) {
  bannerBtn.addEventListener('click', () => {
    if (deferredInstallPrompt) {
      deferredInstallPrompt.prompt();
      deferredInstallPrompt.userChoice.then(() => { deferredInstallPrompt = null; });
    }
    document.getElementById('pwa-banner').style.display = 'none';
    localStorage.setItem('pwa-dismissed', '1');
  });
}

const isIOS = /iPad|iPhone|iPod/.test(navigator.userAgent);
if (isIOS && !isStandalone && !localStorage.getItem('pwa-dismissed')) {
  setTimeout(() => { document.getElementById('pwa-banner').style.display = 'block'; }, 3000);
}

// ── Auth ───────────────────────────────────────────
const redirectTs = parseInt(localStorage.getItem(REDIRECT_KEY) || '0');
if (redirectTs && Date.now() - redirectTs > 120000) {
  localStorage.removeItem(REDIRECT_KEY);
  sessionStorage.clear();
}

document.getElementById('btn-login').onclick = function() {
  err('');
  if (isAndroid) {
    localStorage.setItem(REDIRECT_KEY, Date.now().toString());
    signInWithRedirect(auth, provider);
  } else {
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

// ── Boot ───────────────────────────────────────────
(async () => {
  await setPersistence(auth, browserLocalPersistence);

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
  }

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

  setTimeout(() => {
    if (!authResolved) {
      console.warn('[Auth] Firebase timeout — mostrando login');
      show('login');
    }
  }, 8000);

  const p = new URLSearchParams(location.search);
  const tok = p.get('token') || p.get('id');
  if (tok) verifyReceipt(tok);

  window.RLIGHT_BOOTED = true;
})();

// ── Data ───────────────────────────────────────────
async function loadHistory() {
  try {
    const r = await fetch('/api/history');
    deliveries = await r.json();
    const list = document.getElementById('history-list');
    list.innerHTML = '';
    if (!deliveries.length) {
      list.innerHTML = '<p style="color:var(--muted);font-size:14px;text-align:center;margin-top:24px;">Nenhuma entrega registrada</p>';
      return;
    }
    deliveries.forEach((item, i) => {
      const d = document.createElement('div');
      d.className = 'card';
      d.onclick = () => showDetail(i);
      const date = new Date(item.ts).toLocaleDateString('pt-BR',{hour:'2-digit',minute:'2-digit'});
      d.innerHTML = `<div class="card-row"><div><p class="card-title">${item.carrier}</p><p class="card-meta">${date}</p></div><span class="card-val">${item.weight}</span></div>`;
      list.appendChild(d);
    });
  } catch(e) { console.error(e); }
}

function showDetail(i) {
  const item = deliveries[i];
  document.getElementById('det-title').textContent = 'Entrega Validada';
  document.getElementById('det-carrier').textContent = item.carrier;
  document.getElementById('det-weight').textContent = item.weight;
  document.getElementById('det-date').textContent = new Date(item.ts).toLocaleString('pt-BR');
  
  const photoUrl = item.photo_path ? `/photos/${item.id || item.token_uuid}.jpg` : '';
  const photoEl = document.getElementById('det-photo');
  if (photoUrl) {
    photoEl.src = photoUrl;
    photoEl.style.display = 'block';
    photoEl.onerror = () => { photoEl.style.display = 'none'; };
  } else {
    photoEl.style.display = 'none';
  }
  show('details');
}

async function verifyReceipt(jwt) {
  show('details');
  document.getElementById('det-title').textContent = 'Validando…';
  try {
    const r = await fetch('/api/verify',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({jwt})});
    const data = await r.json();
    if (data.valid) {
      document.getElementById('det-title').textContent = 'Entrega Validada';
      document.getElementById('det-carrier').textContent = data.data.carrier;
      document.getElementById('det-weight').textContent = (data.data.weight/1000).toFixed(2)+' kg';
      document.getElementById('det-date').textContent = new Date(data.data.ts*1000).toLocaleString('pt-BR');
    } else {
      document.getElementById('det-title').textContent = 'Assinatura Inválida';
    }
  } catch(e) { document.getElementById('det-title').textContent = 'Erro de conexão'; }
}
