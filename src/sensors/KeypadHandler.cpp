// src/sensors/KeypadHandler.cpp
#include "KeypadHandler.h"
#include "../comms/UsbBridge.h"
#include "../fsm/StateMachine.h"

// Instância singleton
KeypadHandler& KeypadHandler::instance() {
    static KeypadHandler i;
    return i;
}

// Flag de interrupção
volatile bool g_keypadEvent = false;
void IRAM_ATTR keypad_isr() {
    g_keypadEvent = true;
}

void KeypadHandler::init() {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    if (_keypad.begin()) {
        _isAlive = true;
        _keypad.loadKeyMap(_layout);
        pinMode(PIN_KEYPAD_INT, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(PIN_KEYPAD_INT), keypad_isr, FALLING);
        reset();
        Serial.println("[Keypad] PCF8574 detectado e iniciado.");
    } else {
        _isAlive = false;
        Serial.println("[Keypad] ERRO: PCF8574 nao encontrado!");
    }
}

void KeypadHandler::poll() {
    if (!_isAlive) return;

    // Só lê se houver evento de interrupção ou se o polling for necessário
    // Nota: I2CKeyPad library lida com o debouncing interno básico
    if (g_keypadEvent || (millis() - _lastActivity > 50)) {
        uint8_t index = _keypad.getKey();
        if (index < 12) { // 0-11 são teclas válidas no layout 4x3
            char key = _layout[index];
            _handleKey(key);
            _lastActivity = millis();
        }
        g_keypadEvent = false; 
    }
}

void KeypadHandler::_handleKey(char key) {
    // Notifica o Host sobre a tecla (para asteriscos na tela)
    // Usamos um evento genérico por enquanto que o Host vai entender
    FsmContext& ctx = StateMachine::instance().context();
    
    if (key == '#') {
        // CONFIRMAR
        _buffer[_bufIdx] = '\0';
        _valueReady = true;
        Serial.print("[Keypad] Senha enviada: ");
        Serial.println("******"); // Segurança: não loga senha em texto claro
    } else if (key == '*') {
        // LIMPAR
        reset();
        UsbBridge::instance().sendEvent("KEY_CLEAR", ctx);
    } else {
        // DÍGITO
        if (_bufIdx < MAX_PASS_LEN) {
            _buffer[_bufIdx++] = key;
            UsbBridge::instance().sendEvent("KEY_PRESS", ctx); // Host mostra um '*'
        }
    }
}

void KeypadHandler::reset() {
    memset(_buffer, 0, sizeof(_buffer));
    _bufIdx = 0;
    _valueReady = false;
}
