import sqlite3
import os
import threading

DB_PATH = "/var/lib/rlight/deliveries.db"

class DatabaseManager:
    def __init__(self):
        self._lock = threading.Lock()
        # Garante diretório pai existe
        os.makedirs(os.path.dirname(DB_PATH), exist_ok=True)
        self._init_db()

    def _get_conn(self):
        return sqlite3.connect(DB_PATH, check_same_thread=False)

    def _init_db(self):
        with self._lock:
            with self._get_conn() as conn:
                conn.execute("""
                    CREATE TABLE IF NOT EXISTS deliveries (
                        token_uuid TEXT PRIMARY KEY,
                        ts INTEGER,
                        jwt TEXT,
                        weight_g REAL,
                        carrier TEXT,
                        photo_path TEXT,
                        synced INTEGER DEFAULT 0
                    )
                """)
                # S15: Tabela para ML / Features de Entrega
                conn.execute("""
                    CREATE TABLE IF NOT EXISTS delivery_features (
                        token_uuid TEXT PRIMARY KEY,
                        features_json TEXT,
                        score REAL,
                        FOREIGN KEY(token_uuid) REFERENCES deliveries(token_uuid)
                    )
                """)
                conn.commit()

    def insert_delivery(self, token_uuid: str, ts: int, jwt: str, weight_g: float, carrier: str, photo_path: str):
        with self._lock:
            with self._get_conn() as conn:
                conn.execute("""
                    INSERT OR REPLACE INTO deliveries 
                    (token_uuid, ts, jwt, weight_g, carrier, photo_path, synced)
                    VALUES (?, ?, ?, ?, ?, ?, 0)
                """, (token_uuid, ts, jwt, weight_g, carrier, photo_path))
                conn.commit()

    def get_pending_sync(self, limit=10):
        with self._lock:
            with self._get_conn() as conn:
                cursor = conn.execute("SELECT * FROM deliveries WHERE synced = 0 ORDER BY ts ASC LIMIT ?", (limit,))
                columns = [col[0] for col in cursor.description]
                return [dict(zip(columns, row)) for row in cursor.fetchall()]

    def mark_synced(self, token_uuid: str):
        with self._lock:
            with self._get_conn() as conn:
                conn.execute("UPDATE deliveries SET synced = 1 WHERE token_uuid = ?", (token_uuid,))
                conn.commit()

db_manager = DatabaseManager()
