// src/crypto/JwtSigner.cpp
#include "JwtSigner.h"
#include <stdio.h>
#include <string.h>
#include <ArduinoJson.h>
#include "mbedtls/md.h"
#include "mbedtls/base64.h"

static void base64url_encode(const unsigned char *src, size_t src_len, char *dst, size_t dst_len) {
    size_t olen = 0;
    mbedtls_base64_encode((unsigned char *)dst, dst_len, &olen, src, src_len);
    // Convert to base64url
    for (size_t i = 0; i < olen; i++) {
        if (dst[i] == '+') dst[i] = '-';
        else if (dst[i] == '/') dst[i] = '_';
        else if (dst[i] == '=') {
            dst[i] = '\0'; // trim padding
            break;
        }
    }
}

void JwtSigner::sign(const char* token, const char* qr_code, const char* carrier, float weight, uint32_t ts, char* out_buf, size_t out_sz) {
    // 1. Header
    const char* header = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
    char header_b64[64] = {0};
    base64url_encode((const unsigned char*)header, strlen(header), header_b64, sizeof(header_b64));

    // 2. Payload
    JsonDocument doc;
    doc["qr"] = qr_code;
    doc["carrier"] = carrier;
    doc["weight"] = weight;
    doc["ts"] = ts;
    // 'token' arg is ignored or can be added if needed, maybe as jti (JWT ID)
    if (token && strlen(token) > 0) {
        doc["jti"] = token;
    }
    
    char payload[256];
    serializeJson(doc, payload, sizeof(payload));
    
    char payload_b64[384] = {0};
    base64url_encode((const unsigned char*)payload, strlen(payload), payload_b64, sizeof(payload_b64));

    // 3. Signature Data
    char sig_data[512];
    snprintf(sig_data, sizeof(sig_data), "%s.%s", header_b64, payload_b64);

    // 4. HMAC-SHA256
    unsigned char hmacResult[32];
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
    mbedtls_md_hmac_starts(&ctx, (const unsigned char *)_active_secret, strlen(_active_secret));
    mbedtls_md_hmac_update(&ctx, (const unsigned char *)sig_data, strlen(sig_data));
    mbedtls_md_hmac_finish(&ctx, hmacResult);
    mbedtls_md_free(&ctx);

    // 5. Signature Base64URL
    char signature_b64[64] = {0};
    base64url_encode(hmacResult, 32, signature_b64, sizeof(signature_b64));

    // 6. Final JWT
    snprintf(out_buf, out_sz, "%s.%s.%s", header_b64, payload_b64, signature_b64);
}

void JwtSigner::rotateSecret(const char* new_secret) {
    strlcpy(_prev_secret, _active_secret, sizeof(_prev_secret));
    strlcpy(_active_secret, new_secret, sizeof(_active_secret));
}
