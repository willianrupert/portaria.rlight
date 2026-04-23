const express = require('express');
const crypto = require('crypto');
const path = require('path');

const app = express();
const PORT = process.env.PORT || 3005;

// MESMO SECRET DO ESP32 (JwtSigner.cpp)
const JWT_SECRET = process.env.JWT_SECRET || 'rlight_v6_base_secret';

app.use(express.static(path.join(__dirname, 'public')));
app.use(express.json());

// Rota de Login (Simples)
app.post('/api/login', (req, res) => {
    const { username, password } = req.body;
    // Autenticação mock para o painel
    if (username === 'admin' && password === 'rlight123') {
        res.json({ success: true, token: 'admin_session_token' });
    } else {
        res.status(401).json({ success: false, message: 'Credenciais inválidas' });
    }
});

// Rota de Historico (Mock para o Dashboard)
app.get('/api/history', (req, res) => {
    res.json([
        { id: 'RL-9281', carrier: 'Mercado Livre', weight: '1240g', ts: Date.now() - 3600000 },
        { id: 'RL-1022', carrier: 'Amazon', weight: '450g', ts: Date.now() - 86400000 },
        { id: 'RL-0054', carrier: 'Correios', weight: '2100g', ts: Date.now() - 172800000 }
    ]);
});

// Validador de JWT Criptográfico vindo do ESP32
app.post('/api/verify', (req, res) => {
    const { jwt } = req.body;
    if (!jwt) return res.status(400).json({ valid: false, error: 'Token ausente' });

    try {
        const parts = jwt.split('.');
        if (parts.length !== 3) throw new Error('Formato JWT inválido');

        const [headerB64, payloadB64, signatureB64] = parts;
        
        // Recalcular Assinatura (HMAC-SHA256)
        const sigData = `${headerB64}.${payloadB64}`;
        const hmac = crypto.createHmac('sha256', JWT_SECRET);
        hmac.update(sigData);
        
        // Converter o digest para base64url para bater com o ESP32
        let calcSig = hmac.digest('base64');
        calcSig = calcSig.replace(/\+/g, '-').replace(/\//g, '_').replace(/=+$/, '');

        if (calcSig !== signatureB64) {
            return res.json({ valid: false, error: 'Assinatura inválida (Possível fraude)' });
        }

        // Token Válido! Decodificar Payload
        const payloadStr = Buffer.from(payloadB64, 'base64url').toString('utf-8');
        const payloadObj = JSON.parse(payloadStr);

        res.json({ valid: true, data: payloadObj });

    } catch (e) {
        res.json({ valid: false, error: e.message });
    }
});

app.listen(PORT, () => {
    console.log(`[RLight Cloud] Servidor rodando na porta ${PORT}`);
});
