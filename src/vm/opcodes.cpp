// opcodes.cpp — Opcode definitions and utilities for the Corvus VM
#include "raven/bytecode.hpp"
#include <string>

namespace raven::vm {

std::string opcode_name(OpCode op) {
    switch (op) {
        case OpCode::Constant:      return "CONSTANT";
        case OpCode::Nil:           return "NIL";
        case OpCode::True:          return "TRUE";
        case OpCode::False:         return "FALSE";
        case OpCode::Add:           return "ADD";
        case OpCode::Subtract:      return "SUBTRACT";
        case OpCode::Multiply:      return "MULTIPLY";
        case OpCode::Divide:        return "DIVIDE";
        case OpCode::Modulo:        return "MODULO";
        case OpCode::Equal:         return "EQUAL";
        case OpCode::NotEqual:      return "NOT_EQUAL";
        case OpCode::Greater:       return "GREATER";
        case OpCode::GreaterEqual:  return "GREATER_EQUAL";
        case OpCode::Less:          return "LESS";
        case OpCode::LessEqual:     return "LESS_EQUAL";
        case OpCode::Not:           return "NOT";
        case OpCode::Negate:        return "NEGATE";
        case OpCode::Print:         return "PRINT";
        case OpCode::Pop:           return "POP";
        case OpCode::DefineGlobal:  return "DEFINE_GLOBAL";
        case OpCode::GetGlobal:     return "GET_GLOBAL";
        case OpCode::SetGlobal:     return "SET_GLOBAL";
        case OpCode::GetLocal:      return "GET_LOCAL";
        case OpCode::SetLocal:      return "SET_LOCAL";
        case OpCode::JumpIfFalse:   return "JUMP_IF_FALSE";
        case OpCode::JumpIfNotNil:  return "JUMP_IF_NOT_NIL";
        case OpCode::Jump:          return "JUMP";
        case OpCode::Loop:          return "LOOP";
        case OpCode::BuildArray:    return "BUILD_ARRAY";
        case OpCode::BuildMap:      return "BUILD_MAP";
        case OpCode::IndexGet:      return "INDEX_GET";
        case OpCode::IndexSet:      return "INDEX_SET";
        case OpCode::Length:        return "LENGTH";
        case OpCode::CheckType:     return "CHECK_TYPE";
        case OpCode::Call:          return "CALL";
        case OpCode::Return:        return "RETURN";
        default:                    return "UNKNOWN";
    }
}

int opcode_operand_count(OpCode op) {
    switch (op) {
        case OpCode::Constant:
        case OpCode::DefineGlobal:
        case OpCode::GetGlobal:
        case OpCode::SetGlobal:
        case OpCode::GetLocal:
        case OpCode::SetLocal:
        case OpCode::BuildArray:
        case OpCode::BuildMap:
        case OpCode::CheckType:
        case OpCode::Call:
            return 1;

        case OpCode::JumpIfFalse:
        case OpCode::JumpIfNotNil:
        case OpCode::Jump:
        case OpCode::Loop:
            return 2;

        default:
            return 0;
    }
}

} // namespace raven::vm
