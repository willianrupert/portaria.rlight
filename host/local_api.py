import os
import sqlite3
import json
import time
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.staticfiles import StaticFiles
from ui.websocket_manager import ws_manager
from core.serial_bridge import esp32_bridge
from core.machine_state import host_fsm

app = FastAPI(title="RLight API Local")

DB_PATH = "/var/lib/rlight/deliveries.db"
UI_DIR = os.path.join(os.path.dirname(__file__), 'ui', 'web')

# Garantir que o diretório de fotos exista para não quebrar o mount
os.makedirs("/tmp/rlight_photos", exist_ok=True)

@app.get("/entregas")
def list_deliveries(limit: int = 10):
    try:
        conn = sqlite3.connect(DB_PATH)
        rows = conn.execute(
            "SELECT token_uuid, ts, carrier, weight_g, photo_path FROM deliveries ORDER BY ts DESC LIMIT ?", 
            (limit,)
        ).fetchall()
        return [{"id": r[0], "ts": r[1]*1000, "carrier": r[2], "weight": f"{r[3]}g", "photo_path": r[4]} for r in rows]
    except Exception as e:
        return {"error": str(e)}

@app.get("/doors")
def get_door_state():
    return {
        "p1":   {"reed_open": host_fsm.get_p1_open(),   "strike_active": False},
        "p2":   {"reed_open": host_fsm.get_p2_open(),   "strike_active": False},
        "gate": {"reed_open": host_fsm.get_gate_open(), "relay_active":  False},
        "ts":   int(time.time() * 1000),
        "age_ms": 0
    }

from pydantic import BaseModel
from fastapi import HTTPException

class ActuateRequest(BaseModel):
    device: str

class SettingsRequest(BaseModel):
    key: str | None = None
    val: str | int | float | bool | None = None

@app.post("/actuate")
def actuate_device(body: ActuateRequest):
    cmd_map = {"p1": "CMD_OPEN_P1", "p2": "CMD_OPEN_P2", "gate": "CMD_OPEN_GATE"}
    if body.device not in cmd_map:
        raise HTTPException(status_code=400, detail="Dispositivo inválido")
    esp32_bridge.send_cmd(cmd_map[body.device])
    return {"ok": True}

@app.post("/settings")
def update_settings(body: dict):
    # Mantemos suporte a dict dinâmico mas poderíamos usar SettingsRequest se o cloud enviar um por um
    if "key" in body and "val" in body:
        esp32_bridge.send_cmd("CMD_UPDATE_CFG", {body["key"]: body["val"]})
    else:
        for key, val in body.items():
            esp32_bridge.send_cmd("CMD_UPDATE_CFG", {key: val})
    return {"ok": True}

@app.websocket("/ui/stream")
async def websocket_endpoint(websocket: WebSocket):
    await ws_manager.connect(websocket)
    # Envia o estado atual imediatamente após conectar para sincronizar a UI
    await websocket.send_json({
        "type": "state_change",
        "state": host_fsm.get_state(),
        "jwt": host_fsm.get_jwt(),
        "weight_g": host_fsm.get_weight(),
        "carrier": host_fsm.get_carrier(),
        "resident_label": host_fsm.get_resident_label()
    })
    try:
        while True:
            data = await websocket.receive_text()
            try:
                msg = json.loads(data)
                if msg.get("action") == "update_param":
                    key = msg.get("key")
                    val = msg.get("value")
                    esp32_bridge.send_cmd("CMD_UPDATE_CFG", {key: val})
                elif msg.get("action") == "ring_bell":
                    print("[API] Campainha acionada via UI")
            except json.JSONDecodeError:
                pass
    except WebSocketDisconnect:
        ws_manager.disconnect(websocket)

# Monta o diretório temporário de fotos e interface visual
app.mount("/photos", StaticFiles(directory="/tmp/rlight_photos"), name="photos")
app.mount("/ui", StaticFiles(directory=UI_DIR, html=True), name="ui")

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8080)
