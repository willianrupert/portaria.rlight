import requests
import threading
import time
from core.config import config
from core.db import db_manager

class OracleRESTClient:
    """Integração final em background (Offline-First) com Banco SQLite e Retry Backoff (S7)."""
    def __init__(self):
        self.headers = {
            "Authorization": f"Bearer {config.ORACLE_BEARER}"
        }
        self.running = False
        self.worker_thread = None

    def start_sync_worker(self):
        """Inicia a thread de sincronização em segundo plano."""
        if self.running:
            return
        self.running = True
        self.worker_thread = threading.Thread(target=self._sync_loop, daemon=True)
        self.worker_thread.start()

    def _sync_loop(self):
        """Fila persistente de eventos com retry exponencial infinito."""
        backoff = 1
        while self.running:
            try:
                pending = db_manager.get_pending_sync(limit=10)
                if not pending:
                    time.sleep(10)
                    continue
                
                for row in pending:
                    if self._upload_delivery(row):
                        db_manager.mark_synced(row['token_uuid'])
                        backoff = 1  # Reseta o backoff on success
                time.sleep(backoff)
            except Exception as e:
                print(f"[Oracle Sync] Erro no Worker! Backoff acionado. Detalhe: {e}")
                backoff = min(backoff * 2, 1800) # Até 30 mins
                time.sleep(backoff)

    def _upload_delivery(self, row_data) -> bool:
        """Tentativa individual de upload."""
        photo_path = row_data['photo_path']
        try:
            # Carrega arquivo fswebcam salvo no disco/RAM
            with open(photo_path, 'rb') as f:
                photo_bytes = f.read()

            files = {'image': (f"{row_data['token_uuid']}.jpg", photo_bytes, 'image/jpeg')}
            data = {
                'delivery_token': row_data['jwt'],
                'timestamp': str(row_data['ts']),
                'weight': str(row_data['weight_g']),
                'carrier': row_data['carrier']
            }
            
            print(f"[Oracle Cloud] Subindo comprovante {row_data['token_uuid']}...")
            response = requests.post(
                config.ORACLE_API_URL, 
                headers=self.headers,
                data=data,
                files=files,
                timeout=15
            )
            
            if response.status_code in [200, 201]:
                print("[Oracle Cloud] Upload efetuado com SUCESSO!")
                return True
            else:
                print(f"[Oracle Cloud] Falha ao subir {row_data['token_uuid']}: {response.status_code}")
                return False
        except Exception as e:
            print(f"[Oracle Cloud] Offline (Esperando voltar para {row_data['token_uuid']}): {e}")
            return False

oracle_api = OracleRESTClient()
