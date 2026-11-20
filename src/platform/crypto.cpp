#include "raven/platform/crypto.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/crypto.h>

#include <array>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <memory>
#include <optional>
#include <climits>

namespace raven::platform {

namespace {

struct EvpMdCtxDeleter {
    void operator()(EVP_MD_CTX* ctx) const {
        if (ctx) EVP_MD_CTX_free(ctx);
    }
};
using EvpMdCtxPtr = std::unique_ptr<EVP_MD_CTX, EvpMdCtxDeleter>;

struct EvpCipherCtxDeleter {
    void operator()(EVP_CIPHER_CTX* ctx) const {
        if (ctx) EVP_CIPHER_CTX_free(ctx);
    }
};
using EvpCipherCtxPtr = std::unique_ptr<EVP_CIPHER_CTX, EvpCipherCtxDeleter>;

static std::string bytes_to_hex(const uint8_t* data, size_t len) {
    if (!data || len == 0) return "";
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) {
        oss << std::setw(2) << static_cast<int>(data[i]);
    }
    return oss.str();
}

static std::string bytes_to_hex(const std::vector<uint8_t>& bytes) {
    return bytes_to_hex(bytes.data(), bytes.size());
}

static std::optional<std::vector<uint8_t>> hex_to_bytes(const std::string& hex) {
    if (hex.size() % 2 != 0) return std::nullopt;
    std::vector<uint8_t> bytes;
    bytes.reserve(hex.size() / 2);
    auto hex_nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    for (size_t i = 0; i < hex.size(); i += 2) {
        int hi = hex_nibble(hex[i]);
        int lo = hex_nibble(hex[i + 1]);
        if (hi < 0 || lo < 0) return std::nullopt;
        bytes.push_back(static_cast<uint8_t>((hi << 4) | lo));
    }
    return bytes;
}

static std::optional<std::vector<uint8_t>> parse_aes_key(const std::string& key) {
    // AES-256 requires exactly 32 bytes (256 bits).
    if (key.size() == 64) {
        auto parsed = hex_to_bytes(key);
        if (parsed && parsed->size() == 32) return parsed;
        return std::nullopt;
    }
    if (key.size() == 32) {
        return std::vector<uint8_t>(key.begin(), key.end());
    }
    return std::nullopt;
}

static const std::string BASE64_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string base64_encode(const std::string& input) {
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(BASE64_CHARS[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(BASE64_CHARS[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

} // anonymous namespace

// --- Constant-Time Comparison ---
bool constant_time_compare(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) {
        return false;
    }
    return CRYPTO_memcmp(a.data(), b.data(), a.size()) == 0;
}

// --- CSPRNG ---
std::vector<uint8_t> crypto_random_bytes(size_t count) {
    if (count == 0) return {};
    if (count > INT_MAX) return {};
    std::vector<uint8_t> buf(count, 0);
    int rc = RAND_bytes(buf.data(), static_cast<int>(count));
    if (rc != 1) {
        OPENSSL_cleanse(buf.data(), buf.size());
        return {};
    }
    return buf;
}

std::string crypto_random_hex(size_t count) {
    auto bytes = crypto_random_bytes(count);
    if (bytes.empty() && count > 0) return "";
    return bytes_to_hex(bytes);
}

// --- SHA-256 ---
std::vector<uint8_t> sha256_raw(const uint8_t* data, size_t len) {
    std::vector<uint8_t> digest(32, 0);
    unsigned int out_len = 0;
    EvpMdCtxPtr ctx(EVP_MD_CTX_new());
    if (!ctx) return {};
    if (EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr) != 1) return {};
    if (len > 0 && EVP_DigestUpdate(ctx.get(), data, len) != 1) return {};
    if (EVP_DigestFinal_ex(ctx.get(), digest.data(), &out_len) != 1) return {};
    digest.resize(out_len);
    return digest;
}

std::string sha256_hex(const std::string& input) {
    auto raw = sha256_raw(reinterpret_cast<const uint8_t*>(input.data()), input.size());
    return bytes_to_hex(raw);
}

// --- HMAC-SHA256 ---
std::vector<uint8_t> hmac_sha256_raw(const uint8_t* key, size_t key_len, const uint8_t* data, size_t data_len) {
    if (key_len > INT_MAX || data_len > INT_MAX) return {};
    std::vector<uint8_t> out(32, 0);
    unsigned int out_len = 0;
    unsigned char* res = HMAC(EVP_sha256(), key, static_cast<int>(key_len), data, data_len, out.data(), &out_len);
    if (!res) {
        OPENSSL_cleanse(out.data(), out.size());
        return {};
    }
    out.resize(out_len);
    return out;
}

std::string hmac_sha256_hex(const std::string& key, const std::string& data) {
    auto raw = hmac_sha256_raw(reinterpret_cast<const uint8_t*>(key.data()), key.size(),
                               reinterpret_cast<const uint8_t*>(data.data()), data.size());
    return bytes_to_hex(raw);
}

// --- SHA-512 ---
std::vector<uint8_t> sha512_raw(const uint8_t* data, size_t len) {
    std::vector<uint8_t> digest(64, 0);
    unsigned int out_len = 0;
    EvpMdCtxPtr ctx(EVP_MD_CTX_new());
    if (!ctx) return {};
    if (EVP_DigestInit_ex(ctx.get(), EVP_sha512(), nullptr) != 1) return {};
    if (len > 0 && EVP_DigestUpdate(ctx.get(), data, len) != 1) return {};
    if (EVP_DigestFinal_ex(ctx.get(), digest.data(), &out_len) != 1) return {};
    digest.resize(out_len);
    return digest;
}

std::string sha512_hex(const std::string& input) {
    auto raw = sha512_raw(reinterpret_cast<const uint8_t*>(input.data()), input.size());
    return bytes_to_hex(raw);
}

// --- HMAC-SHA512 ---
std::vector<uint8_t> hmac_sha512_raw(const uint8_t* key, size_t key_len, const uint8_t* data, size_t data_len) {
    if (key_len > INT_MAX || data_len > INT_MAX) return {};
    std::vector<uint8_t> out(64, 0);
    unsigned int out_len = 0;
    unsigned char* res = HMAC(EVP_sha512(), key, static_cast<int>(key_len), data, data_len, out.data(), &out_len);
    if (!res) {
        OPENSSL_cleanse(out.data(), out.size());
        return {};
    }
    out.resize(out_len);
    return out;
}

std::string hmac_sha512_hex(const std::string& key, const std::string& data) {
    auto raw = hmac_sha512_raw(reinterpret_cast<const uint8_t*>(key.data()), key.size(),
                               reinterpret_cast<const uint8_t*>(data.data()), data.size());
    return bytes_to_hex(raw);
}

// --- SHA-1 & WebSocket Helpers ---
std::string sha1_raw(const std::string& input) {
    unsigned char digest[20];
    unsigned int out_len = 0;
    EvpMdCtxPtr ctx(EVP_MD_CTX_new());
    if (!ctx) return "";
    if (EVP_DigestInit_ex(ctx.get(), EVP_sha1(), nullptr) != 1) return "";
    if (!input.empty() && EVP_DigestUpdate(ctx.get(), input.data(), input.size()) != 1) return "";
    if (EVP_DigestFinal_ex(ctx.get(), digest, &out_len) != 1) return "";
    return std::string(reinterpret_cast<char*>(digest), out_len);
}

std::string sha1_hex(const std::string& input) {
    auto raw = sha1_raw(input);
    return bytes_to_hex(reinterpret_cast<const uint8_t*>(raw.data()), raw.size());
}

std::string websocket_accept_key(const std::string& client_key) {
    static const std::string WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string combined = client_key + WS_GUID;
    std::string hash = sha1_raw(combined);
    return base64_encode(hash);
}

// --- Legacy MD5 ---
std::string md5_hex(const std::string& input) {
    unsigned char digest[16];
    unsigned int out_len = 0;
    EvpMdCtxPtr ctx(EVP_MD_CTX_new());
    if (!ctx) return "";
    if (EVP_DigestInit_ex(ctx.get(), EVP_md5(), nullptr) != 1) return "";
    if (!input.empty() && EVP_DigestUpdate(ctx.get(), input.data(), input.size()) != 1) return "";
    if (EVP_DigestFinal_ex(ctx.get(), digest, &out_len) != 1) return "";
    return bytes_to_hex(digest, out_len);
}

// --- PBKDF2-HMAC-SHA256 ---
std::vector<uint8_t> pbkdf2_hmac_sha256_raw(const std::string& password, const std::string& salt, int iterations, size_t key_len) {
    if (key_len == 0 || iterations < 1 || key_len > INT_MAX) return {};
    std::vector<uint8_t> key(key_len, 0);
    int rc = PKCS5_PBKDF2_HMAC(
        reinterpret_cast<const char*>(password.data()),
        static_cast<int>(password.size()),
        reinterpret_cast<const unsigned char*>(salt.data()),
        static_cast<int>(salt.size()),
        iterations,
        EVP_sha256(),
        static_cast<int>(key_len),
        key.data()
    );
    if (rc != 1) {
        OPENSSL_cleanse(key.data(), key.size());
        return {};
    }
    return key;
}

std::string pbkdf2_hmac_sha256_hex(const std::string& password, const std::string& salt, int iterations, size_t key_len) {
    auto raw = pbkdf2_hmac_sha256_raw(password, salt, iterations, key_len);
    std::string hex_str = bytes_to_hex(raw);
    OPENSSL_cleanse(raw.data(), raw.size());
    return hex_str;
}

// --- HKDF-SHA256 ---
std::vector<uint8_t> hkdf_sha256_raw(const std::string& ikm, const std::string& salt, const std::string& info, size_t length) {
    // RFC 5869 Section 2.3: length <= 255 * HashLen (for SHA-256: 255 * 32 = 8160 bytes)
    const size_t hlen = 32;
    if (length == 0 || length > 255 * hlen) {
        return {};
    }

    std::string eff_salt = salt.empty() ? std::string(hlen, '\0') : salt;
    auto prk = hmac_sha256_raw(
        reinterpret_cast<const uint8_t*>(eff_salt.data()), eff_salt.size(),
        reinterpret_cast<const uint8_t*>(ikm.data()), ikm.size()
    );
    if (prk.empty()) return {};

    size_t n = (length + hlen - 1) / hlen;
    std::vector<uint8_t> okm;
    okm.reserve(n * hlen);
    std::vector<uint8_t> t;

    for (size_t i = 1; i <= n; ++i) {
        std::vector<uint8_t> msg = t;
        msg.insert(msg.end(), info.begin(), info.end());
        msg.push_back(static_cast<uint8_t>(i));
        t = hmac_sha256_raw(prk.data(), prk.size(), msg.data(), msg.size());
        if (t.empty()) {
            OPENSSL_cleanse(prk.data(), prk.size());
            OPENSSL_cleanse(okm.data(), okm.size());
            return {};
        }
        okm.insert(okm.end(), t.begin(), t.end());
    }

    OPENSSL_cleanse(prk.data(), prk.size());
    OPENSSL_cleanse(t.data(), t.size());
    okm.resize(length);
    return okm;
}

std::string hkdf_sha256_hex(const std::string& ikm, const std::string& salt, const std::string& info, size_t length) {
    auto raw = hkdf_sha256_raw(ikm, salt, info, length);
    std::string hex_str = bytes_to_hex(raw);
    OPENSSL_cleanse(raw.data(), raw.size());
    return hex_str;
}

// --- AES-256 CTR Mode (Legacy) ---
std::string aes256_ctr_encrypt(const std::string& key, const std::string& iv, const std::string& plaintext) {
    auto k_opt = parse_aes_key(key);
    if (!k_opt) return "";
    auto k_bytes = std::move(*k_opt);

    std::vector<uint8_t> iv_bytes;
    if (iv.size() == 32) {
        auto iv_opt = hex_to_bytes(iv);
        if (!iv_opt || iv_opt->size() != 16) {
            OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
            return "";
        }
        iv_bytes = std::move(*iv_opt);
    } else if (iv.size() == 16) {
        iv_bytes.assign(iv.begin(), iv.end());
    } else {
        OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
        return "";
    }

    EvpCipherCtxPtr ctx(EVP_CIPHER_CTX_new());
    if (!ctx) {
        OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
        return "";
    }
    if (EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_ctr(), nullptr, k_bytes.data(), iv_bytes.data()) != 1) {
        OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
        return "";
    }

    std::vector<uint8_t> out(plaintext.size() + 16, 0);
    int len1 = 0, len2 = 0;
    if (!plaintext.empty()) {
        if (EVP_EncryptUpdate(ctx.get(), out.data(), &len1,
                              reinterpret_cast<const uint8_t*>(plaintext.data()),
                              static_cast<int>(plaintext.size())) != 1) {
            OPENSSL_cleanse(out.data(), out.size());
            OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
            return "";
        }
    }
    if (EVP_EncryptFinal_ex(ctx.get(), out.data() + len1, &len2) != 1) {
        OPENSSL_cleanse(out.data(), out.size());
        OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
        return "";
    }
    out.resize(len1 + len2);
    std::string hex_res = bytes_to_hex(out);
    OPENSSL_cleanse(out.data(), out.size());
    OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
    return hex_res;
}

std::string aes256_ctr_decrypt(const std::string& key, const std::string& iv, const std::string& ciphertext_hex) {
    auto k_opt = parse_aes_key(key);
    if (!k_opt) return "";
    auto k_bytes = std::move(*k_opt);

    std::vector<uint8_t> iv_bytes;
    if (iv.size() == 32) {
        auto iv_opt = hex_to_bytes(iv);
        if (!iv_opt || iv_opt->size() != 16) {
            OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
            return "";
        }
        iv_bytes = std::move(*iv_opt);
    } else if (iv.size() == 16) {
        iv_bytes.assign(iv.begin(), iv.end());
    } else {
        OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
        return "";
    }

    auto ct_opt = hex_to_bytes(ciphertext_hex);
    if (!ct_opt) {
        OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
        return "";
    }
    auto ct_bytes = std::move(*ct_opt);

    EvpCipherCtxPtr ctx(EVP_CIPHER_CTX_new());
    if (!ctx) {
        OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
        return "";
    }
    if (EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_ctr(), nullptr, k_bytes.data(), iv_bytes.data()) != 1) {
        OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
        return "";
    }

    std::vector<uint8_t> out(ct_bytes.size() + 16, 0);
    int len1 = 0, len2 = 0;
    if (!ct_bytes.empty()) {
        if (EVP_DecryptUpdate(ctx.get(), out.data(), &len1, ct_bytes.data(), static_cast<int>(ct_bytes.size())) != 1) {
            OPENSSL_cleanse(out.data(), out.size());
            OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
            return "";
        }
    }
    if (EVP_DecryptFinal_ex(ctx.get(), out.data() + len1, &len2) != 1) {
        OPENSSL_cleanse(out.data(), out.size());
        OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
        return "";
    }
    std::string pt(reinterpret_cast<char*>(out.data()), len1 + len2);
    OPENSSL_cleanse(out.data(), out.size());
    OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
    return pt;
}

// --- AES-256-GCM Authenticated Encryption ---
GcmResult aes256_gcm_encrypt(const std::string& key, const std::string& plaintext, const std::string& aad) {
    GcmResult res;
    auto k_opt = parse_aes_key(key);
    if (!k_opt) return res;
    auto k_bytes = std::move(*k_opt);

    auto iv_bytes = crypto_random_bytes(12);
    if (iv_bytes.size() != 12) {
        OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
        return res;
    }

    EvpCipherCtxPtr ctx(EVP_CIPHER_CTX_new());
    if (!ctx) {
        OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
        return res;
    }

    if (EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) != 1 ||
        EVP_EncryptInit_ex(ctx.get(), nullptr, nullptr, k_bytes.data(), iv_bytes.data()) != 1) {
        OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
        return res;
    }

