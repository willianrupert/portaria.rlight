import pygame
import subprocess
from core.config import config

class KioskWindow:
    def __init__(self):
        # Esconde o mouse
        pygame.mouse.set_visible(False)
        
        # Inicialização do Pygame sem X11 preferindo framebuffer
        try:
            pygame.display.init()
            pygame.font.init()
        except Exception as e:
            print(f"[UI Base] Falha ao iniciar video driver: {e}")
            raise
            
        # OPy resoluções: Forçamos 1024x600 como exigido
        res = (config.SCREEN_WIDTH, config.SCREEN_HEIGHT)
        print(f"[UI Base] Inicializando Surface em {res[0]}x{res[1]}")
        
        # Modo FullScreen para pegar controle da tela inteira (KMS/DRM)
        self.screen = pygame.display.set_mode(res, pygame.FULLSCREEN | pygame.DOUBLEBUF | pygame.HWSURFACE)
        self.clock = pygame.time.Clock()
        self.is_dpms_on = True

        # Cores Estáticas Base (Alto Contraste)
        self.BLACK = (0, 0, 0)
        self.WHITE = (255, 255, 255)
        self.NEON_GREEN = (57, 255, 20)
        self.NEON_RED = (255, 12, 12)
        
        # Carregador de Fontes
        self.font_title = pygame.font.Font(None, 96) # Default font, tamanho gigante
        self.font_subtitle = pygame.font.Font(None, 48)
        self.font_info = pygame.font.Font(None, 36)

    def dpms_control(self, turn_on: bool):
        """Liga ou desliga retroiluminação do monitor físico IPS."""
        if self.is_dpms_on == turn_on:
            return # Evita chamadas shell redundantes
            
        cmd = "xset dpms force on" if turn_on else "xset dpms force off"
        print(f"[UI DPMS] {'LIGANDO' if turn_on else 'DESLIGANDO'} monitor via sub-processo.")
        try:
            # Esse comando supõe que exista servidor X rodando ou wayland
            # Se a placa for CLI pura (sem X), xset falhará e utilizaremos vcgencmd ou fbi
            subprocess.run(cmd.split(), check=False)
        except Exception:
            # Fallback para kernel puro se `xset` falhar e tentarmos mudar no tty
            open("/sys/class/graphics/fb0/blank", "w").write("0" if turn_on else "1") rescue None
            
        self.is_dpms_on = turn_on

    def clear(self):
        self.screen.fill(self.BLACK)

    def flip(self):
        pygame.display.flip()
        self.clock.tick(config.FPS_LIMIT)

# Singleton gerenciador da janela 
kiosk = KioskWindow()
