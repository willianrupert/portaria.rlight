// src/crypto/JwtSigner.h
#pragma once
#include <stdint.h>
#include <stddef.h>

class JwtSigner {
public:
  static JwtSigner& instance() { static JwtSigner i; return i; }
  void sign(const char* token, const char* qr_code, const char* carrier, float weight, uint32_t ts, char* out_buf, size_t out_sz);
private:
  JwtSigner() = default;
};
