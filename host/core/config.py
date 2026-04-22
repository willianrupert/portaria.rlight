import os
import json
from dataclasses import dataclass

@dataclass
class HostConfig:
    # Serial (ESP32)
    ESP32_PORT: str = os.getenv("ESP32_PORT", "/dev/ttyACM0")
    ESP32_BAUDRATE: int = int(os.getenv("ESP32_BAUDRATE", "115200"))
    
    # UI Display
    SCREEN_WIDTH: int = 1024
    SCREEN_HEIGHT: int = 600
    FPS_LIMIT: int = 30
    
    # Camera
    WEBCAM_ID: int = int(os.getenv("WEBCAM_ID", "0")) # Default /dev/video0
    
    # Home Assistant MQTT
    MQTT_BROKER: str = os.getenv("MQTT_BROKER", "192.168.0.100")
    MQTT_PORT: int = int(os.getenv("MQTT_PORT", "1883"))
    MQTT_USER: str = os.getenv("MQTT_USER", "mqtt")
    MQTT_PASS: str = os.getenv("MQTT_PASS", "mqtt_password")
    
    # Oracle Cloud Server
    ORACLE_API_URL: str = os.getenv("ORACLE_API_URL", "https://api.oraclecloud.com/v1/portaria/receipts")
    ORACLE_BEARER: str = os.getenv("ORACLE_BEARER", "YOUR_BEARER_TOKEN")
    
config = HostConfig()

class DynamicTexts:
    """Dados mutáveis 100% In-Memory (Zero gravacao no MicroSD) populados pelo MQTT"""
    def __init__(self):
        self.ENDERECO_TEXT = "Rua Hermes da Fonseca, 315, CEP 54400-333"
        self.NOMES_RECEBEDORES = "Willian Rupert, Maria Aparecida"
        self.TELEFONE_CONTATO = "(81) 9999-9999"
        self.MSG_ERRO = "ERRO DE LEITURA. Toque na campainha abaixo ou chame no WhatsApp (81) 9999-9999"
        self.MSG_DOOR_ALERT = "Se você não está no corredor, acesse o Home Assistant"

dynamic_texts = DynamicTexts()

