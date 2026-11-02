#include "raven/compiler.hpp"

namespace raven {

// Bytecode Generator helper interface
bool generate_bytecode(const std::vector<std::shared_ptr<Stmt>>& statements, Chunk& chunk) {
    Compiler compiler(chunk);
    return compiler.compile(statements);
}

} // namespace raven
