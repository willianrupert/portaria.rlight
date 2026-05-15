# SSH Access - RLight Infrastructure (Cloud & Local)

Este documento centraliza as informações necessárias para acessar e gerenciar a infraestrutura da RLight, incluindo os servidores na Oracle Cloud (Ampere) e o hardware local (Orange Pi).

---

## 🏗️ Arquitetura de Rede

### ☁️ Cloud (Oracle Cloud Infrastructure)
A infraestrutura em nuvem é composta por instâncias em uma VCN privada, acessadas via Bastion:

| Instância | IP Público | IP Privado | Função |
| :--- | :--- | :--- | :--- |
| **Micro 1** | `136.248.96.1` | `10.0.0.134` | **Bastion / Gateway / Nginx** |
| **Ampere** | N/A | `10.0.0.230` | **App Server (Principal)** |

### 🏠 Local (Hardware Host)
O host local que orquestra os sensores e o ESP32:

| Dispositivo | IP Local | Usuário | Função |
| :--- | :--- | :--- | :--- |
| **Orange Pi Zero 3** | `192.168.88.91` | `portaria` | **Host Orchestrator / Local API** |

---

## 🔑 Localização das Chaves SSH

### ☁️ Cloud Keys
- **Bastion**: `/Users/willian/Projetos/RLight_Servers/Oracle_Server/ChavesOracleCloud/136.248.96.1/ssh-key-2025-11-29.key`
- **Ampere**: `/Users/willian/Projetos/RLight_Servers/Oracle_Server/ChavesOracleCloud/Ampere/ssh-key-2026-03-11.key`

### 🏠 Local Keys
- **Orange Pi**: `~/.ssh/id_rlight_opi` (Chave ED25519)

---

## 🚀 Como Acessar (SSH)

### 1. Configuração Recomendada (`~/.ssh/config`)
Adicione este trecho ao seu arquivo de configuração SSH local para acesso rápido:

```text
Host oracle-micro-1
    HostName 136.248.96.1
    User ubuntu
    IdentityFile ~/Projetos/RLight_Servers/Oracle_Server/ChavesOracleCloud/136.248.96.1/ssh-key-2025-11-29.key

Host oracle-ampere
    HostName 10.0.0.230
    User ubuntu
    IdentityFile ~/Projetos/RLight_Servers/Oracle_Server/ChavesOracleCloud/Ampere/ssh-key-2026-03-11.key
    ProxyJump oracle-micro-1

Host rlight-opi
    HostName 192.168.88.91
    User portaria
    IdentityFile ~/.ssh/id_rlight_opi
```

### 2. Comandos de Acesso Direto
Caso não utilize o config:

- **Acessar Bastion**:
  ```bash
  ssh -i ~/Projetos/RLight_Servers/Oracle_Server/ChavesOracleCloud/136.248.96.1/ssh-key-2025-11-29.key ubuntu@136.248.96.1
  ```

- **Acessar Ampere (via Bastion)**:
  ```bash
  ssh -i ~/Projetos/RLight_Servers/Oracle_Server/ChavesOracleCloud/Ampere/ssh-key-2026-03-11.key \
      -o "ProxyCommand ssh -i ~/Projetos/RLight_Servers/Oracle_Server/ChavesOracleCloud/136.248.96.1/ssh-key-2025-11-29.key -W %h:%p ubuntu@136.248.96.1" \
      ubuntu@10.0.0.230
  ```

---

## 📋 Serviços Rodando (Ampere)

O backend principal agora roda na instância Ampere gerenciado pelo **PM2**:

- **rlight-cloud**: Porta `3005` (Backend + UI)
- **prompt-api**: Porta `8000`
- **prompt-ui**: Porta `8501`

**NGINX (na Micro 1)**: Redireciona o tráfego de `portaria.rlight.com.br` para a porta `3005` da Ampere via rede interna.

---

## 🛠️ Comandos de Deploy (SCP)

Para enviar atualizações para o App Server:

```bash
# Exemplo: Atualizar index.html na Ampere
scp -i ~/Projetos/RLight_Servers/Oracle_Server/ChavesOracleCloud/Ampere/ssh-key-2026-03-11.key \
    -o "ProxyJump ubuntu@136.248.96.1" \
    ./public/index.html ubuntu@10.0.0.230:~/cloud_backend/public/index.html
```

---
*Atualizado em 15 de Maio de 2026 conforme nova infraestrutura Ampere.*
