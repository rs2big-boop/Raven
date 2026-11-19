#pragma once
#include <string>
#include <cstdint>
#include <vector>

namespace raven::platform {

// --- SHA-256 & HMAC-SHA256 (FIPS 180-4 / RFC 4231) ---
std::string sha256_hex(const std::string& input);
std::vector<uint8_t> sha256_raw(const uint8_t* data, size_t len);
std::string hmac_sha256_hex(const std::string& key, const std::string& data);
std::vector<uint8_t> hmac_sha256_raw(const uint8_t* key, size_t key_len, const uint8_t* data, size_t data_len);

// --- SHA-512 & HMAC-SHA512 (FIPS 180-4 / RFC 6234 / RFC 4231) ---
std::string sha512_hex(const std::string& input);
std::vector<uint8_t> sha512_raw(const uint8_t* data, size_t len);
std::string hmac_sha512_hex(const std::string& key, const std::string& data);
std::vector<uint8_t> hmac_sha512_raw(const uint8_t* key, size_t key_len, const uint8_t* data, size_t data_len);

// --- SHA-1 & WebSocket RFC 6455 Cryptographic Helpers ---
std::string sha1_raw(const std::string& input);
std::string sha1_hex(const std::string& input);
std::string websocket_accept_key(const std::string& client_key);

// --- Legacy MD5 (non-cryptographic / compatibility only) ---
std::string md5_hex(const std::string& input);

// --- Timing-Safe Equality Comparison (Constant-time to prevent side-channel attacks) ---
bool constant_time_compare(const std::string& a, const std::string& b);

// --- Cryptographically Secure Pseudo-Random Number Generator (CSPRNG) ---
std::vector<uint8_t> crypto_random_bytes(size_t count);
std::string crypto_random_hex(size_t count);

// --- PBKDF2 Key Derivation Function (RFC 2898 / RFC 8018 / RFC 6070) ---
std::string pbkdf2_hmac_sha256_hex(const std::string& password, const std::string& salt, int iterations, size_t key_len = 32);
std::vector<uint8_t> pbkdf2_hmac_sha256_raw(const std::string& password, const std::string& salt, int iterations, size_t key_len = 32);

// --- HKDF Extract-and-Expand Key Derivation Function (RFC 5869) ---
std::string hkdf_sha256_hex(const std::string& ikm, const std::string& salt, const std::string& info, size_t length);
std::vector<uint8_t> hkdf_sha256_raw(const std::string& ikm, const std::string& salt, const std::string& info, size_t length);

// --- AES-256 Symmetric Encryption (FIPS 197 / CTR Mode) ---
std::string aes256_ctr_encrypt(const std::string& key, const std::string& iv, const std::string& plaintext);
std::string aes256_ctr_decrypt(const std::string& key, const std::string& iv, const std::string& ciphertext);

// --- AES-256-GCM Authenticated Encryption (NIST SP 800-38D) ---
struct GcmResult {
    bool success = false;
    std::string iv_hex;
    std::string ciphertext_hex;
    std::string tag_hex;
};

GcmResult aes256_gcm_encrypt(const std::string& key, const std::string& plaintext, const std::string& aad = "");
std::string aes256_gcm_decrypt(const std::string& key, const std::string& iv, const std::string& ciphertext, const std::string& tag, const std::string& aad = "");

// --- Authenticated AES-256 (CTR + HMAC-SHA256 Encrypt-then-MAC with Constant-Time Tag Verification) ---
struct AuthCipherResult {
    bool success = false;
    std::string iv_hex;
    std::string ciphertext_hex;
    std::string tag_hex;
};

AuthCipherResult aes256_authenticated_encrypt(const std::string& key_hex, const std::string& plaintext);
std::string aes256_authenticated_decrypt(const std::string& key_hex, const std::string& iv_hex, const std::string& ciphertext_hex, const std::string& tag_hex);

// --- WebSocket RFC 6455 Frame Framing & Parsing ---
struct WebSocketFrame {
    bool valid = false;
    bool need_more = false;
    bool fin = false;
    int opcode = 0;
    std::string payload;
    size_t consumed = 0;
};

std::string websocket_create_frame(int opcode, const std::string& payload);
WebSocketFrame websocket_parse_frame(const std::string& buffer);

} // namespace raven::platform
