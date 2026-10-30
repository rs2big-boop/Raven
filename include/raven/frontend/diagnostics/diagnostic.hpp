#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>

namespace raven {

enum class DiagnosticLevel { Error, Warning, Note, Info };

struct SourceLocation {
    std::string file;
    int line = 1;
    int column = 1;

    std::string to_string() const {
        return file + ":" + std::to_string(line) + ":" + std::to_string(column);
    }
};

struct Diagnostic {
    DiagnosticLevel level;
    std::string message;
    SourceLocation location;
    std::string source_line;

    std::string format() const {
        std::ostringstream oss;
        switch (level) {
            case DiagnosticLevel::Error:   oss << "\033[1;31merror\033[0m";   break;
            case DiagnosticLevel::Warning: oss << "\033[1;33mwarning\033[0m"; break;
            case DiagnosticLevel::Note:    oss << "\033[1;36mnote\033[0m";    break;
            case DiagnosticLevel::Info:    oss << "\033[1;32minfo\033[0m";    break;
        }
        oss << ": " << location.to_string() << ": " << message;
        if (!source_line.empty()) {
            oss << "\n  " << source_line;
            if (location.column > 0) {
                oss << "\n  \033[1;31m" << std::string(std::max(0, location.column - 1), ' ') << "^\033[0m";
            }
        }
        return oss.str();
    }

    bool is_error() const { return level == DiagnosticLevel::Error; }
};

class DiagnosticEngine {
public:
    void emit(Diagnostic diag) {
        has_errors_ |= diag.is_error();
        diagnostics_.push_back(std::move(diag));
    }

    void emit_error(const std::string& msg, SourceLocation loc, const std::string& line = "") {
        emit({DiagnosticLevel::Error, msg, loc, line});
    }

    void emit_warning(const std::string& msg, SourceLocation loc, const std::string& line = "") {
        emit({DiagnosticLevel::Warning, msg, loc, line});
    }

    bool has_errors() const { return has_errors_; }
    const std::vector<Diagnostic>& all() const { return diagnostics_; }
    size_t count() const { return diagnostics_.size(); }

    void print_all(std::ostream& out = std::cerr) const {
        for (const auto& d : diagnostics_) {
            out << d.format() << "\n";
        }
    }

    void clear() {
        diagnostics_.clear();
        has_errors_ = false;
    }

private:
    std::vector<Diagnostic> diagnostics_;
    bool has_errors_ = false;
};

} // namespace raven
