import sqlite3
import smtplib
import os
from email.mime.text import MIMEText
from datetime import datetime, timedelta

DB_PATH = "/var/lib/rlight/deliveries.db"

def generate_report():
    try:
        if not os.path.exists(DB_PATH):
            return "Banco de dados não encontrado."
            
        conn = sqlite3.connect(DB_PATH)
        week_ago = int((datetime.now() - timedelta(days=7)).timestamp())
        rows = conn.execute(
            "SELECT carrier, weight_g FROM deliveries WHERE ts > ?",
            (week_ago,)
        ).fetchall()

        total = len(rows)
        carriers = {}
        total_weight = 0.0
        for r in rows:
            carriers[r[0]] = carriers.get(r[0], 0) + 1
            total_weight += r[1]

        report_body = f"""Resumo Semanal - Portaria Autónoma RLight
        
Data: {datetime.now().strftime('%d/%m/%Y %H:%M')}
Total de Encomendas Recebidas: {total}
Peso Total Processado: {total_weight:.2f}g
        
Por transportadora:
"""
        for k, v in carriers.items():
            report_body += f"- {k}: {v} pacote(s)\n"

        return report_body
    except Exception as e:
        return f"Erro ao gerar relatório: {e}"

def send_email(body):
    smtp_server = os.getenv("SMTP_SERVER")
    smtp_port   = int(os.getenv("SMTP_PORT", "587"))
    smtp_user   = os.getenv("SMTP_USER")
    smtp_pass   = os.getenv("SMTP_PASS")
    dest_email  = os.getenv("DEST_EMAIL")

    if not all([smtp_server, smtp_user, smtp_pass, dest_email]):
        print("[Weekly Report] SMTP não configurado. E-mail simulado:")
        print(body)
        return

    msg = MIMEText(body)
    msg['Subject'] = f"Relatório Semanal RLight - {datetime.now().strftime('%d/%m/%Y')}"
    msg['From'] = smtp_user
    msg['To'] = dest_email

    try:
        with smtplib.SMTP(smtp_server, smtp_port) as server:
            server.starttls()
            server.login(smtp_user, smtp_pass)
            server.send_message(msg)
            print("[Weekly Report] E-mail enviado com sucesso.")
    except Exception as e:
        print(f"[Weekly Report] Erro ao enviar e-mail: {e}")

if __name__ == "__main__":
    relatorio = generate_report()
    send_email(relatorio)
