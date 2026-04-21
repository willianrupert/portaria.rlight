#!/bin/bash
export DISPLAY=:0

# 1. Garantir que o servidor X esteja rodando (Xorg)
# No Armbian sem desktop, precisamos rodar o X manualmente.
if ! pgrep Xorg > /dev/null; then
    echo "[Kiosk] Iniciando Xorg..."
    # Roda em background, sem cursor, no display :0
    sudo Xorg :0 -nocursor -quiet &
    sleep 3
fi

# 2. Configurações de Display (agora que o X subiu)
xset s off
xset -dpms
xset s noblank

# 3. Gerenciador de Janelas Matchbox
matchbox-window-manager -use_titlebar no &

# 4. Limpeza de Cache do Chromium
rm -rf /home/portaria/.config/chromium

# 5. Launch Chromium
chromium \
    --kiosk \
    --noerrdialogs \
    --disable-infobars \
    --check-for-update-interval=31536000 \
    --window-size=600,1024 \
    --window-position=0,0 \
    --app=http://localhost:8080
