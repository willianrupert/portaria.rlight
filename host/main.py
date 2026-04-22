import time
import uuid
import os
from core.config import config, dynamic_texts
from core.machine_state import host_fsm
from core.serial_bridge import esp32_bridge
from core.db import db_manager
from ui.window import display_manager
from ui.websocket_manager import ws_manager
from integrations.homeassistant import ha_integration
from integrations.camera_usb import webcam
from integrations.oracle_api import oracle_api

try:
    import sdnotify
    systemd_notifier = sdnotify.SystemdNotifier()
except ImportError:
    systemd_notifier = None

def broadcast_ui_config():
    """Envia configurações dinâmicas para o frontend."""
    ws_manager.broadcast_sync({
        "type": "config_update",
        "endereco": dynamic_texts.ENDERECO_TEXT,
        "recebedores": dynamic_texts.NOMES_RECEBEDORES,
        "telefone_contato": dynamic_texts.TELEFONE_CONTATO,
        "msg_erro": dynamic_texts.MSG_ERRO,
        "msg_door_alert": dynamic_texts.MSG_DOOR_ALERT
    })

def on_state_transition(new_state, old_state):
    """Hook chamado sempre que a FSM do microcontrolador mudar."""
    print(f"[Core FSM] Transição: {old_state} -> {new_state}")
    
    # Gerenciamento de Energia da Tela FÍSICA
    if new_state in ["IDLE", "MAINTENANCE"]:
        display_manager.dpms_control(False)
    else:
        display_manager.dpms_control(True)

    # Lógica de Inicialização Local
    if new_state == "AWAKE":
        esp32_bridge.send_cmd("SCREEN_READY")
        
    elif new_state == "CONFIRMING" and old_state != "CONFIRMING":
        webcam.capture_snapshot()
        
    photo_url = ""
    jwt_token = host_fsm.get_jwt()

    if new_state == "RECEIPT":
        # S8: Salvar foto e SQLite, retornando a URL pro WebSocket
        raw_surface = webcam.get_last_surface()
        weight_g = host_fsm.get_weight()
        
        if jwt_token:
            token_uuid = str(uuid.uuid4())[:8]
            photo_dir = "/tmp/rlight_photos" # folder2ram (Zero-write)
            photo_path = os.path.join(photo_dir, f"{token_uuid}.jpg")
            photo_url = f"/photos/{token_uuid}.jpg"
            
            if raw_surface:
                import pygame # Importamos pygame localmente apenas se formos gravar arquivo de raw_surface? Mas peraí... raw_surface é pygame.Surface!
                pygame.image.save(raw_surface, photo_path)
            else:
                photo_path = ""
                photo_url = ""
                
            print(f"[Sync] Enfileirando entrega {token_uuid} no SQLite local.")
            db_manager.insert_delivery(
                token_uuid=token_uuid,
                ts=int(time.time()),
                jwt=jwt_token,
                weight_g=weight_g,
                carrier="UNKNOWN", 
                photo_path=photo_path
            )

    # Envia evento de mudança de estado para o navegador Kiosk
    ws_manager.broadcast_sync({
        "type": "state_change",
        "state": new_state,
        "jwt": jwt_token,
        "photo_url": photo_url,
        "weight_g": host_fsm.get_weight(),
        "carrier": host_fsm.get_carrier(),
        "resident_label": host_fsm.get_resident_label()
    })

def main():
    print("========================================")
    print(" RLight Portaria Digital v7 - Host OPi  ")
    print(" Arquitetura Web SPA Kiosk              ")
    print("========================================")
    
    if systemd_notifier:
        systemd_notifier.notify("READY=1")
    
    ha_integration.start()
    oracle_api.start_sync_worker() 
    host_fsm.subscribe(on_state_transition)
    esp32_bridge.start()
    
    running = True
    tick_count = 0
    try:
        while running:
            if systemd_notifier:
                systemd_notifier.notify("WATCHDOG=1")
            
            # Broadcast periódico de configs para garantir sincronia na UI Web
            if tick_count % 20 == 0:  # 20 * 0.05s = 1 segundo
                broadcast_ui_config()
                
            tick_count += 1
            time.sleep(0.05)
            
    except KeyboardInterrupt:
        print("[Shutdown] Interrompido pelo usuário.")
        
    print("[Shutdown] Desligando módulos...")
    esp32_bridge.stop()

if __name__ == "__main__":
    main()
