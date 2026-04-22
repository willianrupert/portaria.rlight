import subprocess

class DisplayManager:
    def __init__(self):
        self.is_dpms_on = True

    def dpms_control(self, turn_on: bool):
        """Liga ou desliga retroiluminação do monitor físico IPS."""
        if self.is_dpms_on == turn_on:
            return # Evita chamadas shell redundantes
            
        cmd = "xset dpms force on" if turn_on else "xset dpms force off"
        print(f"[UI DPMS] {'LIGANDO' if turn_on else 'DESLIGANDO'} monitor via sub-processo.")
        try:
            # Esse comando supõe que exista servidor X rodando ou wayland
            subprocess.run(cmd.split(), check=False)
        except Exception:
            # Fallback para kernel puro se `xset` falhar e tentarmos mudar no tty
            try:
                with open("/sys/class/graphics/fb0/blank", "w") as f:
                    f.write("0" if turn_on else "1")
            except Exception as e:
                print(f"[UI DPMS] Fallback falhou: {e}")
            
        self.is_dpms_on = turn_on

# Singleton gerenciador da tela
display_manager = DisplayManager()
