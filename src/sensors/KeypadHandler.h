// src/sensors/KeypadHandler.h
#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <I2CKeyPad.h>
#include "../config/Config.h"

// Tamanho da senha (6 dígitos) + # (confirma)
#define MAX_PASS_LEN 6

class KeypadHandler {
public:
    static KeypadHandler& instance();
    void init();
    
    // Processamento não-bloqueante
    void poll();
    
    // Interface para a FSM
    bool hasValue() const { return _valueReady; }
    const char* getBuffer() const { return _buffer; }
    void reset();

    // Saúde do sensor
    bool isAlive() const { return _isAlive; }

private:
    KeypadHandler() : _keypad(I2C_ADDR_KEYPAD) {}
    
    I2CKeyPad _keypad;
    char _buffer[MAX_PASS_LEN + 1];
    uint8_t _bufIdx = 0;
    bool _valueReady = false;
    bool _isAlive = false;
    uint32_t _lastActivity = 0;
    
    // Mapeamento 4x3 para o Keypad
    char _layout[13] = "123456789*0#"; // Índice 0-11
    
    void _handleKey(char key);
};

// Funções de ISR (opcional se usarmos polling, mas guardamos espaço)
void IRAM_ATTR keypad_isr();
