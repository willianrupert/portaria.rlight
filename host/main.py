import time
import asyncio
import uvicorn
import os
from core.machine_state import host_fsm
from core.serial_bridge import esp32_bridge
from local_api import app
from websocket_manager import ws_manager

def set_display_power(on: bool):
    """Controla o backlight da tela via sysfs."""
    val = "0" if on else "1"
    print(f"[Hardware] Display Power: {'ON' if on else 'OFF'}")
    os.system(f"echo {val} | sudo tee /sys/class/backlight/*/bl_power > /dev/null 2>&1")

# Armazenamos o loop principal para referência externa
main_loop = None

def on_state_transition(new_state, old_state):
    """Hook reativo disparado pela thread Serial."""
    print(f"[Core FSM] Transição: {old_state} -> {new_state}")
    
    payload = {
        "event": "STATE_TRANSITION",
        "state": new_state,
        "old_state": old_state,
        "ts": int(time.time()),
        "weight": host_fsm.get_weight(),
        "jwt": host_fsm.get_jwt()
    }
    
    if main_loop and main_loop.is_running():
        # Agenda o broadcast no loop do FastAPI
        main_loop.call_soon_threadsafe(
            lambda: asyncio.create_task(ws_manager.broadcast(payload))
        )
    
    if new_state == "IDLE":
        set_display_power(False)
    elif old_state == "IDLE":
        set_display_power(True)
    
    if new_state == "AWAKE":
        esp32_bridge.send_cmd("SCREEN_READY")

async def run_server():
    global main_loop
    main_loop = asyncio.get_running_loop()
    
    config = uvicorn.Config(app, host="0.0.0.0", port=8080, log_level="info")
    server = uvicorn.Server(config)
    await server.serve()

def main():
    print("========================================")
    print(" RLight Portaria Digital v7 - Web Host  ")
    print("========================================")

    set_display_power(True)
    host_fsm.subscribe(on_state_transition)
    esp32_bridge.start()

    try:
        asyncio.run(run_server())
    except KeyboardInterrupt:
        print("[Shutdown] Encerrando...")
    finally:
        esp32_bridge.stop()

if __name__ == "__main__":
    main()
