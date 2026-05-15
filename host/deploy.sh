#!/bin/bash
# RLight Auto-Deploy Script
# Este script é executado pelo systemd antes de iniciar a API

echo "[Deploy] Iniciando atualização automática..."
cd /home/portaria/rlight || exit

# 1. Busca alterações sem mesclar
git fetch origin prod

# 2. Força o estado local a ser idêntico ao remoto (descarta mudanças locais acidentais)
git reset --hard origin/prod

# 4. Configura autologin no terminal (tty1)
if [ -f "scripts/setup_autologin.sh" ]; then
    chmod +x scripts/setup_autologin.sh
    sudo ./scripts/setup_autologin.sh
fi

echo "[Deploy] Sistema atualizado com sucesso."
