import tempfile
import json
import threading
import paho.mqtt.client as mqtt
from core.config import config, dynamic_texts
from core.machine_state import host_fsm
from core.serial_bridge import esp32_bridge

class HomeAssistantIntegration:
    """Implementa o MQTT Discovery e espelha dados para controle bi-direcional da FSM."""
    def __init__(self):
        self.client = mqtt.Client(client_id="rlight_orange_pi")
        self.client.username_pw_set(config.MQTT_USER, config.MQTT_PASS)
        
        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message
        
        self.PREFIX = "homeassistant"
        self.DEVICE_ID = "rlight_portaria_v8"
        
        # Tópicos HA->OPi (Comandos de Ação)
        self.CMD_TOPIC_P2 = f"rlight/{self.DEVICE_ID}/cmd_p2"
        self.CMD_TOPIC_GATE = f"rlight/{self.DEVICE_ID}/cmd_gate"
        self.CMD_TOPIC_RELOAD = f"rlight/{self.DEVICE_ID}/cmd_reload_nvs"
        self.CMD_TOPIC_UNLOCK_RESIDENT = f"rlight/{self.DEVICE_ID}/cmd_unlock_resident"
        self.SET_TOPIC_P2_DELAY = f"rlight/{self.DEVICE_ID}/set_p2_delay"
        
        # Tópicos HA->OPi (Textos Dinâmicos - Setters)
        self.SET_TOPIC_ENDERECO = f"rlight/{self.DEVICE_ID}/set_endereco"
        self.SET_TOPIC_NOMES = f"rlight/{self.DEVICE_ID}/set_nomes"
        self.SET_TOPIC_MSG_ERRO = f"rlight/{self.DEVICE_ID}/set_msg_erro"
        
        # Tópicos OPi->HA (Leituras / Status Real)
        self.STAT_TOPIC_FSM = f"rlight/{self.DEVICE_ID}/fsm_state"
        self.STAT_TOPIC_WEIGHT = f"rlight/{self.DEVICE_ID}/weight_g"
        self.STAT_TOPIC_GATE = f"rlight/{self.DEVICE_ID}/gate_status"
        self.STAT_TOPIC_P1 = f"rlight/{self.DEVICE_ID}/p1_status"
        self.STAT_TOPIC_P2 = f"rlight/{self.DEVICE_ID}/p2_status"
        self.STAT_TOPIC_SCORE = f"rlight/{self.DEVICE_ID}/score"
        self.STAT_TOPIC_LAST_ACCESS = f"rlight/{self.DEVICE_ID}/last_access"
        self.EVT_TOPIC = f"rlight/{self.DEVICE_ID}/events"
        
    def start(self):
        try:
            self.client.connect_async(config.MQTT_BROKER, config.MQTT_PORT, 60)
            self.client.loop_start()
            
            # Assinar o state para publicar alterações
            host_fsm.subscribe(self._on_fsm_change)
        except Exception as e:
            print(f"[MQTT] Falha na conexão Inicial com Broker: {e}")
            
    def _on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            print("[MQTT] Broker Conectado! Publicando configurações Discovery...")
            self._publish_discovery()
            
            # Se inscreve nos topicos de Comando/Texto vindo do H.A.
            client.subscribe(self.CMD_TOPIC_P2)
            client.subscribe(self.CMD_TOPIC_GATE)
            client.subscribe(self.CMD_TOPIC_RELOAD)
            client.subscribe(self.CMD_TOPIC_UNLOCK_RESIDENT)
            client.subscribe(self.SET_TOPIC_ENDERECO)
            client.subscribe(self.SET_TOPIC_NOMES)
            client.subscribe(self.SET_TOPIC_MSG_ERRO)
            client.subscribe(self.SET_TOPIC_P2_DELAY)
        else:
            print(f"[MQTT] Recusado. RC={rc}")
            
    def _on_message(self, client, userdata, msg):
        topic = msg.topic
        payload = msg.payload.decode('utf-8')
        
        if topic == self.CMD_TOPIC_P2 and payload == "PRESS":
            print("[MQTT/HA] Abrir P2 Solicitado!")
            esp32_bridge.send_cmd("CMD_UNLOCK_P2")
            
        elif topic == self.CMD_TOPIC_GATE and payload == "PRESS":
            print("[MQTT/HA] Abrir Portão Solicitado!")
            esp32_bridge.send_cmd("CMD_UNLOCK_GATE")
            
        elif topic == self.CMD_TOPIC_UNLOCK_RESIDENT and payload == "PRESS":
            print("[MQTT/HA] Acesso Morador Remoto Solicitado!")
            # Aciona FSM para transição de acesso seguro
            esp32_bridge.send_cmd("CMD_RESIDENT_UNLOCK")
            
        elif topic == self.SET_TOPIC_ENDERECO:
            dynamic_texts.ENDERECO_TEXT = payload
        elif topic == self.SET_TOPIC_NOMES:
            dynamic_texts.NOMES_RECEBEDORES = payload
        elif topic == self.SET_TOPIC_MSG_ERRO:
            dynamic_texts.MSG_ERRO = payload
        elif topic == self.SET_TOPIC_P2_DELAY:
            try:
                delay = float(payload)
                esp32_bridge.send_cmd("CMD_UPDATE_CFG", {"key": "p2_delay_ms", "val": delay})
            except ValueError:
                pass
            
    def _on_fsm_change(self, new_state, old_state):
        try:
            self.client.publish(self.STAT_TOPIC_FSM, new_state, retain=True)
            weight = host_fsm.get_weight()
            self.client.publish(self.STAT_TOPIC_WEIGHT, f"{weight:.2f}", retain=False)
            
            self.client.publish(self.STAT_TOPIC_GATE, "ON" if host_fsm.get_gate_open() else "OFF", retain=False)
            self.client.publish(self.STAT_TOPIC_P1, "ON" if host_fsm.get_p1_open() else "OFF", retain=False)
            self.client.publish(self.STAT_TOPIC_P2, "ON" if host_fsm.get_p2_open() else "OFF", retain=False)
            self.client.publish(self.STAT_TOPIC_SCORE, str(host_fsm.get_score()), retain=True)
            
            last_access = f"{host_fsm.get_carrier() or 'Morador'}: {host_fsm.get_resident_label() or 'Geral'}"
            self.client.publish(self.STAT_TOPIC_LAST_ACCESS, last_access, retain=True)
        except Exception as e:
            print(f"[MQTT] Erro ao publicar telemetria: {e}")

    def publish_event(self, event_type, data):
        try:
            payload = {"type": event_type, "data": data}
            self.client.publish(self.EVT_TOPIC, json.dumps(payload), retain=False)
        except Exception:
            pass

    def _publish_discovery(self):
        """Injeta dinamicamente Entidades no HA."""
        device_def = {
            "identifiers": [self.DEVICE_ID],
            "name": "Portaria RLight v8",
            "model": "OrangePi Zero 3 + ESP32-S3",
            "manufacturer": "RLight"
        }
        
        # Sensor - Estado
        s_fsm = {"name": "Status da Portaria", "state_topic": self.STAT_TOPIC_FSM, "unique_id": f"{self.DEVICE_ID}_fsm", "icon": "mdi:shield-home", "device": device_def}
        self.client.publish(f"{self.PREFIX}/sensor/{self.DEVICE_ID}/fsm/config", json.dumps(s_fsm), retain=True)
        
        # Sensor - Peso
        s_peso = {"name": "Balança de Ocorrências", "state_topic": self.STAT_TOPIC_WEIGHT, "unique_id": f"{self.DEVICE_ID}_peso", "unit_of_measurement": "g", "device_class": "weight", "device": device_def}
        self.client.publish(f"{self.PREFIX}/sensor/{self.DEVICE_ID}/peso/config", json.dumps(s_peso), retain=True)

        # Binary Sensors - Doors
        s_gate = {"name": "Status Portão", "state_topic": self.STAT_TOPIC_GATE, "unique_id": f"{self.DEVICE_ID}_gate", "device_class": "door", "device": device_def}
        self.client.publish(f"{self.PREFIX}/binary_sensor/{self.DEVICE_ID}/gate/config", json.dumps(s_gate), retain=True)

        s_p1 = {"name": "Status Porta P1", "state_topic": self.STAT_TOPIC_P1, "unique_id": f"{self.DEVICE_ID}_p1", "device_class": "door", "device": device_def}
        self.client.publish(f"{self.PREFIX}/binary_sensor/{self.DEVICE_ID}/p1/config", json.dumps(s_p1), retain=True)

        s_p2 = {"name": "Status Porta P2", "state_topic": self.STAT_TOPIC_P2, "unique_id": f"{self.DEVICE_ID}_p2", "device_class": "door", "device": device_def}
        self.client.publish(f"{self.PREFIX}/binary_sensor/{self.DEVICE_ID}/p2/config", json.dumps(s_p2), retain=True)

        # Sensors - Last Access, Score
        s_last = {"name": "Último Acesso", "state_topic": self.STAT_TOPIC_LAST_ACCESS, "unique_id": f"{self.DEVICE_ID}_last_acc", "icon": "mdi:history", "device": device_def}
        self.client.publish(f"{self.PREFIX}/sensor/{self.DEVICE_ID}/last_access/config", json.dumps(s_last), retain=True)

        s_score = {"name": "Saúde do Sistema", "state_topic": self.STAT_TOPIC_SCORE, "unique_id": f"{self.DEVICE_ID}_score", "unit_of_measurement": "%", "device": device_def}
        self.client.publish(f"{self.PREFIX}/sensor/{self.DEVICE_ID}/score/config", json.dumps(s_score), retain=True)

        # Number - P2 Delay
        n_p2_delay = {
            "name": "Delay P1->P2 (ms)",
            "command_topic": self.SET_TOPIC_P2_DELAY,
            "state_topic": self.SET_TOPIC_P2_DELAY,
            "unique_id": f"{self.DEVICE_ID}_p2_delay",
            "min": 0, "max": 10000, "step": 100,
            "unit_of_measurement": "ms",
            "device": device_def
        }
        self.client.publish(f"{self.PREFIX}/number/{self.DEVICE_ID}/p2_delay/config", json.dumps(n_p2_delay), retain=True)

        # Button - Unlock Gate
        b_gate = {"name": "Abrir Portão Garen", "command_topic": self.CMD_TOPIC_GATE, "payload_press": "PRESS", "unique_id": f"{self.DEVICE_ID}_unlock_gate", "icon": "mdi:gate", "device": device_def}
        self.client.publish(f"{self.PREFIX}/button/{self.DEVICE_ID}/unlock_gate/config", json.dumps(b_gate), retain=True)

        # Button - Unlock Resident
        b_unlock = {"name": "Acesso Morador Remoto", "command_topic": self.CMD_TOPIC_UNLOCK_RESIDENT, "payload_press": "PRESS", "unique_id": f"{self.DEVICE_ID}_unlock_res", "icon": "mdi:door-open", "device": device_def}
        self.client.publish(f"{self.PREFIX}/button/{self.DEVICE_ID}/unlock_res/config", json.dumps(b_unlock), retain=True)
        
        # Text Input - HA->OPi Error
        t_err = {
            "name": "Msg Display Erro (WhatsApp)", 
            "command_topic": self.SET_TOPIC_MSG_ERRO, 
            "state_topic": self.SET_TOPIC_MSG_ERRO, # Fakes a state for the text input
            "unique_id": f"{self.DEVICE_ID}_txt_err", 
            "device": device_def
        }
        self.client.publish(f"{self.PREFIX}/text/{self.DEVICE_ID}/err_msg/config", json.dumps(t_err), retain=True)

ha_integration = HomeAssistantIntegration()
