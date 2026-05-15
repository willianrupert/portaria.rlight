#!/bin/bash
# scripts/setup_autologin.sh
# Configura login automático para o usuário 'willian' no console tty1

if [ "$EUID" -ne 0 ]; then 
  echo "Por favor, execute como root (sudo)"
  exit 1
fi

# Detecta o usuário que rodou o sudo ou usa 'willian' como default
TARGET_USER=${SUDO_USER:-willian}

echo "[Setup] Configurando autologin para o usuário: $TARGET_USER"

# Cria diretório de override do getty
mkdir -p /etc/systemd/system/getty@tty1.service.d/

# Gera o arquivo de configuração
cat <<EOF > /etc/systemd/system/getty@tty1.service.d/autologin.conf
[Service]
ExecStart=
ExecStart=-/sbin/agetty --autologin $TARGET_USER --noclear %I \$TERM
EOF

# Recarrega o systemd
systemctl daemon-reload

echo "[Setup] Autologin configurado com sucesso para tty1."
echo "[Setup] Na próxima reinicialização, o sistema entrará automaticamente como $TARGET_USER."
