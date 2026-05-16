#!/bin/bash
# scripts/kiosk.sh
# Inicia a interface visual do RLight v8 no Orange Pi

export DISPLAY=:0

# Aguarda a API estar disponível (porta 8080)
while ! python3 -c "import socket; s = socket.socket(); s.connect(('localhost', 8080))" 2>/dev/null; do   
  sleep 1
done

# Desabilita screensaver e economia de energia
xset s off
xset -dpms
xset s noblank

# Rotaciona a tela para a esquerda (90 graus CCW - vertical)
echo "[Kiosk] Rotacionando tela para a esquerda..."
xrandr --output HDMI-1 --rotate left

# Limpa flag de erro do Chromium se existir
sed -i 's/"exited_cleanly":false/"exited_cleanly":true/' ~/.config/chromium/Default/Preferences 2>/dev/null
sed -i 's/"exit_type":"Crashed"/"exit_type":"Normal"/' ~/.config/chromium/Default/Preferences 2>/dev/null

echo "[Kiosk] Iniciando Chromium em modo Fullscreen..."

# Inicia o Chromium
/usr/bin/chromium \
    --no-sandbox \
    --noerrdialogs \
    --disable-infobars \
    --kiosk \
    --autoplay-policy=no-user-gesture-required \
    --check-for-update-interval=31536000 \
    http://localhost:8080/ui/index.html
