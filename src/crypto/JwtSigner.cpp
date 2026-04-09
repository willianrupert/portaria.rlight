// src/crypto/JwtSigner.cpp
#include "JwtSigner.h"
#include <stdio.h>

void JwtSigner::sign(const char* token, const char* qr_code, const char* carrier, float weight, uint32_t ts, char* out_buf, size_t out_sz) {
  // Simulação de geração JWT sem a key real e biblioteca pesada por enquanto
  snprintf(out_buf, out_sz, "eyJhbG.. simulating JWT for %s (%.1fg)", qr_code, weight);
}
