#!/bin/bash
# RLight Auto-Deploy Script
# Este script é executado pelo systemd antes de iniciar a API

echo "[Deploy] Iniciando atualização automática..."
cd /home/portaria/rlight || exit

# 1. Busca alterações sem mesclar
git fetch origin prod

# 2. Força o estado local a ser idêntico ao remoto (descarta mudanças locais acidentais)
git reset --hard origin/prod

# 3. Permissões (opcional mas recomendado)
chmod +x host/deploy.sh

echo "[Deploy] Sistema atualizado com sucesso."
