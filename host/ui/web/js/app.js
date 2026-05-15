    // ─── STATE ────────────────────────────────────────────────
    let currentState = 'IDLE';
    let awakeInterval = null;
    let awakeSeconds = 30;
    let doorSeconds = 0;
    let doorInterval = null;
    let confirmTimer = null;
    let p2Interval = null;
    let fallbackTimer = null;
    let fallbackVisible = false;

    // fix #2 — online state simulation (toggle with 'o' key in demo)
    let isOnline = true;

    const stateLabels = {
      IDLE: '',
      AWAKE: 'Aguardando Leitura',
      WAITING_QR: 'Aguardando Leitura',
      AUTHORIZED: 'Acesso Liberado',
      INSIDE_WAIT: 'Dentro do Corredor',
      DELIVERING: 'Depósito em Curso',
      DOOR_REOPENED: 'Aguardando Saída',
      CONFIRMING: 'Gerando Comprovante',
      RECEIPT: 'Comprovante',
      ABORTED: 'Ciclo Encerrado',
      WAITING_PASS: 'Aguardando Senha',
      LOCKOUT_KEYPAD: 'Teclado Bloqueado',
      RESIDENT_P1: 'Morador — P1',
      RESIDENT_P2: 'Morador — P2',
      REVERSE_PICKUP: 'Coleta Reversa',
      DOOR_ALERT: 'Alerta de Segurança',
      MAINTENANCE: 'Manutenção',
      ERROR: 'Falha Crítica',
    };

    // ─── CLOCK ────────────────────────────────────────────────
    const days = ['DOM', 'SEG', 'TER', 'QUA', 'QUI', 'SEX', 'SÁB'];
    const months = ['JAN', 'FEV', 'MAR', 'ABR', 'MAI', 'JUN', 'JUL', 'AGO', 'SET', 'OUT', 'NOV', 'DEZ'];
    function pad(n) { return String(n).padStart(2, '0') }

    function updateClock() {
      const now = new Date();
      const h = pad(now.getHours()), m = pad(now.getMinutes()), s = pad(now.getSeconds());
      document.getElementById('clk-hm').textContent = h + ':' + m;
      document.getElementById('clk-s').textContent = s;
      const dstr = days[now.getDay()] + ', ' + pad(now.getDate()) + ' ' + months[now.getMonth()];
      document.getElementById('clk-date').textContent = dstr;
      document.getElementById('idle-hm').textContent = h + ':' + m;
      document.getElementById('idle-date').textContent = dstr;
    }
    setInterval(updateClock, 1000);
    updateClock();

    // ─── BLOB COLORS ──────────────────────────────────────────
    const blobConfigs = {
      IDLE: { b1: '#30D158', b2: '#0A84FF', op1: 0.05, op2: 0.05 },
      AWAKE: { b1: '#FFD60A', b2: '#30D158', op1: 0.18, op2: 0.14 },
      WAITING_QR: { b1: '#FFD60A', b2: '#30D158', op1: 0.20, op2: 0.12 },
      AUTHORIZED: { b1: '#30D158', b2: '#30D158', op1: 0.28, op2: 0.18 },
      INSIDE_WAIT: { b1: '#30D158', b2: '#0A84FF', op1: 0.18, op2: 0.14 },
      DELIVERING: { b1: '#30D158', b2: '#FFD60A', op1: 0.18, op2: 0.14 },
      DOOR_REOPENED: { b1: '#FFD60A', b2: '#0A84FF', op1: 0.14, op2: 0.12 },
      CONFIRMING: { b1: '#0A84FF', b2: '#30D158', op1: 0.16, op2: 0.14 },
      RECEIPT: { b1: '#0A84FF', b2: '#30D158', op1: 0.18, op2: 0.14 },
      ABORTED: { b1: '#636366', b2: '#636366', op1: 0.12, op2: 0.10 },
      RESIDENT_P1: { b1: '#30D158', b2: '#0A84FF', op1: 0.22, op2: 0.16 },
      RESIDENT_P2: { b1: '#0A84FF', b2: '#0A84FF', op1: 0.22, op2: 0.16 },
      REVERSE_PICKUP: { b1: '#BF5AF2', b2: '#0A84FF', op1: 0.18, op2: 0.14 },
      DOOR_ALERT: { b1: '#FF453A', b2: '#FF9F0A', op1: 0.28, op2: 0.22 },
      MAINTENANCE: { b1: '#FFD60A', b2: '#FF9F0A', op1: 0.16, op2: 0.12 },
      ERROR: { b1: '#FF453A', b2: '#8B0000', op1: 0.30, op2: 0.20 },
    };

    function setBlobs(state) {
      // Blobs removed for performance (solid Apple-style background)
    }

    // ─── ONLINE DOT — fix #2 ─────────────────────────────────
    function setOnlineStatus(online) {
      isOnline = online;
      const dot = document.getElementById('online-dot');
      if (online) {
        dot.className = 'online-dot online';
      } else {
        dot.className = 'online-dot offline';
      }
    }

    // ─── TRANSITION ───────────────────────────────────────────
    function goto(state) {
      if (currentState === state && state !== 'AWAKE') return;

      clearInterval(awakeInterval); awakeInterval = null;
      clearInterval(doorInterval); doorInterval = null;
      clearInterval(confirmTimer); confirmTimer = null;
      clearInterval(p2Interval); p2Interval = null;
      clearTimeout(fallbackTimer); fallbackTimer = null;
      document.getElementById('fallback-popup').classList.remove('show');
      fallbackVisible = false;
      document.getElementById('shimmer').classList.remove('active');

      document.querySelectorAll('.screen').forEach(s => s.classList.remove('active'));
      document.querySelectorAll('.nav-btn').forEach(b => b.classList.toggle('active', b.dataset.s === state));

      const hud = document.getElementById('hud');
      if (state === 'IDLE') {
        hud.classList.add('hide');
      } else {
        hud.classList.remove('hide');
        document.getElementById('hud-state').textContent = stateLabels[state] || state;
      }

      setBlobs(state);

      const el = document.getElementById('s-' + state);
      if (el) setTimeout(() => el.classList.add('active'), 50);

      currentState = state;

      switch (state) {
        case 'AWAKE':
        case 'WAITING_QR':
          startAwakeTimer();
          if (state === 'WAITING_QR') {
            document.getElementById('shimmer').classList.add('active');
            fallbackTimer = setTimeout(() => showFallback(), 15000);
          }
          break;
        case 'AUTHORIZED':
          startRadialTimer('auth-radial', 10000);
          break;
        case 'DOOR_ALERT':
          startDoorAlertTimer();
          break;
        case 'CONFIRMING':
          startConfirmingSteps();
          break;
        case 'RESIDENT_P1':
          startRadialTimer('res-p1-radial', 90000);
          break;
        case 'RESIDENT_P2':
          startP2Timer();
          break;
        case 'ERROR':
          const hex = document.querySelector('.err-hex');
          hex.classList.remove('shake');
          void hex.offsetWidth;
          hex.classList.add('shake');
          setTimeout(() => hex.classList.remove('shake'), 700);
          break;
        case 'ABORTED':
          setTimeout(() => goto('IDLE'), 3000);
          break;
      }
    }

    // ─── AWAKE TIMER ──────────────────────────────────────────
    function startAwakeTimer() {
      awakeSeconds = 30;
      const bar = document.getElementById('awake-bar');
      const txt = document.getElementById('awake-timer-txt');
      bar.style.width = '100%';
      awakeInterval = setInterval(() => {
        awakeSeconds--;
        const pct = (awakeSeconds / 30) * 100;
        bar.style.width = pct + '%';
        txt.textContent = 'Aguardando leitura · ' + awakeSeconds + 's';
        if (awakeSeconds <= 0) { clearInterval(awakeInterval); goto('IDLE'); }
      }, 1000);
    }

    // ─── RADIAL TIMER ─────────────────────────────────────────
    function startRadialTimer(id, ms) {
      const el = document.getElementById(id);
      if (!el) return;
      const total = ms / 1000;
      let elapsed = 0;
      el.setAttribute('stroke-dasharray', '100, 100');
      el.setAttribute('stroke-dashoffset', '100'); // start empty
      const iv = setInterval(() => {
        elapsed += 0.1;
        const pct = 100 - Math.min(100, (elapsed / total) * 100);
        el.setAttribute('stroke-dashoffset', pct); // fill toward 0
        if (elapsed >= total) clearInterval(iv);
      }, 100);
    }

    // ─── DOOR ALERT TIMER ─────────────────────────────────────
    function startDoorAlertTimer() {
      doorSeconds = 0;
      const el = document.getElementById('door-timer');
      doorInterval = setInterval(() => {
        doorSeconds++;
        const mm = Math.floor(doorSeconds / 60), ss = doorSeconds % 60;
        el.textContent = 'Aberta há: ' + pad(mm) + ':' + pad(ss);
      }, 1000);
    }

    // ─── CONFIRMING STEPS ─────────────────────────────────────
    function startConfirmingSteps() {
      const steps = ['st1', 'st2', 'st3'];
      steps.forEach(id => {
        const el = document.getElementById(id);
        el.classList.remove('done');
        el.style.opacity = '0';
        el.querySelector('.step-dot').textContent = '○';
      });
      steps.forEach((id, idx) => {
        setTimeout(() => {
          const el = document.getElementById(id);
          el.style.animation = 'none';
          void el.offsetWidth;
          el.style.animation = 'step-in 0.35s ease forwards';
          el.style.opacity = '1';
        }, idx * 400);
        setTimeout(() => {
          const el = document.getElementById(id);
          el.classList.add('done');
          el.querySelector('.step-dot').textContent = '✓';
        }, idx * 400 + 800);
      });
    }

    // ─── P2 TIMER ─────────────────────────────────────────────
    function startP2Timer() {
      const total = 326.73;
      const ms = 2000;
      const el = document.getElementById('p2-radial');
      const lock = document.getElementById('p2-lock');
      const txt = document.getElementById('p2-txt');
      el.style.strokeDashoffset = total;
      lock.innerHTML = '<svg width="64" height="64" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/></svg>';
      txt.textContent = 'Aguarde...';
      txt.style.color = 'var(--t2)';
      let progress = 0;
      const step = 50;
      const inc = total / (ms / step);
      p2Interval = setInterval(() => {
        progress += inc;
        el.style.strokeDashoffset = Math.max(0, total - progress);
        if (progress >= total) {
          clearInterval(p2Interval);
          lock.innerHTML = '<svg width="64" height="64" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"/><path d="M7 11V7a5 5 0 0 1 9.9-1"/></svg>';
          txt.textContent = 'Entrando';
          txt.style.color = 'var(--blue)';
        }
      }, step);
    }

    // ─── FALLBACK ─────────────────────────────────────────────
    function showFallback() {
      document.getElementById('fallback-popup').classList.add('show');
      fallbackVisible = true;
    }

    // ─── NAV ──────────────────────────────────────────────────
    document.querySelectorAll('.nav-btn').forEach(btn => {
      btn.addEventListener('click', () => goto(btn.dataset.s));
    });

    // ─── KEYBOARD ─────────────────────────────────────────────
    const demoStates = ['IDLE', 'AWAKE', 'WAITING_QR', 'AUTHORIZED', 'INSIDE_WAIT', 'DELIVERING',
      'DOOR_REOPENED', 'CONFIRMING', 'RECEIPT', 'ABORTED', 'RESIDENT_P1', 'RESIDENT_P2',
      'REVERSE_PICKUP', 'DOOR_ALERT', 'MAINTENANCE', 'ERROR'];
    let demoIdx = 0;

    document.addEventListener('keydown', e => {
      if (e.key === 'ArrowRight') {
        demoIdx = (demoIdx + 1) % demoStates.length;
        goto(demoStates[demoIdx]);
      } else if (e.key === 'ArrowLeft') {
        demoIdx = (demoIdx - 1 + demoStates.length) % demoStates.length;
        goto(demoStates[demoIdx]);
      } else if (e.key === 'o' || e.key === 'O') {
        // demo: toggle online status
        setOnlineStatus(!isOnline);
      }
    });

    // ─── WEBSOCKET ───────────────────────────────────────────
    let ws = null;
    function connectWS() {
      const proto = location.protocol === 'https:' ? 'wss:' : 'ws:';
      ws = new WebSocket(`${proto}//${location.host}/ui/stream`);
      ws.onopen = () => { setOnlineStatus(true); console.log('[WS] Conectado'); };
      ws.onclose = () => { setOnlineStatus(false); setTimeout(connectWS, 2000); };
      ws.onmessage = (e) => {
        const msg = JSON.parse(e.data);
        if (msg.type === 'state_change') {
          if (msg.carrier) document.getElementById('auth-carrier').textContent = 'via ' + msg.carrier;
          goto(msg.state);
        } else if (msg.type === 'telemetry_update') {
          updateTelemetry(msg);
        } else if (msg.type === 'KEY_PRESSED') {
          handleKeyPress();
        }
      };
    }

    function updateTelemetry(msg) {
      if (currentState === 'INSIDE_WAIT') {
        const w = msg.weight_g || 0;
        document.getElementById('inside-weight').textContent = (w/1000).toFixed(2) + ' kg';
        document.getElementById('inside-wbar').style.width = Math.min(100, (w/2000)*100) + '%';
      }
      if (currentState === 'DELIVERING') {
        document.getElementById('deliv-weight').textContent = ((msg.weight_g || 0)/1000).toFixed(2) + ' kg';
      }
    }

    let keyCount = 0;
    function handleKeyPress() {
      keyCount = (keyCount % 6) + 1;
      const dots = document.querySelectorAll('.pass-dot');
      dots.forEach((d, i) => d.classList.toggle('active', i < keyCount));
      // Reset after a while or on state change
    }

    // ─── AUTO-CYCLE DEMO MODE ─────────────────────────────────
    // Cycles through all screens every 10 seconds for hardware testing
    const demoScreens = ['IDLE', 'AWAKE', 'WAITING_PASS', 'LOCKOUT_KEYPAD',
      'AUTHORIZED', 'INSIDE_WAIT', 'DELIVERING', 'DOOR_REOPENED',
      'CONFIRMING', 'RECEIPT', 'RESIDENT_P1', 'RESIDENT_P2',
      'REVERSE_PICKUP', 'DOOR_ALERT', 'MAINTENANCE', 'ERROR', 'ABORTED'];
    let demoCycleIdx = 0;
    let demoCycleTimer = null;

    function startDemoCycle() {
      if (demoCycleTimer) clearInterval(demoCycleTimer);
      demoCycleIdx = 0;
      goto(demoScreens[0]);
      demoCycleTimer = setInterval(() => {
        demoCycleIdx = (demoCycleIdx + 1) % demoScreens.length;
        goto(demoScreens[demoCycleIdx]);
      }, 10000);
    }

    function stopDemoCycle() {
      if (demoCycleTimer) { clearInterval(demoCycleTimer); demoCycleTimer = null; }
    }

    // ─── INIT ─────────────────────────────────────────────────
    connectWS();
    // Start in demo cycle mode (no sensors connected)
    startDemoCycle();
