import threading

class MachineState:
    """
    Observer pattern para espelhar a FSM robusta do ESP32 dentro do Python.
    Permite que múltiplas frentes (UI, Home Assistant) assinem e reajam à mudanças de estado.
    """
    def __init__(self):
        self._state = "IDLE"
        self._last_state = None
        self._weight_realtime = 0.0
        self._jwt_token = None
        self._carrier = ""
        self._resident_label = ""
        self._p1_open = False
        self._p2_open = False
        self._gate_open = False
        self._int_button_pressed = False
        self._score = 100
        
        self._lock = threading.Lock()
        self._listeners = []
        self._event_listeners = []

    def subscribe(self, callback):
        """Registra callback que será chamado em variações de estado."""
        with self._lock:
            self._listeners.append(callback)

    def subscribe_event(self, callback):
        """Registra callback que será chamado para eventos pontuais (ex: KEY_PRESSED)."""
        with self._lock:
            self._event_listeners.append(callback)

    def update_from_payload(self, payload: dict):
        """Atualiza a FSM interna baseada em dados USB e notifica listeners se houve alteração no state principal."""
        changed = False
        with self._lock:
            if "state" in payload:
                new_state = payload["state"]
                if new_state != self._state:
                    self._last_state = self._state
                    self._state = new_state
                    changed = True
            
            if "weight" in payload:
                self._weight_realtime = float(payload["weight"])
                
            if "jwt" in payload:
                self._jwt_token = payload["jwt"]

            if "carrier" in payload:
                self._carrier = payload["carrier"]
                
            if "resident_label" in payload:
                self._resident_label = payload["resident_label"]
                
            if "p1_open" in payload:
                self._p1_open = payload["p1_open"]
            if "p2_open" in payload:
                self._p2_open = payload["p2_open"]
            if "gate_open" in payload:
                self._gate_open = payload["gate_open"]
            if "int_button_pressed" in payload:
                self._int_button_pressed = payload["int_button_pressed"]
            if "score" in payload:
                self._score = payload["score"]

        if changed:
            self._notify_listeners()
        
        if "type" in payload and payload["type"] == "EVENT":
            self._notify_event(payload.get("event", "UNKNOWN"))

    def _notify_listeners(self):
        # Dispara fora do lock principal para não prender a thread Serial
        state = self.get_state()
        last = self.get_last_state()
        for cb in self._listeners:
            try:
                cb(state, last)
            except Exception as e:
                print(f"[FSM Error] Listener exception: {e}")

    def _notify_event(self, event_name):
        for cb in self._event_listeners:
            try:
                cb(event_name)
            except Exception as e:
                print(f"[Event Error] Listener exception: {e}")

    def get_state(self):
        return "AWAKE" # OVERRIDE PARA OBSERVAÇÃO

    def get_last_state(self):
        with self._lock:
            return self._last_state

    def get_weight(self):
        with self._lock:
            return self._weight_realtime
            
    def get_jwt(self):
        with self._lock:
            return self._jwt_token

    def get_carrier(self):
        with self._lock:
            return self._carrier
            
    def get_resident_label(self):
        with self._lock:
            return self._resident_label

    def get_p1_open(self):
        with self._lock:
            return self._p1_open

    def get_p2_open(self):
        with self._lock:
            return self._p2_open

    def get_gate_open(self):
        with self._lock:
            return self._gate_open

    def get_int_button_pressed(self):
        with self._lock:
            return self._int_button_pressed

    def get_score(self):
        with self._lock:
            return self._score

# Instância Singleton global para o host
host_fsm = MachineState()