    if (!aad.empty()) {
        int unused = 0;
        if (EVP_EncryptUpdate(ctx.get(), nullptr, &unused,
                              reinterpret_cast<const uint8_t*>(aad.data()),
                              static_cast<int>(aad.size())) != 1) {
            OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
            return res;
        }
    }

    std::vector<uint8_t> ct(plaintext.size() + 16, 0);
    int len1 = 0, len2 = 0;
    if (!plaintext.empty()) {
        if (EVP_EncryptUpdate(ctx.get(), ct.data(), &len1,
                              reinterpret_cast<const uint8_t*>(plaintext.data()),
                              static_cast<int>(plaintext.size())) != 1) {
            OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
            OPENSSL_cleanse(ct.data(), ct.size());
            return res;
        }
    }

    if (EVP_EncryptFinal_ex(ctx.get(), ct.data() + len1, &len2) != 1) {
        OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
        OPENSSL_cleanse(ct.data(), ct.size());
        return res;
    }

    uint8_t tag[16];
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, 16, tag) != 1) {
        OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
        OPENSSL_cleanse(ct.data(), ct.size());
        return res;
    }

    OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
    ct.resize(len1 + len2);
    res.success = true;
    res.iv_hex = bytes_to_hex(iv_bytes);
    res.ciphertext_hex = bytes_to_hex(ct);
    res.tag_hex = bytes_to_hex(tag, 16);
    return res;
}

