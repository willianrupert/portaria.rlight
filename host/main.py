import time
import pygame
from core.config import config
from core.machine_state import host_fsm
from core.serial_bridge import esp32_bridge
from ui.window import kiosk
from ui.screens import StateScreens
from integrations.homeassistant import ha_integration
from integrations.camera_usb import webcam
from integrations.oracle_api import oracle_api

def on_state_transition(new_state, old_state):
    """
    Hook chamado sempre que a FSM do microcontrolador mudar.
    """
    print(f"[Core FSM] Transição: {old_state} -> {new_state}")
    
    # 1. Gatilhos de Tela e Câmera
    if new_state == "AWAKE":
        # Avisa o ESP32 que a UI host está pronta pra focar no QR
        esp32_bridge.send_cmd("SCREEN_READY")
        
    elif new_state == "CONFIRMING" and old_state != "CONFIRMING":
        # Momento Crítico. ESP32 diz que estabilizou e preparou assinatura.
        # Nós tiramos a foto 1080p na memória em background.
        webcam.capture_snapshot()
        
    elif new_state == "RECEIPT":
        # A API Oracle receberá o JWT fornecido pelo ESP32 e a Foto capturada antes.
        # Fire-and-forget background upload
        raw_img = webcam.last_frame
        jwt_token = host_fsm.get_jwt()
        if raw_img and jwt_token:
            oracle_api.push_receipt_async(jwt_token, raw_img)


def main():
    print("========================================")
    print(" RLight Portaria Digital v6 - Host OPi  ")
    print(" Padrão de Alta Confiabilidade (Kiosk)  ")
    print("========================================")
    
    # Inicializa Home Assistant
    ha_integration.start()
    
    # Prepara listeners 
    host_fsm.subscribe(on_state_transition)
    
    # Inicia a ponte USB com a S3
    esp32_bridge.start()
    
    running = True
    
    # Main Loop Gráfico
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    # Escape sai apenas durante debug via teclado USB
                    running = False

        state = host_fsm.get_state()
        
        # UI Mapper Baseado no Espelho FSM
        if state == "IDLE":
            StateScreens.render_idle()
        elif state == "AWAKE":
            StateScreens.render_awake()
        elif state == "WAITING_QR":
            StateScreens.render_waiting_qr()
        elif state in ["AUTHORIZED", "INSIDE_WAIT"]:
            StateScreens.render_authorized()
        elif state == "DELIVERING" or state == "DOOR_REOPENED":
            StateScreens.render_delivering()
        elif state == "CONFIRMING":
            StateScreens.render_confirming()
        elif state == "RECEIPT":
            StateScreens.render_receipt(host_fsm.get_jwt(), webcam.get_last_surface())
        elif state in ["DOOR_ALERT", "ERROR"]:
            StateScreens.render_door_alert()
        else:
            StateScreens.render_idle()
            
        kiosk.flip()
        
    print("[Shutdown] Desligando módulos...")
    esp32_bridge.stop()
    pygame.quit()

if __name__ == "__main__":
    main()
