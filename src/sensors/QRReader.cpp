// src/sensors/QRReader.cpp
#include "QRReader.h"
#include <ctype.h>

QRReader& QRReader::instance() { static QRReader i; return i; }

bool QRReader::init() {
  Serial1.begin(9600, SERIAL_8N1, PIN_UART0_RX, PIN_UART0_TX); 
  // Nota: o código original usa a macro da Serial normal pra GM861S mas setamos na UART0/1.
  return true;
}

const char* QRReader::poll() {
  _has_result = false;

  // Lê apenas bytes disponíveis — zero espera
  while (Serial1.available()) {    // UART0/1 do GM861S
    char c = (char)Serial1.read();

    if (c == '\r' || c == '\n') {
      if (_rx_pos > 0) {
        _rx_buf[_rx_pos] = '\0';
        strlcpy(_result, _rx_buf, sizeof(_result));
        _rx_pos     = 0;
        _has_result = true;
        break;
      }
    } else {
      if (_rx_pos < QR_RX_BUF_SIZE - 1) {
        _rx_buf[_rx_pos++] = c;
      } else {
        // Buffer cheio: código inválido, descarta
        _rx_pos = 0;
      }
    }
  }

  return _has_result ? _result : nullptr;
}

// Identificação de transportadora por prefixo/estrutura
static bool isCorreios(const char* s) {
  if (strlen(s) != 13) return false;
  if (!isalpha(s[0]) || !isalpha(s[1])) return false;
  if (s[11] != 'B' || s[12] != 'R') return false;
  for (int i = 2; i < 11; i++) if (!isdigit(s[i])) return false;
  return true;
}

void QRReader::identifyCarrier(const char* code, char* out, size_t sz) {
  size_t len = strlen(code);
  if (isCorreios(code))                              { strlcpy(out,"CORREIOS",sz); return; }
  if (strncmp(code,"SPX",3)==0 && len>=12)           { strlcpy(out,"SHOPEE",sz);   return; }
  if (strncmp(code,"TBA",3)==0 && len>=14)           { strlcpy(out,"AMAZON",sz);   return; }
  if ((strncmp(code,"OI",2)==0 || strncmp(code,"LP",2)==0 ||
       strncmp(code,"MLE",3)==0) && len>=16)         { strlcpy(out,"ML",sz);       return; }
  if (strncmp(code,"LGI",3)==0 && len>=14)           { strlcpy(out,"LOGGI",sz);    return; }
  if (strncmp(code,"J",1)==0 && len>=12)             { strlcpy(out,"JADLOG",sz);   return; }
  strlcpy(out, len >= 10 ? "UNKNOWN" : "INVALID", sz);
}

bool QRReader::isAllowedCarrier(const char* carrier) {
  // Modo lista de bloqueio: INVALID não passa, todo o resto abre
  return strcmp(carrier, "INVALID") != 0;
}
