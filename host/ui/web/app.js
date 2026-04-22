class UIController {
    constructor() {
        this.currentState = "IDLE";
        this.socket = null;
        this.reconnectTimer = null;
        this.lastHeartbeat = Date.now();
        
        // Timers and intervals
        this.timers = {};
        this.fallbackTimeout = null;
        this.stateEntryTime = Date.now();

        // Config Data
        this.config = {
            timeout_awake: 30000,
            timeout_authorized: 10000,
            timeout_inside: 180000,
            timeout_receipt: 120000,
            timeout_resident_p1: 90000,
            timeout_resident_p2: 2000, // delay configured in core
            timeout_reverse: 180000,
            timeout_door: 90000,
            telephone: "(81) 9999-9999"
        };
        
        // Background Blobs
        this.blob1 = document.getElementById("bg-blob-1");
        this.blob2 = document.getElementById("bg-blob-2");

        // DOM elements cache
        this.elHudState = document.getElementById("ui-hud-state");
        this.elOnlineDot = document.getElementById("ui-online-dot");
        
        // Screens
        this.screens = document.querySelectorAll('.screen');

        this.initClock();
        this.initConnectionMonitor();
        this.initWebSocket();
    }

    initClock() {
        setInterval(() => {
            const now = new Date();
            const h = String(now.getHours()).padStart(2, '0');
            const m = String(now.getMinutes()).padStart(2, '0');
            const s = String(now.getSeconds()).padStart(2, '0');
            
            // Blink colon
            const colon = now.getSeconds() % 2 === 0 ? ':' : '<span style="opacity:0.5">:</span>';
            document.getElementById('ui-clock-hm').innerHTML = `${h}${colon}${m}`;
            document.getElementById('ui-clock-s').innerHTML = `:${s}`;

            const days = ['DOM','SEG','TER','QUA','QUI','SEX','SÁB'];
            const months = ['JAN','FEV','MAR','ABR','MAI','JUN','JUL','AGO','SET','OUT','NOV','DEZ'];
            document.getElementById('ui-date').textContent = `${days[now.getDay()]}, ${now.getDate()} ${months[now.getMonth()]}`;
            
            // Custom door timer tick
            if (this.currentState === "DOOR_ALERT") {
                const diff = Math.floor((Date.now() - this.stateEntryTime) / 1000);
                const dm = String(Math.floor(diff / 60)).padStart(2, '0');
                const ds = String(diff % 60).padStart(2, '0');
                const el = document.getElementById("ui-door-alert-time");
                if(el) el.textContent = `Aberta há: ${dm}:${ds}`;
            }

        }, 1000);
    }

    initConnectionMonitor() {
        setInterval(() => {
            const idleSecs = (Date.now() - this.lastHeartbeat) / 1000;
            this.elOnlineDot.className = 'status-dot'; // reset
            if (idleSecs > 90) {
                this.elOnlineDot.classList.add('offline');
                document.getElementById('ui-health-score').textContent = "Score: 0/100 (Offline)";
            } else if (idleSecs > 15) {
                this.elOnlineDot.classList.add('degraded');
                document.getElementById('ui-health-score').textContent = "Score: 80/100 (Degradado)";
            } else {
                this.elOnlineDot.classList.add('online');
                document.getElementById('ui-health-score').textContent = "Score: 100/100 (Excelente)";
            }
        }, 5000);
    }

    initWebSocket() {
        const protocol = window.location.protocol === "https:" ? "wss:" : "ws:";
        const wsUrl = `${protocol}//${window.location.host}/ui/stream`;
        
        console.log(`[UI] Connecting to ${wsUrl}...`);
        this.socket = new WebSocket(wsUrl);

        this.socket.onopen = () => {
            console.log("[UI] WebSocket Connected!");
            this.lastHeartbeat = Date.now();
            if (this.reconnectTimer) clearInterval(this.reconnectTimer);
        };

        this.socket.onmessage = (event) => {
            this.lastHeartbeat = Date.now();
            try {
                const data = JSON.parse(event.data);
                this.handleMessage(data);
            } catch (e) {
                console.error("[UI] Error parsing message:", e);
            }
        };

        this.socket.onclose = () => {
            console.log("[UI] WebSocket Disconnected. Retrying in 2s...");
            this.reconnectTimer = setTimeout(() => this.initWebSocket(), 2000);
        };
        
        this.socket.onerror = (err) => console.error("[UI] WebSocket Error:", err);
    }

    handleMessage(data) {
        if (data.type === "config_update") {
            if (data.endereco) document.getElementById("ui-endereco").textContent = data.endereco;
            if (data.recebedores) document.getElementById("ui-recebedores").textContent = data.recebedores;
            if (data.msg_erro) document.getElementById("ui-msg-erro").textContent = data.msg_erro;
            if (data.telefone_contato) {
                this.config.telephone = data.telefone_contato;
                document.getElementById("ui-telefone").textContent = data.telefone_contato;
            }
            if (data.msg_door_alert) document.getElementById("ui-msg-door-alert").textContent = data.msg_door_alert;
        } 
        else if (data.type === "state_change") {
            this.updateLiveValues(data);
            this.transitionTo(data.state, data);
        }
    }

    updateLiveValues(data) {
        // Update Weight
        if (data.weight_g !== undefined) {
            let kg = (data.weight_g / 1000).toFixed(2);
            document.getElementById("ui-inside-weight").textContent = `${kg} kg`;
            document.getElementById("ui-delivering-weight").textContent = `${kg} kg`;
            document.getElementById("ui-receipt-weight").textContent = `${kg} kg`;
            
            // If REVERSE, delta is negative
            let isReverse = this.currentState === "REVERSE_PICKUP";
            if (isReverse) {
                document.getElementById("ui-reverse-delta").textContent = `Δ ${kg} kg`;
            }
            
            // Inside progress visual feedback
            let pct = Math.min((data.weight_g / 30000) * 100, 100);
            document.getElementById("ui-inside-weight-bar").style.width = `${pct}%`;
        }
        
        // Update Carrier Name
        if (data.carrier) {
            let label = data.carrier === "UNKNOWN" ? "DESCONHECIDA" : data.carrier;
            const carrierPill = document.getElementById("ui-carrier-pill");
            if(carrierPill) carrierPill.innerHTML = `<span class="caption">via ${label}</span>`;
            
            const authCarrier = document.getElementById("ui-authorized-carrier");
            if(authCarrier) authCarrier.textContent = `via ${label}`;
            
            const reverseCarrier = document.getElementById("ui-reverse-carrier");
            if(reverseCarrier) reverseCarrier.textContent = label;
        }

        // Update Resident Label
        if (data.resident_label) {
            const resName = document.getElementById("ui-resident-name");
            if(resName) resName.textContent = data.resident_label;
        }
    }

    transitionTo(newState, payload = {}) {
        if (this.currentState === newState) return;
        console.log(`[UI] State Transition: ${this.currentState} -> ${newState}`);
        
        this.stateEntryTime = Date.now();
        this.cleanupState(this.currentState);
        
        // Specific UI behaviors before transition
        if (newState === "RECEIPT") this.buildReceipt(payload.jwt, payload.photo_url);
        if (newState === "CONFIRMING") this.runConfirmingAnim();
        
        // Map states to screens if shared
        let targetId = newState;
        if (newState === "WAITING_QR") targetId = "AWAKE";

        // Global HUD adjustments
        const statesDict = {
            "IDLE": "",
            "AWAKE": "Aguardando Leitura",
            "WAITING_QR": "Lendo Código...",
            "AUTHORIZED": "Porta Liberada",
            "INSIDE_WAIT": "Entrega em Curso",
            "DELIVERING": "Pacote Registrado",
            "DOOR_REOPENED": "Saída Rápida",
            "CONFIRMING": "Validando",
            "RECEIPT": "Comprovante",
            "ABORTED": "Cancelado",
            "RESIDENT_P1": "Acesso Morador",
            "RESIDENT_P2": "Acesso Morador P2",
            "REVERSE_PICKUP": "Coleta Reversa",
            "DOOR_ALERT": "⚠ Alerta de Segurança",
            "MAINTENANCE": "Manutenção",
            "ERROR": "Falha"
        };
        this.elHudState.textContent = statesDict[newState] || newState;
        
        // Theme Colors
        if (["ERROR", "DOOR_ALERT"].includes(newState)) {
            this.blob1.style.background = "var(--accent-red)";
            this.blob2.style.background = "var(--accent-orange)";
        } else if (newState === "MAINTENANCE") {
            this.blob1.style.background = "var(--accent-yellow)";
            this.blob2.style.background = "var(--accent-orange)";
        } else {
            this.blob1.style.background = "var(--accent-green)";
            this.blob2.style.background = "var(--accent-blue)";
        }

        // Hide all screens
        this.screens.forEach(el => el.classList.remove('active'));

        // Show target
        const targetScreen = document.getElementById(`screen-${targetId}`);
        if (targetScreen) targetScreen.classList.add('active');

        this.setupStateDecorators(newState);
        this.currentState = newState;
    }

    cleanupState(oldState) {
        // Clear all timers
        Object.values(this.timers).forEach(clearInterval);
        this.timers = {};
        
        if (this.fallbackTimeout) {
            clearTimeout(this.fallbackTimeout);
            this.fallbackTimeout = null;
        }

        // Hide fallback if visible
        document.getElementById("fallback-popup").classList.add("hidden");
    }

    setupStateDecorators(state) {
        let timerMax = 1;
        let pBarId = null;
        let cPathId = null;

        if (state === "AWAKE" || state === "WAITING_QR") {
            timerMax = this.config.timeout_awake;
            pBarId = "ui-awake-progress";
            document.getElementById("ui-awake-timer-text").textContent = "Aguardando leitura · 30s";
            
            // Pop fallback after 15s in WAITING_QR
            if(state === "WAITING_QR") {
                this.fallbackTimeout = setTimeout(() => {
                    document.getElementById("fallback-popup").classList.remove("hidden");
                }, 15000);
            }
        }
        else if (state === "AUTHORIZED") {
            timerMax = this.config.timeout_authorized;
            cPathId = "auth-radial";
        }
        else if (state === "INSIDE_WAIT") {
            timerMax = this.config.timeout_inside;
            pBarId = "ui-inside-timer-bar";
        }
        else if (state === "DELIVERING") {
             // Just keeping the UI clean
        }
        else if (state === "RECEIPT") {
            timerMax = this.config.timeout_receipt;
            pBarId = "ui-receipt-timer-bar";
        }
        else if (state === "RESIDENT_P1") {
            timerMax = this.config.timeout_resident_p1;
            cPathId = "res-p1-radial";
        }
        else if (state === "RESIDENT_P2") {
            timerMax = this.config.timeout_resident_p2;
            cPathId = "res-p2-radial";
            document.getElementById("p2-status-text").textContent = "Aguarde...";
            document.getElementById("p2-lock-icon").textContent = "🔒";
            
            // Auto complete fake visual
            setTimeout(() => {
                document.getElementById("p2-status-text").textContent = "Entrando";
                document.getElementById("p2-lock-icon").textContent = "🔓";
            }, timerMax - 200);
        }
        else if (state === "REVERSE_PICKUP") {
            timerMax = this.config.timeout_reverse;
            pBarId = "ui-reverse-timer-bar";
        }

        // Execute Bar Timers
        if (pBarId) {
            const bar = document.getElementById(pBarId);
            bar.style.transition = "none";
            bar.style.width = "100%";
            setTimeout(() => {
                bar.style.transition = `width ${timerMax}ms linear`;
                bar.style.width = "0%";
            }, 50);
            
            // Tick for text if required
            if (pBarId === "ui-awake-progress") {
                this.timers['awake'] = setInterval(() => {
                    const elapsed = Date.now() - this.stateEntryTime;
                    const remain = Math.max(0, Math.ceil((timerMax - elapsed)/1000));
                    document.getElementById("ui-awake-timer-text").textContent = `Aguardando leitura · ${remain}s`;
                }, 1000);
            }
        }

        // Execute Circular Timers
        if (cPathId) {
            const circle = document.getElementById(cPathId);
            // Circle config
            // stroke-dasharray="100, 100" means 100 is max. For res-p2 it is 339.29
            const fullLen = cPathId === "res-p2-radial" ? 339.29 : 100;
            
            circle.style.transition = "none";
            if(cPathId === "res-p2-radial") {
                circle.style.strokeDashoffset = fullLen; // Empty
                setTimeout(() => {
                    circle.style.transition = `stroke-dashoffset ${timerMax}ms linear`;
                    circle.style.strokeDashoffset = 0; // Fill
                }, 50);
            } else {
                circle.style.strokeDasharray = `${fullLen}, ${fullLen}`; // Full
                setTimeout(() => {
                    circle.style.transition = `stroke-dasharray ${timerMax}ms linear`;
                    circle.style.strokeDasharray = `0, ${fullLen}`; // Empty
                }, 50);
            }
        }
    }

    runConfirmingAnim() {
        const steps = [
            document.getElementById("step-1"),
            document.getElementById("step-2"),
            document.getElementById("step-3")
        ];
        
        steps.forEach(s => {
            s.classList.remove('active');
            s.querySelector('.step-icon').textContent = '○';
        });

        const animNext = (idx) => {
            if (idx >= steps.length) return;
            steps[idx].classList.add('active');
            steps[idx].querySelector('.step-icon').textContent = '●';
            // Wait 0.7s to do the next step (synthetic visual feedback)
            setTimeout(() => animNext(idx+1), 700);
        };
        setTimeout(() => animNext(0), 500);
    }

    buildReceipt(jwtToken, photoUrl) {
        document.getElementById("ui-qrcode-container").innerHTML = "";
        if (jwtToken) {
            const tokenHead = jwtToken.substring(0, 8);
            document.getElementById("ui-receipt-token").textContent = tokenHead;
            
            const urlRecibo = `https://portaria.rlight.com.br/?id=${jwtToken.substring(0, 30)}`;
            const qr = new QRCode({
                content: urlRecibo,
                padding: 0,
                width: 180,
                height: 180,
                color: "#000000",
                background: "#ffffff",
                ecl: "M"
            });
            document.getElementById("ui-qrcode-container").innerHTML = qr.svg();
        }

        const phEl = document.getElementById("ui-photo");
        if (photoUrl) {
            phEl.src = photoUrl + "?t=" + new Date().getTime(); 
            phEl.classList.remove("hidden");
        } else {
            phEl.classList.add("hidden");
        }

        // Set time
        const now = new Date();
        const hm = `${String(now.getHours()).padStart(2, '0')}:${String(now.getMinutes()).padStart(2, '0')}`;
        document.getElementById("ui-receipt-time").textContent = `Hoje, ${hm}`;
    }
}

// Initializer
window.onload = () => {
    window.appUI = new UIController();
    
    // Tapo door bell mockup button interaction
    document.getElementById("btn-interfone").addEventListener("click", () => {
        console.log("SENDING MQTT REQUEST TO RING TAPO");
        // To route this to python, we could send a websocket message
        if(window.appUI.socket && window.appUI.socket.readyState === WebSocket.OPEN) {
            window.appUI.socket.send(JSON.stringify({ action: "ring_bell" }));
        }
    });
};
