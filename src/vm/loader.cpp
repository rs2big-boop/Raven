// loader.cpp — Bytecode chunk validation utilities
#include "raven/bytecode.hpp"

namespace raven::vm {

bool validate_chunk(const Chunk& chunk) {
    for (size_t i = 0; i < chunk.code.size(); i++) {
        OpCode op = static_cast<OpCode>(chunk.code[i]);
        if (op == OpCode::Constant) {
            if (i + 1 >= chunk.code.size()) return false;
            uint8_t idx = chunk.code[i + 1];
            if (idx >= chunk.constants.size()) return false;
            i += 1;
        }
    }
    return true;
}

} // namespace raven::vm
