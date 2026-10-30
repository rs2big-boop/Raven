// verifier.cpp — Bytecode verification utilities for the Corvus VM
#include "raven/bytecode.hpp"
#include <string>
#include <vector>

namespace raven::vm {

struct VerificationResult {
    bool ok = true;
    std::string error_message;
};

VerificationResult verify_chunk_safety(const Chunk& chunk) {
    VerificationResult res;
    size_t i = 0;
    while (i < chunk.code.size()) {
        OpCode op = static_cast<OpCode>(chunk.code[i]);
        if (op == OpCode::Constant) {
            if (i + 1 >= chunk.code.size()) {
                res.ok = false;
                res.error_message = "Truncated Constant instruction";
                return res;
            }
            uint8_t idx = chunk.code[i + 1];
            if (idx >= chunk.constants.size()) {
                res.ok = false;
                res.error_message = "Constant index out of bounds: " + std::to_string(idx);
                return res;
            }
            i += 2;
        } else if (op == OpCode::Jump || op == OpCode::JumpIfFalse || op == OpCode::JumpIfNotNil) {
            if (i + 2 >= chunk.code.size()) {
                res.ok = false;
                res.error_message = "Truncated Jump instruction";
                return res;
            }
            i += 3;
        } else if (op == OpCode::Loop) {
            if (i + 2 >= chunk.code.size()) {
                res.ok = false;
                res.error_message = "Truncated Loop instruction";
                return res;
            }
            i += 3;
        } else if (op == OpCode::GetLocal || op == OpCode::SetLocal ||
                   op == OpCode::GetGlobal || op == OpCode::SetGlobal ||
                   op == OpCode::DefineGlobal || op == OpCode::Call ||
                   op == OpCode::BuildArray || op == OpCode::BuildMap ||
                   op == OpCode::CheckType) {
            if (i + 1 >= chunk.code.size()) {
                res.ok = false;
                res.error_message = "Instruction truncated";
                return res;
            }
            i += 2;
        } else {
            i += 1;
        }
    }
    return res;
}

} // namespace raven::vm
