import pygame
import time
import math
import qrcode
from .window import kiosk
from core.config import config, dynamic_texts

class ScreenRender:
    """Classe base contendo helpers de renderização"""
    @staticmethod
    def draw_centered_text(text, font, color, y_pos):
        surface = font.render(text, True, color)
        rect = surface.get_rect(center=(config.SCREEN_WIDTH // 2, y_pos))
        kiosk.screen.blit(surface, rect)

class StateScreens:
    """Gerencia a renderização de cada estado da máquina de estados."""
    
    @staticmethod
    def render_idle():
        kiosk.clear()
        kiosk.dpms_control(False)
        
    @staticmethod
    def render_awake():
        kiosk.dpms_control(True)
        kiosk.clear()
        
        ScreenRender.draw_centered_text("PORTARIA DIGITAL", kiosk.font_title, kiosk.WHITE, 120)
        
        ScreenRender.draw_centered_text(dynamic_texts.ENDERECO_TEXT, kiosk.font_subtitle, kiosk.WHITE, 250)
        ScreenRender.draw_centered_text("Recebedores: " + dynamic_texts.NOMES_RECEBEDORES, kiosk.font_info, (180, 180, 180), 320)
        
        pulse = (math.sin(time.time() * 5) + 1) / 2 # 0.0 to 1.0
        g_color = int(0x99 + (0xFF - 0x99) * pulse)
        call_action_color = (0, g_color, 0)
        
        ScreenRender.draw_centered_text("APROXIME O CÓDIGO DA ENCOMENDA NO LEITOR", kiosk.font_subtitle, call_action_color, 480)

    @staticmethod
    def render_waiting_qr():
        kiosk.dpms_control(True)
        kiosk.clear()
        ScreenRender.draw_centered_text("VALIDANDO ENCOMENDA...", kiosk.font_title, kiosk.WHITE, config.SCREEN_HEIGHT//2)

    @staticmethod
    def render_authorized():
        kiosk.clear()
        ScreenRender.draw_centered_text("PORTA DESTRAVADA", kiosk.font_title, kiosk.NEON_GREEN, 200)
        ScreenRender.draw_centered_text("Abaixe a maçaneta e entre com o pacote.", kiosk.font_subtitle, kiosk.WHITE, 350)

    @staticmethod
    def render_delivering():
        kiosk.clear()
        ScreenRender.draw_centered_text("COLOQUE O PACOTE", kiosk.font_title, kiosk.WHITE, 200)
        # Amarelo/Branco informativo, NÂO É ERRO
        ScreenRender.draw_centered_text("FECHE A PORTA PARA FINALIZAR A ENTREGA", kiosk.font_subtitle, (255, 255, 50), 350)
        ScreenRender.draw_centered_text("E GERAR O SEU RECIBO", kiosk.font_subtitle, kiosk.WHITE, 430)

    @staticmethod
    def render_confirming():
        kiosk.clear()
        ScreenRender.draw_centered_text("FOTOGRAFANDO", kiosk.font_title, kiosk.WHITE, 200)
        ScreenRender.draw_centered_text("Por favor, não se movimente...", kiosk.font_subtitle, (200, 200, 200), 350)

    @staticmethod
    def render_receipt(jwt_token: str, photo_surface=None):
        kiosk.clear()
        ScreenRender.draw_centered_text("ENTREGA CONFIRMADA!", kiosk.font_title, kiosk.NEON_GREEN, 70)
        
        if jwt_token:
            # Transformamos em link URL
            url_recibo = f"https://portaria.rlight.com.br/?id={jwt_token[:30]}" 
            
            qr = qrcode.QRCode(box_size=8, border=2)
            qr.add_data(url_recibo) # Apenas Link Leve (Resolve o limite do QR)
            qr.make(fit=True)
            img = qr.make_image(fill_color="black", back_color="white")
            
            qr_surf = pygame.image.fromstring(img.tobytes(), img.size, img.mode)
            
            if photo_surface:
                ph_w = int(config.SCREEN_WIDTH * 0.4)
                ratio = ph_w / photo_surface.get_width()
                ph_h = int(photo_surface.get_height() * ratio)
                scaled_photo = pygame.transform.scale(photo_surface, (ph_w, ph_h))
                
                kiosk.screen.blit(scaled_photo, (config.SCREEN_WIDTH*0.1, 200))
                kiosk.screen.blit(qr_surf, (config.SCREEN_WIDTH*0.6, 200))
            else:
                rect = qr_surf.get_rect(center=(config.SCREEN_WIDTH//2, 300))
                kiosk.screen.blit(qr_surf, rect)

    @staticmethod
    def render_door_alert():
        kiosk.dpms_control(True)
        kiosk.clear()
        pulse = int((math.sin(time.time() * 10) + 1) / 2 * 100)
        kiosk.screen.fill(( pulse, 0, 0 ))
        
        # Alerta Padrão
        ScreenRender.draw_centered_text("ALERTA DE SEGURANÇA", kiosk.font_title, kiosk.WHITE, 150)
        
        # Texto Dinâmico de Instução (Erro Genérico Carregado do HA)
        text_lines = dynamic_texts.MSG_ERRO.split(". ")
        y_pos = 300
        for line in text_lines:
            ScreenRender.draw_centered_text(line, kiosk.font_subtitle, kiosk.WHITE, y_pos)
            y_pos += 60
            
        # Seta visual apontando p/ Baixo-Esquerda (Onde a campainha física Tapo estaria)
        # Seta em ASCII art robusta p/ Fonte grande
        ScreenRender.draw_centered_text("<------- TOQUE AQUI", kiosk.font_title, kiosk.WHITE, 500)
