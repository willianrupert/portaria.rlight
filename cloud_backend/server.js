require('dotenv').config();
const express = require('express');
const crypto  = require('crypto');
const path    = require('path');
const { createProxyMiddleware } = require('http-proxy-middleware');

const Database = require('better-sqlite3');
const multer   = require('multer');
const fs       = require('fs');

const app  = express();
const PORT = process.env.PORT || 3005;
const OPI_HOST = process.env.OPI_HOST || 'http://192.168.1.100:8080';
const DB_PATH  = process.env.DB_PATH || path.join(__dirname, 'deliveries.db');
const UPLOADS_DIR = path.join(__dirname, 'public', 'photos');

// Inicializa DB se não existir
if (!fs.existsSync(UPLOADS_DIR)) fs.mkdirSync(UPLOADS_DIR, { recursive: true });
const db = new Database(DB_PATH);
db.exec(`
    CREATE TABLE IF NOT EXISTS deliveries (
        token_uuid TEXT PRIMARY KEY,
        ts INTEGER,
        carrier TEXT,
        weight_g INTEGER,
        photo_path TEXT,
        jwt TEXT
    )
`);

const storage = multer.diskStorage({
    destination: (req, file, cb) => cb(null, UPLOADS_DIR),
    filename: (req, file, cb) => cb(null, file.originalname)
});
const upload = multer({ storage });

const JWT_SECRET = process.env.JWT_SECRET;
const JWT_PREV_SECRET = process.env.JWT_PREV_SECRET;

if (!JWT_SECRET) {
  console.error('[FATAL] JWT_SECRET não definido. Defina no .env ou variável de ambiente.');
  process.exit(1);
}

// ── Firebase Auth Proxy ───────────────────────────────────────────────────
app.use('/__/', createProxyMiddleware({
    target: 'https://portaria-rlight.firebaseapp.com',
    changeOrigin: true,
    logLevel: 'silent'
}));

// ── Static + JSON ─────────────────────────────────────────────────────────
app.use(express.static(path.join(__dirname, 'public')));
app.use(express.json());

// ── API: Sync (Recebe do OPi) ─────────────────────────────────────────────
app.post('/api/sync', upload.single('image'), (req, res) => {
    const { delivery_token, timestamp, weight, carrier } = req.body;
    const token_uuid = req.file ? req.file.originalname.replace('.jpg', '') : `rec-${Date.now()}`;
    
    try {
        const stmt = db.prepare(`
            INSERT OR REPLACE INTO deliveries (token_uuid, ts, carrier, weight_g, photo_path, jwt)
            VALUES (?, ?, ?, ?, ?, ?)
        `);
        stmt.run(token_uuid, parseInt(timestamp), carrier, parseInt(weight), `/photos/${token_uuid}.jpg`, delivery_token);
        res.json({ ok: true });
    } catch (e) {
        res.status(500).json({ ok: false, error: e.message });
    }
});

// ── API: History (Lê do DB local) ─────────────────────────────────────────
app.get('/api/history', (req, res) => {
    try {
        const rows = db.prepare('SELECT * FROM deliveries ORDER BY ts DESC LIMIT 50').all();
        res.json(rows.map(r => ({
            id: r.token_uuid,
            carrier: r.carrier,
            weight: r.weight_g + 'g',
            ts: r.ts * 1000,
            photo_path: r.photo_path
        })));
    } catch (e) {
        res.status(500).json({ ok: false, error: e.message });
    }
});

// ── Photos Proxy (Mantenha se quiser ainda buscar do OPi, mas agora servimos local também via express.static em /public)
app.use('/photos/', express.static(UPLOADS_DIR));

// ── API: Door Status Proxy ────────────────────────────────────────────────
app.use('/api/doors', createProxyMiddleware({
    target: OPI_HOST,
    changeOrigin: true,
    pathRewrite: { '^/api/doors': '/doors' }
}));

// ── API: Actuate (Proxy with Fetch or Middleware) ─────────────────────────
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

// ── API: Settings Proxy ───────────────────────────────────────────────────
app.post('/api/settings', async (req, res) => {
    try {
        const r = await fetch(`${OPI_HOST}/settings`, {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify(req.body)
        });
        const data = await r.json();
        res.json(data);
    } catch(e) {
        res.status(503).json({ ok: false, error: 'OPi inacessível' });
    }
});

// ── API: Verify JWT ───────────────────────────────────────────────────────
app.post('/api/verify', (req, res) => {
    const { jwt } = req.body;
    if (!jwt) return res.status(400).json({ valid: false, error: 'Token ausente' });
    
    try {
        const parts = jwt.split('.');
        if (parts.length !== 3) throw new Error('Formato JWT inválido');
        const [h, p, sig] = parts;
        
        const verify = (secret) => {
            if (!secret) return false;
            const hmac = crypto.createHmac('sha256', secret);
            hmac.update(`${h}.${p}`);
            return hmac.digest('base64')
                .replace(/\+/g,'-').replace(/\//g,'_').replace(/=+$/,'') === sig;
        };

        if (verify(JWT_SECRET) || (JWT_PREV_SECRET && verify(JWT_PREV_SECRET))) {
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

app.listen(PORT, () => console.log(`[RLight Cloud] :${PORT}`));
