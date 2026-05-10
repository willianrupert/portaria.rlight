import cv2
import threading
import os
from core.config import config

class WebCamCapture:
    """Acesso direto à WebCam Headless via OpenCV para tirar a foto oficial da entrega."""
    
    def __init__(self):
        self.lock = threading.Lock()
        self.last_photo_path = os.path.join(os.path.dirname(__file__), "..", "ui", "web", "assets", "last_delivery.jpg")
        
        # Garante que a pasta assets existe
        os.makedirs(os.path.dirname(self.last_photo_path), exist_ok=True)

    def capture_snapshot(self, token: str = "last_delivery") -> str:
        """Abre a câmera, tira a foto, salva no disco e retorna o caminho relativo para a UI."""
        with self.lock:
            try:
                cap = cv2.VideoCapture(config.WEBCAM_ID)
                cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
                cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
                
                # Descarta os primeiros frames (ajuste automático de brilho/foco)
                for _ in range(5):
                    cap.read()
                    
                ret, frame = cap.read()
                cap.release()

                if ret:
                    photo_name = f"{token}.jpg"
                    photo_path = os.path.join("/tmp/rlight_photos", photo_name)
                    os.makedirs("/tmp/rlight_photos", exist_ok=True)
                    
                    # Salva no disco com qualidade otimizada para o recibo
                    cv2.imwrite(photo_path, frame, [int(cv2.IMWRITE_JPEG_QUALITY), 80])
                    
                    # Também salva em last_delivery para o dashboard
                    cv2.imwrite(os.path.join("/tmp/rlight_photos", "last_delivery.jpg"), frame, [int(cv2.IMWRITE_JPEG_QUALITY), 80])
                    
                    # Retorna o path relativo que o local_api (static) já serve para o front-end
                    return f"/photos/{photo_name}"
                return None
            except Exception as e:
                print(f"[WebCam] Erro ao capturar snapshot: {e}")
                return None

webcam = WebCamCapture()
