#pragma once

#include <string>
#include <vector>
#include "raven/bytecode.hpp"

namespace raven {

struct VerificationResult {
    bool is_valid = true;
    std::string error_message;
};

class BytecodeVerifier {
public:
    static VerificationResult verify(const Chunk& chunk) {
        const auto& code = chunk.code;
        const auto& constants = chunk.constants;
        size_t offset = 0;

        while (offset < code.size()) {
            uint8_t raw_op = code[offset];
            if (raw_op > static_cast<uint8_t>(OpCode::Return)) {
                return {false, "Invalid opcode byte " + std::to_string(static_cast<int>(raw_op)) + " at offset " + std::to_string(offset)};
            }

            OpCode op = static_cast<OpCode>(raw_op);
            switch (op) {
                case OpCode::Constant:
                case OpCode::DefineGlobal:
                case OpCode::GetGlobal:
                case OpCode::SetGlobal: {
                    if (offset + 1 >= code.size()) {
                        return {false, "Truncated constant operand at offset " + std::to_string(offset)};
                    }
                    uint8_t const_idx = code[offset + 1];
                    if (const_idx >= constants.size()) {
                        return {false, "Constant index " + std::to_string(const_idx) + " out of bounds (pool size: " + std::to_string(constants.size()) + ") at offset " + std::to_string(offset)};
                    }
                    offset += 2;
                    break;
                }
                case OpCode::GetLocal:
                case OpCode::SetLocal:
                case OpCode::BuildArray:
                case OpCode::Call: {
                    if (offset + 1 >= code.size()) {
                        return {false, "Truncated operand at offset " + std::to_string(offset)};
                    }
                    offset += 2;
                    break;
                }
                case OpCode::Jump:
                case OpCode::JumpIfFalse: {
                    if (offset + 2 >= code.size()) {
                        return {false, "Truncated jump offset at offset " + std::to_string(offset)};
                    }
                    uint16_t jump = (static_cast<uint16_t>(code[offset + 1]) << 8) | code[offset + 2];
                    size_t target = offset + 3 + jump;
                    if (target > code.size()) {
                        return {false, "Jump target " + std::to_string(target) + " exceeds code bounds at offset " + std::to_string(offset)};
                    }
                    offset += 3;
                    break;
                }
                case OpCode::Loop: {
                    if (offset + 2 >= code.size()) {
                        return {false, "Truncated loop offset at offset " + std::to_string(offset)};
                    }
                    uint16_t jump = (static_cast<uint16_t>(code[offset + 1]) << 8) | code[offset + 2];
                    if (offset + 3 < jump) {
                        return {false, "Loop underflow target at offset " + std::to_string(offset)};
                    }
                    offset += 3;
                    break;
                }
                default: {
                    offset += 1;
                    break;
                }
            }
        }

        return {true, ""};
    }
};

} // namespace raven
