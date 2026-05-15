#include "QRReader.h"
#include <ctype.h>
#include "../health/HealthMonitor.h"

QRReader& QRReader::instance() { static QRReader i; return i; }

bool QRReader::init() {
  Serial1.begin(9600, SERIAL_8N1, PIN_UART0_RX, PIN_UART0_TX); 
  delay(100); // Aguarda boot do sensor
  configure();
  // Assume OK se inicializou serial; a saúde real é validada no poll()
  HealthMonitor::instance().report(SensorID::GM861, true, "init");
  return true;
}
const char* QRReader::poll() {
  _has_result = false;
  
  // S20: Rate limiter
  if (millis() < _cooldown_until) {
    while (Serial1.available()) Serial1.read(); 
    return nullptr;
  }

  // Lê bytes disponíveis do GM861S
  while (Serial1.available()) {
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
        _rx_pos = 0;
      }
    }
  }

  if (_has_result) {
    HealthMonitor::instance().report(SensorID::GM861, true, "data_rx");
    char carrier_buf[24];
    identifyCarrier(_result, carrier_buf, sizeof(carrier_buf));
    
    if (!isAllowedCarrier(carrier_buf)) {
      if (millis() - _last_read_time < 2000) {
        _failed_reads++;
      } else {
        _failed_reads = 1;
      }
      _last_read_time = millis();
      
      if (_failed_reads >= 3) {
        _cooldown_until = millis() + 5000; // 5s block
        _failed_reads = 0;
      }
      return nullptr;
    } else {
      _failed_reads = 0;
    }
  }

  return _has_result ? _result : nullptr;
}

void QRReader::configure() {
  // Configuração global (Zone Bit 0x0000):
  // Bit 6: 0 (Mute ON)
  // Bit 5-4: 01 (Standard Light)
  // Bit 3-2: 01 (Standard Aiming)
  // Bit 1-0: 01 (Command Trigger Mode)
  // Valor: 0x15
  uint8_t cfg_cmd[] = {0x7E, 0x00, 0x08, 0x01, 0x00, 0x00, 0x15, 0xAB, 0xCD};
  _send_command(cfg_cmd, sizeof(cfg_cmd));
  delay(50);
  
  // Salva configurações na Flash do sensor para persistir após reboot
  uint8_t save_cmd[] = {0x7E, 0x00, 0x09, 0x01, 0x00, 0x00, 0x00, 0xDE, 0xC8};
  _send_command(save_cmd, sizeof(save_cmd));
}

void QRReader::start_scan() {
  // Comando de Trigger Serial (Página 26 do manual)
  uint8_t trigger[] = {0x7E, 0x00, 0x08, 0x01, 0x00, 0x02, 0x01, 0xAB, 0xCD};
  _send_command(trigger, sizeof(trigger));
}

void QRReader::stop_scan() {
  // Para interromper a leitura antes do timeout (setando bit trigger para 0)
  uint8_t stop[] = {0x7E, 0x00, 0x08, 0x01, 0x00, 0x02, 0x00, 0xAB, 0xCD};
  _send_command(stop, sizeof(stop));
}

void QRReader::_send_command(const uint8_t* cmd, size_t len) {
  Serial1.write(cmd, len);
  Serial1.flush();
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
