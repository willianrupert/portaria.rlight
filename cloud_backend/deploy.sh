#!/bin/bash
# RLight Cloud Auto-Deploy Script
# Este script é executado pelo systemd antes de iniciar o backend cloud

echo "[Cloud Deploy] Iniciando atualização automática..."
cd /home/ubuntu/cloud_backend || exit

# 1. Busca alterações sem mesclar
git fetch origin main

# 2. Força o estado local a ser idêntico ao remoto
git reset --hard origin/main

# 3. Atualiza dependências se necessário
if [ -f "package.json" ]; then
    npm install --production
fi

echo "[Cloud Deploy] Sistema atualizado com sucesso."
