import time
import uuid
import os
import pygame
from core.config import config
from core.machine_state import host_fsm
from core.serial_bridge import esp32_bridge
from core.db import db_manager
from ui.window import kiosk
from ui.screens import StateScreens
from integrations.homeassistant import ha_integration
from integrations.camera_usb import webcam
from integrations.oracle_api import oracle_api

try:
    import sdnotify
    systemd_notifier = sdnotify.SystemdNotifier()
except ImportError:
    systemd_notifier = None

def on_state_transition(new_state, old_state):
    """
    Hook chamado sempre que a FSM do microcontrolador mudar.
    """
    print(f"[Core FSM] Transição: {old_state} -> {new_state}")
    
    if new_state == "AWAKE":
        esp32_bridge.send_cmd("SCREEN_READY")
        
    elif new_state == "CONFIRMING" and old_state != "CONFIRMING":
        webcam.capture_snapshot()
        
    elif new_state == "RECEIPT":
        # S8: Salvar a foto localmente vinculada a um UUID no filesystem temporário
        raw_surface = webcam.get_last_surface()
        jwt_token = host_fsm.get_jwt()
        weight_g = host_fsm.get_weight()
        
        if jwt_token:
            token_uuid = str(uuid.uuid4())[:8]
            photo_dir = "/tmp/rlight_photos" # folder2ram (Zero-write)
            os.makedirs(photo_dir, exist_ok=True)
            photo_path = os.path.join(photo_dir, f"{token_uuid}.jpg")
            
            if raw_surface:
                pygame.image.save(raw_surface, photo_path)
            else:
                photo_path = "" # Sem foto
                
            print(f"[Sync] Enfileirando entrega {token_uuid} no SQLite local.")
            db_manager.insert_delivery(
                token_uuid=token_uuid,
                ts=int(time.time()),
                jwt=jwt_token,
                weight_g=weight_g,
                carrier="UNKNOWN", 
                photo_path=photo_path
            )

def main():
    print("========================================")
    print(" RLight Portaria Digital v6 - Host OPi  ")
    print(" Padrão de Alta Confiabilidade (Kiosk)  ")
    print("========================================")
    
    if systemd_notifier:
        systemd_notifier.notify("READY=1")
    
    ha_integration.start()
    oracle_api.start_sync_worker() # S7: Inicia thread isolada do banco SQLite
    host_fsm.subscribe(on_state_transition)
    esp32_bridge.start()
    
    running = True
    while running:
        if systemd_notifier:
            systemd_notifier.notify("WATCHDOG=1") # S9: Watchdog
            
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
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
        elif state == "RESIDENT_P1":
            StateScreens.render_resident_p1()
        elif state == "RESIDENT_P2":
            StateScreens.render_resident_p2()
        elif state == "REVERSE_PICKUP":
            StateScreens.render_reverse_pickup()
        elif state in ["DOOR_ALERT", "ERROR"]:
            StateScreens.render_door_alert()
        else:
            StateScreens.render_idle()
            
        kiosk.flip()
        time.sleep(0.05) # Yielding limpo perante o thread pool serial
        
    print("[Shutdown] Desligando módulos...")
    esp32_bridge.stop()
    pygame.quit()

if __name__ == "__main__":
    main()
