import time
import uuid
import os
import base64
import json
import shutil
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

def broadcast_telemetry():
    """Envia os dados físicos vitais (portão, peso) periodicamente para o frontend."""
    ws_manager.broadcast_sync({
        "type": "telemetry_update",
        "p1_open": host_fsm.get_p1_open(),
        "p2_open": host_fsm.get_p2_open(),
        "gate_open": host_fsm.get_gate_open(),
        "int_button": host_fsm.get_int_button_pressed(),
        "weight_g": host_fsm.get_weight(),
        "score": host_fsm.get_score()
    })

def on_state_transition(new_state, old_state):
    """Hook chamado sempre que a FSM do microcontrolador mudar."""
    print(f"[Core FSM] Transição: {old_state} -> {new_state}")
    
    # Gerenciamento de Energia da Tela FÍSICA
    # Em v8, fluxo de senha (morador) não deve acender a tela por segurança e elegância.
    if new_state in ["IDLE", "MAINTENANCE", "OUT_OF_HOURS", "WAITING_PASS", "LOCKOUT_KEYPAD", "RESIDENT_P1", "RESIDENT_P2"]:
        display_manager.dpms_control(False)
    else:
        display_manager.dpms_control(True)

    # Lógica de Inicialização Local
    if new_state == "AWAKE":
        esp32_bridge.send_cmd("SCREEN_READY")
    
    # Captura foto no momento da confirmação ou acesso morador
    if new_state in ["CONFIRMING", "RESIDENT_P1", "REVERSE_PICKUP"] and old_state != new_state:
        # Se for morador ou reversa, tentamos capturar a foto imediatamente
        webcam.capture_snapshot("temp_last")
        
    photo_url = ""
    jwt_token = host_fsm.get_jwt()

    if new_state == "RECEIPT":
        weight_g = host_fsm.get_weight()
        
        if jwt_token:
            # v8: Extrai token_uuid real do payload JWT (sem verificação de assinatura no host)
            try:
                parts = jwt_token.split('.')
                if len(parts) >= 2:
                    # Adiciona padding Base64 se necessário
                    payload_raw = parts[1]
                    payload_raw += '=' * (-len(payload_raw) % 4)
                    payload = json.loads(base64.b64decode(payload_raw))
                    token_uuid = payload.get('jti') or payload.get('token') or str(uuid.uuid4())[:8]
                else:
                    token_uuid = str(uuid.uuid4())[:8]
            except Exception as e:
                print(f"[Error] Falha ao decodificar JWT: {e}")
                token_uuid = str(uuid.uuid4())[:8]

            # Renomeia a captura temporária para o nome final
            temp_path = "/tmp/rlight_photos/temp_last.jpg"
            final_path = f"/tmp/rlight_photos/{token_uuid}.jpg"
            if os.path.exists(temp_path):
                shutil.move(temp_path, final_path)
                photo_url = final_path
            else:
                # Fallback caso a captura direta tenha falhado
                photo_url = webcam.capture_snapshot(token_uuid) or ""
                
            print(f"[Sync] Enfileirando entrega {token_uuid} no SQLite local.")
            db_manager.insert_delivery(
                token_uuid=token_uuid,
                ts=int(time.time()),
                jwt=jwt_token,
                weight_g=weight_g,
                carrier=host_fsm.get_carrier() or "UNKNOWN", 
                photo_path=photo_url
            )

    # Envia evento para o Home Assistant se for acesso completo
    if new_state == "IDLE" and old_state == "RESIDENT_P2":
        ha_integration.publish_event("RESIDENT_ACCESS_COMPLETE", {
            "label": host_fsm.get_resident_label(),
            "ts": int(time.time())
        })

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

def on_event(event_name):
    """Hook para eventos assíncronos (não transições)."""
    if event_name == "KEY_PRESSED":
        ws_manager.broadcast_sync({"type": "KEY_PRESSED"})

def main():
    print("========================================")
    print(" RLight Portaria Digital v8 - Host OPi  ")
    print(" Arquitetura Web SPA Kiosk              ")
    print("========================================")
    
    if systemd_notifier:
        systemd_notifier.notify("READY=1")
    
    ha_integration.start()
    oracle_api.start_sync_worker() 
    host_fsm.subscribe(on_state_transition)
    host_fsm.subscribe_event(on_event)
    esp32_bridge.start()
    
    running = True
    tick_count = 0
    last_sync_time = 0
    
    try:
        while running:
            now = time.time()
            if systemd_notifier:
                systemd_notifier.notify("WATCHDOG=1")
            
            # Sincronização de Tempo: Host (OPi + DS3231) -> ESP32
            # Ocorre no boot e a cada 10 minutos (12000 ticks de 0.05s)
            if tick_count == 0 or (now - last_sync_time >= 600):
                esp32_bridge.send_cmd("CMD_SYNC_TIME", {"ts": int(now)})
                last_sync_time = now
                print(f"[Sync] Tempo enviado para o ESP32: {int(now)}")

            # Broadcast periódico de configs para garantir sincronia na UI Web
            if tick_count % 20 == 0:  # 20 * 0.05s = 1 segundo
                broadcast_ui_config()
                
            # Broadcast frequente de telemetria (portão, peso)
            if tick_count % 10 == 0:  # 10 * 0.05s = 0.5 segundo
                broadcast_telemetry()
                
            tick_count += 1
            time.sleep(0.05)
            
    except KeyboardInterrupt:
        print("[Shutdown] Interrompido pelo usuário.")
        
    print("[Shutdown] Desligando módulos...")
    esp32_bridge.stop()

if __name__ == "__main__":
    main()
