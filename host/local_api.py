import os
import sqlite3
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.staticfiles import StaticFiles
from ui.websocket_manager import ws_manager

app = FastAPI(title="RLight API Local")

DB_PATH = os.path.join(os.path.dirname(__file__), 'deliveries.db')
UI_DIR = os.path.join(os.path.dirname(__file__), 'ui', 'web')

# Garantir que o diretório de fotos exista para não quebrar o mount
os.makedirs("/tmp/rlight_photos", exist_ok=True)

@app.get("/entregas")
def list_deliveries(limit: int = 10):
    try:
        conn = sqlite3.connect(DB_PATH)
        rows = conn.execute(
            "SELECT ts, carrier, weight_g, synced FROM deliveries ORDER BY ts DESC LIMIT ?", 
            (limit,)
        ).fetchall()
        return [{"ts": r[0], "carrier": r[1], "peso_g": r[2], "sync": bool(r[3])} for r in rows]
    except Exception as e:
        return {"error": str(e)}

@app.websocket("/ui/stream")
async def websocket_endpoint(websocket: WebSocket):
    await ws_manager.connect(websocket)
    try:
        while True:
            # Mantém vivo
            data = await websocket.receive_text()
    except WebSocketDisconnect:
        ws_manager.disconnect(websocket)

# Monta o diretório temporário de fotos e interface visual
app.mount("/photos", StaticFiles(directory="/tmp/rlight_photos"), name="photos")
app.mount("/ui", StaticFiles(directory=UI_DIR, html=True), name="ui")

if __name__ == "__main__":
    import uvicorn
    # Acessível via http://IP_DA_ORANGE_PI:8080/entregas ou /ui/index.html
    uvicorn.run(app, host="0.0.0.0", port=8080)
