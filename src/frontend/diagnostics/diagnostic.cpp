// diagnostic.cpp — Diagnostic (error/warning) message system
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>

namespace raven {

enum class DiagnosticLevel { Error, Warning, Note, Info };

struct SourceLocation {
    std::string file;
    int line;
    int column;

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
            case DiagnosticLevel::Error:   oss << "error";   break;
            case DiagnosticLevel::Warning: oss << "warning"; break;
            case DiagnosticLevel::Note:    oss << "note";    break;
            case DiagnosticLevel::Info:    oss << "info";    break;
        }
        oss << ": " << location.to_string() << ": " << message;
        if (!source_line.empty()) {
            oss << "\n  " << source_line;
            if (location.column > 0) {
                oss << "\n  " << std::string(std::max(0, location.column - 1), ' ') << "^";
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

    void emit_error(const std::string& msg, SourceLocation loc) {
        emit({DiagnosticLevel::Error, msg, loc, ""});
    }

    void emit_warning(const std::string& msg, SourceLocation loc) {
        emit({DiagnosticLevel::Warning, msg, loc, ""});
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
