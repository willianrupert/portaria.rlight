# Oracle Cloud Access - RLight Backend

Este documento centraliza as informações necessárias para acessar e gerenciar o servidor de nuvem da Oracle onde o backend do RLight está hospedado.

---

## 🌐 Informações de Conexão

- **IP Público**: `136.248.96.1`
- **Usuário**: `ubuntu`
- **Chave SSH**: `/Users/willian/Documents/ChavesOracleCloud/136.248.96.1/ssh-key-2025-11-29.key`
- **Subdomínio**: `portaria.rlight.com.br`

### Comando de Acesso (Terminal)
```bash
ssh -i /Users/willian/Documents/ChavesOracleCloud/136.248.96.1/ssh-key-2025-11-29.key ubuntu@136.248.96.1
```

---

## 📁 Estrutura do Projeto

- **Diretório do Backend**: `/home/ubuntu/cloud_backend/`
- **Arquivos Públicos (UI)**: `/home/ubuntu/cloud_backend/public/`
- **Logs do Servidor**: Gerenciados via PM2.

---

## 🚀 Gerenciamento de Processos (PM2)

O backend utiliza o **PM2** para garantir que o servidor Node.js permaneça online 24/7.

- **Listar processos**: `pm2 list`
- **Ver logs em tempo real**: `pm2 logs server`
- **Reiniciar o servidor**: `pm2 restart server`
- **Parar o servidor**: `pm2 stop server`

---

## 🛡️ Configuração de Rede e NGINX

O NGINX atua como proxy reverso para a porta `3005`.

- **Arquivo de Configuração**: `/etc/nginx/sites-available/portaria.rlight.conf`
- **Testar configuração**: `sudo nginx -t`
- **Recarregar NGINX**: `sudo systemctl reload nginx`

---

## 🔐 Certificados SSL (Certbot)

Os certificados são gerenciados pelo Certbot (Let's Encrypt).

- **Verificar status**: `sudo certbot certificates`
- **Renovação manual**: `sudo certbot renew`

---

## 🛠️ Comandos Úteis de SCP (Upload)

Para enviar atualizações da sua máquina local para a nuvem:

### Atualizar o Dashboard (index.html)
```bash
scp -i /Users/willian/Documents/ChavesOracleCloud/136.248.96.1/ssh-key-2025-11-29.key ./cloud_backend/public/index.html ubuntu@136.248.96.1:/home/ubuntu/cloud_backend/public/index.html
```

### Atualizar o Servidor (server.js)
```bash
scp -i /Users/willian/Documents/ChavesOracleCloud/136.248.96.1/ssh-key-2025-11-29.key ./cloud_backend/server.js ubuntu@136.248.96.1:/home/ubuntu/cloud_backend/server.js
```

---
*Documentação gerada em 10 de Maio de 2026.*
