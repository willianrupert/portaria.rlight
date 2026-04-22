import serial
import json
import threading
import time
from .machine_state import host_fsm
from .config import config

class SerialBridge:
    def __init__(self):
        self.ser = None
        self.running = False
        self.thread = None
        
    def start(self):
        self.running = True
        self.thread = threading.Thread(target=self._read_loop, daemon=True)
        self.thread.start()

    def _connect(self):
        while self.running:
            try:
                if self.ser is None or not self.ser.is_open:
                    print(f"[Serial] Conectando a {config.ESP32_PORT} a {config.ESP32_BAUDRATE} bps...")
                    self.ser = serial.Serial(config.ESP32_PORT, config.ESP32_BAUDRATE, timeout=1)
                    print("[Serial] Ponte USB CDC conectada!")
                return True
            except serial.SerialException:
                print(f"[Serial] Falha na ponte. Retentando em 2 segundos.")
                time.sleep(2)

    def _read_loop(self):
        while self.running:
            if not self._connect():
                continue
                
            try:
                line = self.ser.readline()
                if line:
                    decoded = line.decode('utf-8').strip()
                    if not decoded:
                        continue
                        
                    # Parse Json vindo do S3
                    try:
                        payload = json.loads(decoded)
                        host_fsm.update_from_payload(payload)
                    except json.JSONDecodeError:
                        print(f"[Serial Lixo] {decoded}")
                        
            except serial.SerialException:
                print("[Serial] Desconexão física detectada!")
                self.ser.close()
                self.ser = None
                time.sleep(1)
            except Exception as e:
                print(f"[Serial Loop Fatal] {e}")

    def send_cmd(self, cmd_name: str, args: dict = None):
        """Envia comandos para o ESP32 (`AWAKE`, `CMD_SYNC_TIME`, `CMD_UNLOCK_P2`)"""
        if self.ser and self.ser.is_open:
            payload = {"cmd": cmd_name}
            if args:
                payload.update(args)
            
            raw = json.dumps(payload) + "\n"
            try:
                self.ser.write(raw.encode('utf-8'))
                self.ser.flush()
                print(f"[Serial TX] -> {raw.strip()}")
            except Exception as e:
                print(f"[Serial TX Error] {e}")

    def stop(self):
        self.running = False
        if self.ser and self.ser.is_open:
            self.ser.close()

# Singleton do bridge serial
esp32_bridge = SerialBridge()
