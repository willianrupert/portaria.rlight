import os
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse
from websocket_manager import ws_manager

app = FastAPI(title="RLight v7 Local API")

# Caminho para os arquivos estáticos da UI
UI_PATH = os.path.join(os.path.dirname(__file__), "ui")

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await ws_manager.connect(websocket)
    try:
        while True:
            # Mantém a conexão aberta e aguarda mensagens (opcional)
            data = await websocket.receive_text()
            # Se a UI enviar comandos via WS, tratar aqui
            print(f"[WS] Recebido: {data}")
    except WebSocketDisconnect:
        ws_manager.disconnect(websocket)

@app.get("/status")
async def get_status():
    from core.machine_state import host_fsm
    return {
        "state": host_fsm.get_state(),
        "weight": host_fsm.get_weight(),
        "jwt": host_fsm.get_jwt()
    }

# Rota para desbloqueio manual via API (HA/UI)
@app.post("/unlock/p1")
async def unlock_p1():
    from core.serial_bridge import esp32_bridge
    esp32_bridge.send_cmd("UNLOCK_P1")
    return {"status": "cmd_sent"}

# Servir o SPA (deve ser a última rota ou usar StaticFiles)
if os.path.exists(UI_PATH):
    app.mount("/ui", StaticFiles(directory=UI_PATH, html=True), name="ui")
    
    @app.get("/")
    async def read_index():
        return FileResponse(os.path.join(UI_PATH, "index.html"))
