import asyncio
import json
from typing import List
from fastapi import WebSocket

class ConnectionManager:
    """
    Gerencia as conexões WebSocket ativas (UI Kiosk).
    Permite broadcast de estados da FSM em tempo real.
    """
    def __init__(self):
        self.active_connections: List[WebSocket] = []

    async def connect(self, websocket: WebSocket):
        await websocket.accept()
        self.active_connections.append(websocket)
        print(f"[WS] Nova conexão estabelecida. Total: {len(self.active_connections)}")

    def disconnect(self, websocket: WebSocket):
        if websocket in self.active_connections:
            self.active_connections.remove(websocket)
            print(f"[WS] Conexão encerrada. Total: {len(self.active_connections)}")

    async def broadcast(self, message: dict):
        """Envia mensagem para todos os clientes conectados."""
        if not self.active_connections:
            return

        payload = json.dumps(message)
        # Envia para todos os clientes ativos de forma assíncrona
        for connection in self.active_connections:
            try:
                await connection.send_text(payload)
            except Exception as e:
                print(f"[WS] Erro no broadcast: {e}")
                # A limpeza de conexões mortas pode ser feita aqui ou no loop de recepção
                pass

# Singleton manager
ws_manager = ConnectionManager()
