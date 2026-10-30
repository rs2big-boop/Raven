// parser_error.cpp — Parser error formatting and recovery utilities
#include "raven/parser.hpp"
#include <sstream>
#include <algorithm>

namespace raven {

std::string format_parse_error(const std::string& message,
                               const std::string& source,
                               int line, int column) {
    std::ostringstream oss;
    oss << "[line " << line << ", col " << column << "] Parse error: " << message;

    if (!source.empty()) {
        std::istringstream src_stream(source);
        std::string cur_line;
        int cur = 1;
        while (std::getline(src_stream, cur_line) && cur < line) cur++;
        if (cur == line && !cur_line.empty()) {
            oss << "\n  " << cur_line;
            oss << "\n  " << std::string(std::max(0, column - 1), ' ') << "^";
        }
    }
    return oss.str();
}

} // namespace raven
