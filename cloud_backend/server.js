const express = require('express');
const crypto  = require('crypto');
const path    = require('path');
const { createProxyMiddleware } = require('http-proxy-middleware');

const app  = express();
const PORT = process.env.PORT || 3005;
const JWT_SECRET = process.env.JWT_SECRET || 'rlight_v6_base_secret';

// ── Firebase Auth Proxy (fix iOS Safari ITP) ─────────────────────────────
// Serve /__/auth/* pelo próprio domínio para evitar bloqueio cross-origin
app.use('/__/', createProxyMiddleware({
    target: 'https://portaria-rlight.firebaseapp.com',
    changeOrigin: true,
    logLevel: 'silent'
}));

// ── Static + JSON ─────────────────────────────────────────────────────────
app.use(express.static(path.join(__dirname, 'public')));
app.use(express.json());

// ── Mock door state (será substituído por leitura real do ESP32) ──────────
let doorState = {
    p1:   { reed_open: false, strike_active: false },
    p2:   { reed_open: false, strike_active: false },
    gate: { reed_open: false, relay_active:  false },
    ts: Date.now(),
    age_ms: 0
};

// ── API: Door Status ──────────────────────────────────────────────────────
app.get('/api/doors', (req, res) => {
    doorState.age_ms = Date.now() - doorState.ts;
    res.json(doorState);
});

// ── API: Actuate ──────────────────────────────────────────────────────────
app.post('/api/actuate', (req, res) => {
    const { device } = req.body;
    if (!['p1','p2','gate'].includes(device))
        return res.status(400).json({ ok: false, error: 'Dispositivo inválido' });

    // Simula acionamento (pulso de 500ms)
    const key = device === 'gate' ? 'relay_active' : 'strike_active';
    doorState[device][key] = true;
    doorState.ts = Date.now();
    setTimeout(() => {
        doorState[device][key] = false;
        doorState.ts = Date.now();
    }, 600);

    console.log(`[Actuate] ${device} acionado`);
    res.json({ ok: true });
});

// ── API: History ──────────────────────────────────────────────────────────
app.get('/api/history', (req, res) => {
    res.json([
        { id: 'RL-9281', carrier: 'Mercado Livre', weight: '1240g', ts: Date.now() - 3600000 },
        { id: 'RL-1022', carrier: 'Amazon',        weight: '450g',  ts: Date.now() - 86400000 },
        { id: 'RL-0054', carrier: 'Correios',      weight: '2100g', ts: Date.now() - 172800000 }
    ]);
});

// ── API: Verify JWT ───────────────────────────────────────────────────────
const JWT_PREV_SECRET = process.env.JWT_PREV_SECRET || 'rlight_v6_base_secret_old';

app.post('/api/verify', (req, res) => {
    const { jwt } = req.body;
    if (!jwt) return res.status(400).json({ valid: false, error: 'Token ausente' });
    
    try {
        const parts = jwt.split('.');
        if (parts.length !== 3) throw new Error('Formato JWT inválido');
        const [h, p, sig] = parts;
        
        const verify = (secret) => {
            const hmac = crypto.createHmac('sha256', secret);
            hmac.update(`${h}.${p}`);
            return hmac.digest('base64')
                .replace(/\+/g,'-').replace(/\//g,'_').replace(/=+$/,'') === sig;
        };

        if (verify(JWT_SECRET) || verify(JWT_PREV_SECRET)) {
            const payload = JSON.parse(Buffer.from(p, 'base64url').toString('utf-8'));
            console.log(`[Verify] JWT Válido: ${payload.jti} (${payload.carrier})`);
            return res.json({ valid: true, data: payload });
        } else {
            console.warn(`[Verify] Assinatura Inválida para token`);
            return res.json({ valid: false, error: 'Assinatura inválida' });
        }
    } catch(e) {
        console.error(`[Verify] Erro:`, e.message);
        res.json({ valid: false, error: e.message });
    }
});

// ── API: Settings ─────────────────────────────────────────────────────────
let mockSettings = {
  maintenance_mode: false, always_open: false, op_start_h: 8, op_end_h: 22,
  strike_p1_en: true, door_open_ms: 3000, auth_timeout_ms: 10000,
  strike_p2_en: true, p2_open_ms: 3000, p2_delay_ms: 2000,
  gate_en: true, gate_relay_ms: 500, gate_auto_close: true,
  radar_en: true, radar_dbc: 1500, radar_dist_max: 250,
  min_w_g: 50, az_int: 30, jwt_rotate_en: false, kp_max_fails: 5
};

app.get('/api/settings', (req, res) => {
    res.json(mockSettings);
});

app.post('/api/settings', (req, res) => {
    mockSettings = { ...mockSettings, ...req.body };
    console.log(`[CFG] Updated:`, req.body);
    res.json({ ok: true });
});

app.listen(PORT, () => console.log(`[RLight Cloud] :${PORT}`));
