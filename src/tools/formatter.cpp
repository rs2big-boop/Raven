#include "raven/formatter.hpp"
#include <sstream>
#include <vector>
#include <algorithm>

namespace raven {

static std::string trim_spaces(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::string Formatter::format(const std::string& source) {
    std::istringstream stream(source);
    std::string line;
    std::vector<std::string> raw_lines;

    while (std::getline(stream, line)) {
        raw_lines.push_back(line);
    }

    std::ostringstream out;
    int indent_level = 0;
    bool prev_empty = false;

    for (const auto& raw : raw_lines) {
        std::string trimmed = trim_spaces(raw);

        if (trimmed.empty()) {
            if (!prev_empty && out.tellp() > 0) {
                out << "\n";
                prev_empty = true;
            }
            continue;
        }
        prev_empty = false;

        // Count leading closing braces that should de-indent this line
        int closing_at_start = 0;
        size_t idx = 0;
        while (idx < trimmed.size() && (trimmed[idx] == '}' || trimmed[idx] == ']' || trimmed[idx] == ')')) {
            closing_at_start++;
            idx++;
        }

        int effective_indent = std::max(0, indent_level - closing_at_start);
        for (int i = 0; i < effective_indent; i++) {
            out << "    ";
        }

        out << trimmed << "\n";

        // Update indent_level for subsequent lines
        for (char ch : trimmed) {
            if (ch == '{') indent_level++;
            else if (ch == '}') indent_level = std::max(0, indent_level - 1);
        }
    }

    return out.str();
}

} // namespace raven
