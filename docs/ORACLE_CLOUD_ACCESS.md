# Oracle Cloud Access - RLight Infrastructure (Ampere Update)

Este documento centraliza as informações necessárias para acessar e gerenciar a infraestrutura multi-instância da RLight na Oracle Cloud, atualizada para a arquitetura Ampere.

---

## 🏗️ Arquitetura de Rede

A infraestrutura é composta por instâncias em uma VCN privada, acessadas via Bastion:

| Instância | IP Público | IP Privado | Função |
| :--- | :--- | :--- | :--- |
| **Micro 1** | `136.248.96.1` | `10.0.0.134` | **Bastion / Gateway / Nginx** |
| **Ampere** | N/A | `10.0.0.230` | **App Server (Principal)** |

---

## 🔑 Localização das Chaves SSH

As chaves agora estão centralizadas na pasta do projeto de servidores:
- **Bastion**: `/Users/willian/Projetos/RLight_Servers/Oracle_Server/ChavesOracleCloud/136.248.96.1/ssh-key-2025-11-29.key`
- **Ampere**: `/Users/willian/Projetos/RLight_Servers/Oracle_Server/ChavesOracleCloud/Ampere/ssh-key-2026-03-11.key`

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