GcmDecryptResult aes256_gcm_decrypt(
    const std::string& key,
    const std::string& iv_hex,
    const std::string& ciphertext_hex,
    const std::string& tag_hex,
    const std::string& aad
) {
    GcmDecryptResult res;
    auto k_opt = parse_aes_key(key);
    if (!k_opt) return res;
    auto k_bytes = std::move(*k_opt);

    auto iv_opt = hex_to_bytes(iv_hex);
    if (!iv_opt || iv_opt->size() != 12) {
        OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
        return res;
    }
    auto iv_bytes = std::move(*iv_opt);

    auto tag_opt = hex_to_bytes(tag_hex);
    if (!tag_opt || tag_opt->size() != 16) {
        OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
        return res;
    }
    auto tag_bytes = std::move(*tag_opt);

    auto ct_opt = hex_to_bytes(ciphertext_hex);
    if (!ct_opt) {
        OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
        return res;
    }
    auto ct_bytes = std::move(*ct_opt);

    EvpCipherCtxPtr ctx(EVP_CIPHER_CTX_new());
    if (!ctx) {
        OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
        return res;
    }

    if (EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) != 1 ||
        EVP_DecryptInit_ex(ctx.get(), nullptr, nullptr, k_bytes.data(), iv_bytes.data()) != 1) {
        OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
        return res;
    }

    if (!aad.empty()) {
        int unused = 0;
        if (EVP_DecryptUpdate(ctx.get(), nullptr, &unused,
                              reinterpret_cast<const uint8_t*>(aad.data()),
                              static_cast<int>(aad.size())) != 1) {
            OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
            return res;
        }
    }

    std::vector<uint8_t> pt(ct_bytes.size() + 16, 0);
    int len1 = 0, len2 = 0;
    if (!ct_bytes.empty()) {
        if (EVP_DecryptUpdate(ctx.get(), pt.data(), &len1, ct_bytes.data(), static_cast<int>(ct_bytes.size())) != 1) {
            OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
            OPENSSL_cleanse(pt.data(), pt.size());
            return res;
        }
    }

    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, 16, tag_bytes.data()) != 1) {
        OPENSSL_cleanse(k_bytes.data(), k_bytes.size());
        OPENSSL_cleanse(pt.data(), pt.size());
        return res;
    }

    int ok = EVP_DecryptFinal_ex(ctx.get(), pt.data() + len1, &len2);
    OPENSSL_cleanse(k_bytes.data(), k_bytes.size());

    if (ok <= 0) {
        OPENSSL_cleanse(pt.data(), pt.size());
        return res; // Authentication failed or ciphertext tampered!
    }

    res.success = true;
    res.plaintext = std::string(reinterpret_cast<char*>(pt.data()), len1 + len2);
    OPENSSL_cleanse(pt.data(), pt.size());
    return res;
}

