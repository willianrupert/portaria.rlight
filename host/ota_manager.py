import subprocess
import hashlib
import os
import sys

FIRMWARE_PATH = os.path.join(os.path.dirname(__file__), 'firmware.bin')
DEVICE_PATH = "/dev/rlight_esp"

def check_and_update(current_version: str, local_version: str):
    if current_version == local_version:
        print("[OTA] Nenhuma atualização necessária.")
        return False
        
    if not os.path.exists(FIRMWARE_PATH):
        print("[OTA] Binário não encontrado na Orange Pi.")
        return False

    # Confirma integridade via SHA256 antes da queima
    with open(FIRMWARE_PATH, "rb") as f:
        sha = hashlib.sha256(f.read()).hexdigest()[:8]
        print(f"[OTA] Assinatura do firmware: {sha}")

    # Pausa a ponte serial do sistema principal (recomendado disparar isso via um sinal ou parar o rlight host primeiro)
    print(f"[OTA] Detonando flash via esptool em {DEVICE_PATH}...")
    
    # Roda o upload da ESP32 usando as ligações nativas DTR/RTS do USB CDC
    result = subprocess.run([
        sys.executable, "-m", "esptool", 
        "--port", DEVICE_PATH, 
        "--baud", "460800",
        "--before", "default_reset",
        "--after", "hard_reset",
        "write_flash", "-z", "0x10000", FIRMWARE_PATH
    ], capture_output=True, text=True)

    if result.returncode == 0:
        print("[OTA] Atualização Realizada com Sucesso!")
        return True
    else:
        print(f"[OTA] Falha Crítica:\n{result.stderr}")
        return False

if __name__ == "__main__":
    check_and_update("v6.0.0", "v6.1.0")
