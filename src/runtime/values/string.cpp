#include "raven/value.hpp"
#include <algorithm>

namespace raven::runtime {

std::string string_concat(const std::string& a, const std::string& b) {
    return a + b;
}

std::string string_slice(const std::string& str, size_t start, size_t end) {
    if (start >= str.size()) return "";
    if (end > str.size()) end = str.size();
    if (end <= start) return "";
    return str.substr(start, end - start);
}

bool string_has_prefix(const std::string& str, const std::string& prefix) {
    if (str.size() < prefix.size()) return false;
    return str.compare(0, prefix.size(), prefix) == 0;
}

bool string_has_suffix(const std::string& str, const std::string& suffix) {
    if (str.size() < suffix.size()) return false;
    return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

} // namespace raven::runtime
