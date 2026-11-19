#pragma once
#include <string>
#include <cstdint>
#include <vector>

namespace raven::platform {

// Pure C++ cryptographic hash implementations (in-memory, no external processes)
std::string sha256_hex(const std::string& input);
std::string md5_hex(const std::string& input);
std::string hmac_sha256_hex(const std::string& key, const std::string& data);

// SHA-1 & WebSocket RFC 6455 Cryptographic Helpers
std::string sha1_raw(const std::string& input);
std::string sha1_hex(const std::string& input);
std::string websocket_accept_key(const std::string& client_key);

// WebSocket RFC 6455 Frame Framing & Parsing
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
