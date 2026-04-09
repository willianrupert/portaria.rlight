# scripts/rtc_sync.py — v6.0
import time, json, threading

def crc8(data: bytes) -> int:
    crc = 0x00
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = (crc << 1) ^ 0x07 if crc & 0x80 else crc << 1
    return crc & 0xFF

def sync_esp32(ser) -> bool:
    """Envia timestamp atual ao ESP32 via CMD_SYNC_TIME."""
    # time.time() usa o hwclock do kernel, que gerencia o DS3231
    # Armbian já fez hwclock --hctosys no boot
    ts = int(time.time())

    payload = {"cmd": "CMD_SYNC_TIME", "ts": ts}
    raw = json.dumps(payload, separators=(",",":"))
    payload["crc"] = crc8(raw.encode())
    line = json.dumps(payload, separators=(",",":")) + "\n"

    try:
        ser.write(line.encode())
        return True
    except Exception as e:
        print(f"rtc_sync error: {e}")
        return False

def sync_ds3231_from_ntp():
    """Sincroniza DS3231 com NTP via hwclock do sistema."""
    import subprocess
    try:
        subprocess.run(["hwclock", "--systohc"], check=True)
    except Exception as e:
        print(f"hwclock sync error: {e}")

def rtc_sync_loop(ser):
    """Roda em thread daemon no serial_bridge.py."""
    while True:
        sync_esp32(ser)
        time.sleep(600)   # 10 minutos
