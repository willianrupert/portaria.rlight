// src/sensors/KeypadHandler.cpp
#include "KeypadHandler.h"

KeypadHandler& KeypadHandler::instance() { static KeypadHandler i; return i; }

void KeypadHandler::init(uint8_t address) {
    _addr = address;
    Wire.begin(); // ESP32-S3 default SDA/SCL (8/9 ou 41/42 dependendo da config)
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
    if (c == 0) return;

    if (c == '#') {
        // Fim da senha
        _final_code = _current_code;
        _current_code = "";
    } else if (c == '*') {
        // Limpa buffer
        _current_code = "";
    } else {
        // Adiciona dígito se não estourar 16 chars
        if (_current_code.length() < 16) {
            _current_code += c;
        }
    }
}

String KeypadHandler::getCode() {
    String res = _final_code;
    _final_code = "";
    return res;
}
