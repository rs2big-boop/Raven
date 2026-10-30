#include "raven/platform/crypto.hpp"
#include <array>
#include <iomanip>
#include <sstream>
#include <cstring>

namespace raven::platform {

namespace {

// --- SHA-256 implementation ---
inline uint32_t rotr(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
}

inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

inline uint32_t sig0(uint32_t x) {
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

inline uint32_t sig1(uint32_t x) {
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

inline uint32_t theta0(uint32_t x) {
    return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

inline uint32_t theta1(uint32_t x) {
    return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

// --- MD5 constants & functions ---
static const uint32_t S11 = 7, S12 = 12, S13 = 17, S14 = 22;
static const uint32_t S21 = 5, S22 = 9,  S23 = 14, S24 = 20;
static const uint32_t S31 = 4, S32 = 11, S33 = 16, S34 = 23;
static const uint32_t S41 = 6, S42 = 10, S43 = 15, S44 = 21;

inline uint32_t F_md5(uint32_t x, uint32_t y, uint32_t z) { return (x & y) | (~x & z); }
inline uint32_t G_md5(uint32_t x, uint32_t y, uint32_t z) { return (x & z) | (y & ~z); }
inline uint32_t H_md5(uint32_t x, uint32_t y, uint32_t z) { return x ^ y ^ z; }
inline uint32_t I_md5(uint32_t x, uint32_t y, uint32_t z) { return y ^ (x | ~z); }

inline uint32_t rotate_left(uint32_t x, uint32_t n) {
    return (x << n) | (x >> (32 - n));
}

inline void FF(uint32_t &a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
    a = rotate_left(a + F_md5(b, c, d) + x + ac, s) + b;
}
inline void GG(uint32_t &a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
    a = rotate_left(a + G_md5(b, c, d) + x + ac, s) + b;
}
inline void HH(uint32_t &a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
    a = rotate_left(a + H_md5(b, c, d) + x + ac, s) + b;
}
inline void II(uint32_t &a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
    a = rotate_left(a + I_md5(b, c, d) + x + ac, s) + b;
}

static std::vector<uint8_t> sha256_raw(const uint8_t* data, size_t len) {
    uint32_t h0 = 0x6a09e667;
    uint32_t h1 = 0xbb67ae85;
    uint32_t h2 = 0x3c6ef372;
    uint32_t h3 = 0xa54ff53a;
    uint32_t h4 = 0x510e527f;
    uint32_t h5 = 0x9b05688c;
    uint32_t h6 = 0x1f83d9ab;
    uint32_t h7 = 0x5be0cd19;

    uint64_t msg_len = len;
    uint64_t bit_len = msg_len * 8;

    std::vector<uint8_t> padded(data, data + len);
    padded.push_back(0x80);
    while ((padded.size() % 64) != 56) {
        padded.push_back(0x00);
    }
    for (int i = 7; i >= 0; --i) {
        padded.push_back(static_cast<uint8_t>((bit_len >> (i * 8)) & 0xff));
    }

    for (size_t chunk = 0; chunk < padded.size(); chunk += 64) {
        uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<uint32_t>(padded[chunk + i * 4]) << 24) |
                   (static_cast<uint32_t>(padded[chunk + i * 4 + 1]) << 16) |
                   (static_cast<uint32_t>(padded[chunk + i * 4 + 2]) << 8) |
                   (static_cast<uint32_t>(padded[chunk + i * 4 + 3]));
        }
        for (int i = 16; i < 64; ++i) {
            w[i] = theta1(w[i - 2]) + w[i - 7] + theta0(w[i - 15]) + w[i - 16];
        }

        uint32_t a = h0, b = h1, c = h2, d = h3, e = h4, f = h5, g = h6, h = h7;

        for (int i = 0; i < 64; ++i) {
            uint32_t t1 = h + sig1(e) + ch(e, f, g) + K[i] + w[i];
            uint32_t t2 = sig0(a) + maj(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }

        h0 += a; h1 += b; h2 += c; h3 += d;
        h4 += e; h5 += f; h6 += g; h7 += h;
    }

    std::vector<uint8_t> digest(32);
    uint32_t vals[8] = {h0, h1, h2, h3, h4, h5, h6, h7};
    for (int i = 0; i < 8; ++i) {
        digest[i * 4 + 0] = static_cast<uint8_t>((vals[i] >> 24) & 0xff);
        digest[i * 4 + 1] = static_cast<uint8_t>((vals[i] >> 16) & 0xff);
        digest[i * 4 + 2] = static_cast<uint8_t>((vals[i] >> 8) & 0xff);
        digest[i * 4 + 3] = static_cast<uint8_t>(vals[i] & 0xff);
    }
    return digest;
}

} // namespace

std::string sha256_hex(const std::string& input) {
    auto raw = sha256_raw(reinterpret_cast<const uint8_t*>(input.data()), input.size());
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (uint8_t b : raw) {
        oss << std::setw(2) << static_cast<int>(b);
    }
    return oss.str();
}

std::string hmac_sha256_hex(const std::string& key, const std::string& data) {
    const size_t block_size = 64;
    std::vector<uint8_t> k(block_size, 0);

    if (key.size() > block_size) {
        auto key_hash = sha256_raw(reinterpret_cast<const uint8_t*>(key.data()), key.size());
        std::copy(key_hash.begin(), key_hash.end(), k.begin());
    } else {
        std::copy(key.begin(), key.end(), k.begin());
    }

    std::vector<uint8_t> ipad(block_size);
    std::vector<uint8_t> opad(block_size);
    for (size_t i = 0; i < block_size; ++i) {
        ipad[i] = k[i] ^ 0x36;
        opad[i] = k[i] ^ 0x5c;
    }

    std::vector<uint8_t> inner_msg = ipad;
    inner_msg.insert(inner_msg.end(), data.begin(), data.end());
    auto inner_hash = sha256_raw(inner_msg.data(), inner_msg.size());

    std::vector<uint8_t> outer_msg = opad;
    outer_msg.insert(outer_msg.end(), inner_hash.begin(), inner_hash.end());
    auto final_hash = sha256_raw(outer_msg.data(), outer_msg.size());

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (uint8_t b : final_hash) {
        oss << std::setw(2) << static_cast<int>(b);
    }
    return oss.str();
}

std::string md5_hex(const std::string& input) {
    uint32_t a = 0x67452301;
    uint32_t b = 0xefcdab89;
    uint32_t c = 0x98badcfe;
    uint32_t d = 0x10325476;

    uint64_t msg_len = input.size();
    uint64_t bit_len = msg_len * 8;

    std::vector<uint8_t> padded(input.begin(), input.end());
    padded.push_back(0x80);
    while ((padded.size() % 64) != 56) {
        padded.push_back(0x00);
    }
    for (int i = 0; i < 8; ++i) {
        padded.push_back(static_cast<uint8_t>((bit_len >> (i * 8)) & 0xff));
    }

    for (size_t chunk = 0; chunk < padded.size(); chunk += 64) {
        uint32_t x[16];
        for (int i = 0; i < 16; ++i) {
            x[i] = (static_cast<uint32_t>(padded[chunk + i * 4])) |
                   (static_cast<uint32_t>(padded[chunk + i * 4 + 1]) << 8) |
                   (static_cast<uint32_t>(padded[chunk + i * 4 + 2]) << 16) |
                   (static_cast<uint32_t>(padded[chunk + i * 4 + 3]) << 24);
        }

        uint32_t aa = a, bb = b, cc = c, dd = d;

        // Round 1
        FF(a, b, c, d, x[ 0], S11, 0xd76aa478);
        FF(d, a, b, c, x[ 1], S12, 0xe8c7b756);
        FF(c, d, a, b, x[ 2], S13, 0x242070db);
        FF(b, c, d, a, x[ 3], S14, 0xc1bdceee);
        FF(a, b, c, d, x[ 4], S11, 0xf57c0faf);
        FF(d, a, b, c, x[ 5], S12, 0x4787c62a);
        FF(c, d, a, b, x[ 6], S13, 0xa8304613);
        FF(b, c, d, a, x[ 7], S14, 0xfd469501);
        FF(a, b, c, d, x[ 8], S11, 0x698098d8);
        FF(d, a, b, c, x[ 9], S12, 0x8b44f7af);
        FF(c, d, a, b, x[10], S13, 0xffff5bb1);
        FF(b, c, d, a, x[11], S14, 0x895cd7be);
        FF(a, b, c, d, x[12], S11, 0x6b901122);
        FF(d, a, b, c, x[13], S12, 0xfd987193);
        FF(c, d, a, b, x[14], S13, 0xa679438e);
        FF(b, c, d, a, x[15], S14, 0x49b40821);

        // Round 2
        GG(a, b, c, d, x[ 1], S21, 0xf61e2562);
        GG(d, a, b, c, x[ 6], S22, 0xc040b340);
        GG(c, d, a, b, x[11], S23, 0x265e5a51);
        GG(b, c, d, a, x[ 0], S24, 0xe9b6c7aa);
        GG(a, b, c, d, x[ 5], S21, 0xd62f105d);
        GG(d, a, b, c, x[10], S22,  0x2441453);
        GG(c, d, a, b, x[15], S23, 0xd8a1e681);
        GG(b, c, d, a, x[ 4], S24, 0xe7d3fbc8);
        GG(a, b, c, d, x[ 9], S21, 0x21e1cde6);
        GG(d, a, b, c, x[14], S22, 0xc33707d6);
        GG(c, d, a, b, x[ 3], S23, 0xf4d50d87);
        GG(b, c, d, a, x[ 8], S24, 0x455a14ed);
        GG(a, b, c, d, x[13], S21, 0xa9e3e905);
        GG(d, a, b, c, x[ 2], S22, 0xfcefa3f8);
        GG(c, d, a, b, x[ 7], S23, 0x676f02d9);
        GG(b, c, d, a, x[12], S24, 0x8d2a4c8a);

        // Round 3
        HH(a, b, c, d, x[ 5], S31, 0xfffa3942);
        HH(d, a, b, c, x[ 8], S32, 0x8771f681);
        HH(c, d, a, b, x[11], S33, 0x6d9d6122);
        HH(b, c, d, a, x[14], S34, 0xfde5380c);
        HH(a, b, c, d, x[ 1], S31, 0xa4beea44);
        HH(d, a, b, c, x[ 4], S32, 0x4bdecfa9);
        HH(c, d, a, b, x[ 7], S33, 0xf6bb4b60);
        HH(b, c, d, a, x[10], S34, 0xbebfbc70);
        HH(a, b, c, d, x[13], S31, 0x289b7ec6);
        HH(d, a, b, c, x[ 0], S32, 0xeaa127fa);
        HH(c, d, a, b, x[ 3], S33, 0xd4ef3085);
        HH(b, c, d, a, x[ 6], S34,  0x4881d05);
        HH(a, b, c, d, x[ 9], S31, 0xd9d4d039);
        HH(d, a, b, c, x[12], S32, 0xe6db99e5);
        HH(c, d, a, b, x[15], S33, 0x1fa27cf8);
        HH(b, c, d, a, x[ 2], S34, 0xc4ac5665);

        // Round 4
        II(a, b, c, d, x[ 0], S41, 0xf4292244);
        II(d, a, b, c, x[ 7], S42, 0x432aff97);
        II(c, d, a, b, x[14], S43, 0xab9423a7);
        II(b, c, d, a, x[ 5], S44, 0xfc93a039);
        II(a, b, c, d, x[12], S41, 0x655b59c3);
        II(d, a, b, c, x[ 3], S42, 0x8f0ccc92);
        II(c, d, a, b, x[10], S43, 0xffeff47d);
        II(b, c, d, a, x[ 1], S44, 0x85845dd1);
        II(a, b, c, d, x[ 8], S41, 0x6fa87e4f);
        II(d, a, b, c, x[15], S42, 0xfe2ce6e0);
        II(c, d, a, b, x[ 6], S43, 0xa3014314);
        II(b, c, d, a, x[13], S44, 0x4e0811a1);
        II(a, b, c, d, x[ 4], S41, 0xf7537e82);
        II(d, a, b, c, x[11], S42, 0xbd3af235);
        II(c, d, a, b, x[ 2], S43, 0x2ad7d2bb);
        II(b, c, d, a, x[ 9], S44, 0xeb86d391);

        a += aa; b += bb; c += cc; d += dd;
    }

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    uint32_t vals[4] = {a, b, c, d};
    for (uint32_t v : vals) {
        for (int i = 0; i < 4; ++i) {
            oss << std::setw(2) << ((v >> (i * 8)) & 0xff);
        }
    }
    return oss.str();
}

} // namespace raven::platform
