import pygame
import time
import math
import qrcode
from .window import kiosk
from core.config import config, dynamic_texts

class ScreenRender:
    """Classe base contendo helpers de renderização visual (S22)"""
    @staticmethod
    def draw_centered_text(text, font, color, y_pos):
        surface = font.render(text, True, color)
        rect = surface.get_rect(center=(config.SCREEN_WIDTH // 2, y_pos))
        kiosk.screen.blit(surface, rect)

    @staticmethod
    def draw_pulsing_arrow(text, y_pos, color_base):
        """Desenha setas com animação de pulso (S22) para chamar a atenção."""
        pulse = (math.sin(time.time() * 6) + 1) / 2 # 0 a 1
        color = (
            min(255, int(color_base[0] * (0.5 + pulse*0.5))),
            min(255, int(color_base[1] * (0.5 + pulse*0.5))),
            min(255, int(color_base[2] * (0.5 + pulse*0.5)))
        )
        ScreenRender.draw_centered_text(text, kiosk.font_title, color, y_pos)

class StateScreens:
    """Gerencia a renderização de cada estado focando na experiência do entregador."""
    
    @staticmethod
    def render_idle():
        kiosk.clear()
        kiosk.dpms_control(False)
        
    @staticmethod
    def render_awake():
        kiosk.dpms_control(True)
        kiosk.clear()
        
        ScreenRender.draw_centered_text(dynamic_texts.ENDERECO_TEXT, kiosk.font_title, kiosk.WHITE, 120)
        ScreenRender.draw_centered_text("Recebedores Autorizados:", kiosk.font_subtitle, (200, 200, 200), 220)
        ScreenRender.draw_centered_text(dynamic_texts.NOMES_RECEBEDORES, kiosk.font_title, kiosk.NEON_GREEN, 290)
        
        ScreenRender.draw_pulsing_arrow("APROXIME O PACOTE DO LEITOR ACIMA ->", 450, (255, 255, 0))
        ScreenRender.draw_centered_text("(Tempo limite 30 segundos)", kiosk.font_info, (150, 150, 150), 520)

    @staticmethod
    def render_waiting_qr():
        # A própria FSM drena os 30s. Tela exibe variação simples.
        StateScreens.render_awake()

    @staticmethod
    def render_authorized():
        kiosk.clear()
        ScreenRender.draw_centered_text("AUTORIZADO!", kiosk.font_title, kiosk.NEON_GREEN, 150)
        ScreenRender.draw_pulsing_arrow("EMPURRE A PORTA AGORA ==>", 320, kiosk.WHITE)
        ScreenRender.draw_centered_text("A fechadura elétrica já está liberada.", kiosk.font_subtitle, (200,200,200), 450)

    @staticmethod
    def render_delivering():
        kiosk.clear()
        ScreenRender.draw_centered_text("DEIXE O PACOTE NA BALANÇA", kiosk.font_title, kiosk.WHITE, 150)
        ScreenRender.draw_pulsing_arrow("<== SAIA E FECHE A PORTA", 320, (255, 255, 50))
        ScreenRender.draw_centered_text("O recibo será gerado quando a porta travar.", kiosk.font_subtitle, (200,200,200), 450)

    @staticmethod
    def render_confirming():
        kiosk.clear()
        ScreenRender.draw_centered_text("GERANDO COMPROVANTE...", kiosk.font_title, kiosk.WHITE, 250)
        ScreenRender.draw_centered_text("Registrando peso e foto criptografada.", kiosk.font_subtitle, (150,150,150), 350)

    @staticmethod
    def render_resident_p1():
        kiosk.clear()
        ScreenRender.draw_centered_text("BEM-VINDO!", kiosk.font_title, kiosk.NEON_GREEN, 150)
        ScreenRender.draw_pulsing_arrow("ENTRE E FECHE A PORTA ==>", 320, kiosk.WHITE)
        ScreenRender.draw_centered_text("A porta interna abrirá automaticamente.", kiosk.font_subtitle, (200, 200, 200), 450)

    @staticmethod
    def render_resident_p2():
        kiosk.clear()
        ScreenRender.draw_centered_text("AGUARDE...", kiosk.font_title, (255, 255, 0), 150)
        ScreenRender.draw_centered_text("Liberando acesso seguro.", kiosk.font_subtitle, kiosk.WHITE, 320)

    @staticmethod
    def render_reverse_pickup():
        kiosk.clear()
        ScreenRender.draw_centered_text("COLETA AUTORIZADA", kiosk.font_title, kiosk.NEON_GREEN, 150)
        ScreenRender.draw_pulsing_arrow("RETIRE O PACOTE E FECHE A PORTA <==", 320, (255, 255, 50))
        ScreenRender.draw_centered_text("Recibo gerado após o travamento.", kiosk.font_subtitle, (200, 200, 200), 450)

    @staticmethod
    def render_receipt(jwt_token: str, photo_surface=None):
        kiosk.clear()
        ScreenRender.draw_centered_text("ENTREGA COM SUCESSO!", kiosk.font_title, kiosk.NEON_GREEN, 60)
        
        if jwt_token:
            url_recibo = f"https://portaria.rlight.com.br/?id={jwt_token[:30]}" 
            
            qr = qrcode.QRCode(box_size=8, border=2)
            qr.add_data(url_recibo)
            qr.make(fit=True)
            img = qr.make_image(fill_color="black", back_color="white")
            qr_surf = pygame.image.fromstring(img.tobytes(), img.size, img.mode)
            
            if photo_surface:
                ph_w = int(config.SCREEN_WIDTH * 0.4)
                ratio = ph_w / photo_surface.get_width()
                ph_h = int(photo_surface.get_height() * ratio)
                scaled_photo = pygame.transform.scale(photo_surface, (ph_w, ph_h))
                
                kiosk.screen.blit(scaled_photo, (config.SCREEN_WIDTH*0.1, 150))
                kiosk.screen.blit(qr_surf, (config.SCREEN_WIDTH*0.6, 170))
            else:
                rect = qr_surf.get_rect(center=(config.SCREEN_WIDTH//2, 300))
                kiosk.screen.blit(qr_surf, rect)
                
            ScreenRender.draw_centered_text("Fotografe o QR Code acima para sua segurança.", kiosk.font_subtitle, kiosk.WHITE, 520)

    @staticmethod
    def render_door_alert():
        kiosk.dpms_control(True)
        kiosk.clear()
        pulse = int((math.sin(time.time() * 10) + 1) / 2 * 100)
        kiosk.screen.fill(( pulse, 0, 0 ))
        
        ScreenRender.draw_centered_text("! PORTA BLOQUEADA !", kiosk.font_title, kiosk.WHITE, 120)
        
        text_lines = dynamic_texts.MSG_ERRO.split(". ")
        y_pos = 280
        for line in text_lines:
            ScreenRender.draw_centered_text(line, kiosk.font_subtitle, kiosk.WHITE, y_pos)
            y_pos += 60
            
        ScreenRender.draw_pulsing_arrow("V APERTE A CAMPAINHA AQUI ABAIXO V", 480, kiosk.WHITE)
