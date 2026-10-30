#include "raven/semantic/analyzer.hpp"

namespace raven::semantic {

bool Analyzer::analyze(const std::vector<std::shared_ptr<Stmt>>& program) {
    return resolver_.resolve(program);
}

void Analyzer::print_errors(std::ostream& out) const {
    for (const auto& err : resolver_.errors()) {
        out << "Semantic error";
        if (err.line > 0) out << " [line " << err.line << "]";
        out << ": " << err.message << "\n";
    }
}

} // namespace raven::semantic
