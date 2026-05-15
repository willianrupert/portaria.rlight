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

# Configuração para tty1 (Console Físico/HDMI)
mkdir -p /etc/systemd/system/getty@tty1.service.d/
cat <<EOF > /etc/systemd/system/getty@tty1.service.d/autologin.conf
[Service]
ExecStart=
ExecStart=-/sbin/agetty --autologin $TARGET_USER --noclear %I \$TERM
EOF

# Configuração para ttyS0 (Console Serial/Debug)
mkdir -p /etc/systemd/system/serial-getty@ttyS0.service.d/
cat <<EOF > /etc/systemd/system/serial-getty@ttyS0.service.d/autologin.conf
[Service]
ExecStart=
ExecStart=-/sbin/agetty --autologin $TARGET_USER --keep-baud 115200,57600,38400,9600 %I \$TERM
EOF

# Recarrega o systemd
systemctl daemon-reload

echo "[Setup] Autologin configurado com sucesso para tty1."
echo "[Setup] Na próxima reinicialização, o sistema entrará automaticamente como $TARGET_USER."
