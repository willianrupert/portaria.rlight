// src/sensors/KeypadHandler.h
#pragma once
#include <Arduino.h>
#include <Wire.h>

/**
 * Keypad Matrix 4x3 via I2C PCF8574.
 * Otimizado para não bloqueio (non-blocking) e debounce robusto.
 */
class KeypadHandler {
public:
    static KeypadHandler& instance();
    
    bool init(uint8_t address = 0x20);
    
    // Retorna o caractere da tecla pressionada ('0'-'9', '*', '#') ou 0 se nenhuma.
    char getChar();

    // Loop interno para processar senhas e retornar códigos completos (ex: 1234#)
    void update();
    
    // Retorna código completo se finalizado com #, ou NULL.
    // O chamador deve consumir o código (ele será limpo após a chamada).
    const char* getCode(); 

    // Retorna a quantidade de dígitos já digitados no buffer parcial
    int getPendingLength();

private:
    uint8_t _addr;
    uint32_t _last_scan = 0;
    char _last_key = 0;
    char _current_code[17] = "";
    char _final_code[17] = "";
    
    const char _keys[4][3] = {
        {'1','2','3'},
        {'4','5','6'},
        {'7','8','9'},
        {'*','0','#'}
    };

    KeypadHandler() = default;
};
