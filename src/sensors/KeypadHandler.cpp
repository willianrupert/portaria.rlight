// src/sensors/KeypadHandler.cpp
#include "KeypadHandler.h"
#include "../config/Config.h"
#include "../comms/UsbBridge.h"
#include "../sensors/Scale.h"
#include "../actuators/Strike.h"
#include "../health/HealthMonitor.h"
#include "../fsm/StateMachine.h"
#include "../actuators/Led.h"

KeypadHandler& KeypadHandler::instance() { static KeypadHandler i; return i; }

bool KeypadHandler::init(uint8_t address) {
    _addr = address;
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    
    // S21: I2C Connectivity Check
    Wire.beginTransmission(_addr);
    bool ok = (Wire.endTransmission() == 0);
    if (!ok) Serial.printf("[Keypad] Falha ao encontrar PCF8574 em 0x%02X\n", _addr);
    return ok;
}

char KeypadHandler::getChar() {
    uint32_t now = millis();
    if (now - _last_scan < 50) return 0; // 20Hz scan
    _last_scan = now;

    char found = 0;
    // PCF8574: P0-P3 Rows (Outputs), P4-P6 Cols (Inputs)
    // Para ler: Seta Linha LOW e lê Colunas.
    
    for (int r = 0; r < 4; r++) {
        // Seta todas as linhas em HIGH exceto a atual em LOW
        uint8_t data = 0xFF & ~(1 << r);
        Wire.beginTransmission(_addr);
        Wire.write(data);
        Wire.endTransmission();

        // Lê o estado das colunas (P4, P5, P6)
        Wire.requestFrom(_addr, (uint8_t)1);
        if (Wire.available()) {
            uint8_t read = Wire.read();
            for (int c = 0; c < 3; c++) {
                // Colunas P4, P5, P6. Se bit for 0, está pressionada.
                if (!(read & (1 << (c + 4)))) {
                    found = _keys[r][c];
                    break;
                }
            }
        }
        if (found) break;
    }

    // Debounce simples: evita repetição rápida
    if (found != 0) {
        if (found == _last_key) return 0;
        _last_key = found;
        return found;
    } else {
        _last_key = 0;
        return 0;
    }
}

void KeypadHandler::update() {
    char c = getChar();
    
    // Reporta saúde (pelo menos o I2C respondeu algo)
    HealthMonitor::instance().report(SensorID::KEYPAD, true, "poll");

    if (c == 0) return;
    
    // Feedback tátil visual: 80ms de brilho no botão
    Led::btn().blink_once(80);

    if (c == '#') {
        // Fim da senha
        strlcpy(_final_code, _current_code, sizeof(_final_code));
        _current_code[0] = '\0';
    } else if (c == '*') {
        // Limpa buffer
        _current_code[0] = '\0';
    } else {
        // Adiciona dígito se não estourar 16 chars
        size_t len = strlen(_current_code);
        if (len < 16) {
            _current_code[len] = c;
            _current_code[len + 1] = '\0';
            UsbBridge::instance().sendEvent("KEY_PRESSED", StateMachine::instance().ctx());
        }
    }
}

const char* KeypadHandler::getCode() {
    if (_final_code[0] == '\0') return NULL;
    static char buf[17];
    strlcpy(buf, _final_code, sizeof(buf));
    _final_code[0] = '\0'; // Consome o código
    return buf;
}

int KeypadHandler::getPendingLength() {
    return strlen(_current_code);
}