AuthCipherResult aes256_authenticated_encrypt(const std::string& key_hex, const std::string& plaintext) {
    auto gcm = aes256_gcm_encrypt(key_hex, plaintext);
    AuthCipherResult res;
    res.success = gcm.success;
    res.iv_hex = gcm.iv_hex;
    res.ciphertext_hex = gcm.ciphertext_hex;
    res.tag_hex = gcm.tag_hex;
    return res;
}

GcmDecryptResult aes256_authenticated_decrypt(const std::string& key_hex, const std::string& iv_hex, const std::string& ciphertext_hex, const std::string& tag_hex) {
    return aes256_gcm_decrypt(key_hex, iv_hex, ciphertext_hex, tag_hex);
}

// --- WebSocket Frame Framing & Parsing (RFC 6455) ---
std::string websocket_create_frame(int opcode, const std::string& payload) {
    std::string frame;
    frame.reserve(10 + payload.size());

    uint8_t byte0 = 0x80 | (opcode & 0x0F);
    frame.push_back(static_cast<char>(byte0));

    size_t len = payload.size();
    if (len <= 125) {
        frame.push_back(static_cast<char>(len));
    } else if (len <= 65535) {
        frame.push_back(static_cast<char>(126));
        frame.push_back(static_cast<char>((len >> 8) & 0xFF));
        frame.push_back(static_cast<char>(len & 0xFF));
    } else {
        frame.push_back(static_cast<char>(127));
        for (int i = 7; i >= 0; --i) {
            frame.push_back(static_cast<char>((len >> (i * 8)) & 0xFF));
        }
    }

    frame.append(payload);
    return frame;
}

