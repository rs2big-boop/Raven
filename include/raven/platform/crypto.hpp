#pragma once
#include <string>
#include <cstdint>
#include <vector>

namespace raven::platform {

// Pure C++ cryptographic hash implementations (in-memory, no external processes)
std::string sha256_hex(const std::string& input);
std::string md5_hex(const std::string& input);
std::string hmac_sha256_hex(const std::string& key, const std::string& data);

} // namespace raven::platform
