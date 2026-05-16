#!/bin/bash
# scripts/kiosk.sh
# Inicia a interface visual do RLight v8 no Orange Pi

export DISPLAY=:0

# Aguarda a API estar disponível (porta 8080)
while ! nc -z localhost 8080; do   
  sleep 1
done

# Desabilita screensaver e economia de energia
xset s off
xset -dpms
xset s noblank

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
    --window-size=1024,600 \
    --window-position=0,0 \
    --autoplay-policy=no-user-gesture-required \
    --check-for-update-interval=31536000 \
    http://localhost:8080/ui/index.html