WebSocketFrame websocket_parse_frame(const std::string& buffer) {
    WebSocketFrame res;
    if (buffer.size() < 2) {
        res.need_more = true;
        return res;
    }

    uint8_t byte0 = static_cast<uint8_t>(buffer[0]);
    uint8_t byte1 = static_cast<uint8_t>(buffer[1]);

    res.fin = (byte0 & 0x80) != 0;
    res.opcode = (byte0 & 0x0F);
    bool masked = (byte1 & 0x80) != 0;
    uint64_t payload_len = (byte1 & 0x7F);

    size_t header_len = 2;
    if (payload_len == 126) {
        if (buffer.size() < 4) {
            res.need_more = true;
            return res;
        }
        payload_len = (static_cast<uint64_t>(static_cast<uint8_t>(buffer[2])) << 8) |
                      static_cast<uint64_t>(static_cast<uint8_t>(buffer[3]));
        header_len = 4;
    } else if (payload_len == 127) {
        if (buffer.size() < 10) {
            res.need_more = true;
            return res;
        }
        payload_len = 0;
        for (int i = 0; i < 8; ++i) {
            payload_len = (payload_len << 8) | static_cast<uint8_t>(buffer[2 + i]);
        }
        header_len = 10;
    }

    size_t mask_len = masked ? 4 : 0;
    if (buffer.size() < header_len + mask_len + payload_len) {
        res.need_more = true;
        return res;
    }

    uint8_t mask[4] = {0, 0, 0, 0};
    if (masked) {
        for (int i = 0; i < 4; ++i) {
            mask[i] = static_cast<uint8_t>(buffer[header_len + i]);
        }
    }

    res.payload.resize(payload_len);
    size_t data_start = header_len + mask_len;
    for (size_t i = 0; i < payload_len; ++i) {
        uint8_t byte = static_cast<uint8_t>(buffer[data_start + i]);
        if (masked) {
            byte ^= mask[i % 4];
        }
        res.payload[i] = static_cast<char>(byte);
    }

    res.valid = true;
    res.consumed = header_len + mask_len + payload_len;
    return res;
}

} // namespace raven::platform
