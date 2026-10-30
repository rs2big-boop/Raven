// source_location.cpp — Source location tracking
#include <string>
#include <vector>
#include <sstream>

namespace raven {

struct SourceSpan {
    int start_line;
    int start_column;
    int end_line;
    int end_column;

    bool is_single_line() const { return start_line == end_line; }

    std::string to_string() const {
        if (is_single_line()) {
            return std::to_string(start_line) + ":" +
                   std::to_string(start_column) + "-" +
                   std::to_string(end_column);
        }
        return std::to_string(start_line) + ":" + std::to_string(start_column) +
               " to " + std::to_string(end_line) + ":" + std::to_string(end_column);
    }
};

std::string extract_line(const std::string& source, int line_number) {
    std::istringstream stream(source);
    std::string line;
    int current = 1;
    while (std::getline(stream, line)) {
        if (current == line_number) return line;
        current++;
    }
    return "";
}

int count_lines(const std::string& source) {
    if (source.empty()) return 0;
    int count = 1;
    for (char c : source) if (c == '\n') count++;
    return count;
}

} // namespace raven
