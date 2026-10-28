#pragma once

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "raven/value.hpp"

namespace raven {

class BytecodeFunction : public Callable {
public:
    BytecodeFunction(std::string name, int arity, size_t entry_ip)
        : name_(std::move(name)), arity_(arity), entry_ip_(entry_ip) {}

    int arity() const override { return arity_; }
    size_t entry_ip() const { return entry_ip_; }
    const std::string& name() const { return name_; }

    Value call(Interpreter&, const std::vector<Value>&) override {
        return Value();
    }

    std::string to_string() const override {
        return "<vm fn " + name_ + ">";
    }

private:
    std::string name_;
    int arity_;
    size_t entry_ip_;
};

enum class OpCode : uint8_t {
    Constant,
    Nil,
    True,
    False,

    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,

    Equal,
    NotEqual,
    Greater,
    GreaterEqual,
    Less,
    LessEqual,

    Not,
    Negate,

    Print,
    Pop,

    DefineGlobal,
    GetGlobal,
    SetGlobal,
    GetLocal,
    SetLocal,

    JumpIfFalse,
    Jump,
    Loop,

    BuildArray,
    IndexGet,
    IndexSet,

    Call,
    Return
};

class Chunk {
public:
    void write(uint8_t byte, int line) {
        code.push_back(byte);
        lines.push_back(line);
    }

    void write_op(OpCode op, int line) {
        write(static_cast<uint8_t>(op), line);
    }

    int add_constant(Value value) {
        constants.push_back(std::move(value));
        return static_cast<int>(constants.size() - 1);
    }

    void disassemble(std::ostream& os, const std::string& name) const {
        os << "== " << name << " (Bytecode Disassembly) ==\n";
        for (int offset = 0; offset < static_cast<int>(code.size());) {
            offset = disassemble_instruction(os, offset);
        }
        os << "===============================================\n";
    }

    int disassemble_instruction(std::ostream& os, int offset) const {
        os << std::setfill('0') << std::setw(4) << offset << " ";

        if (offset > 0 && lines[offset] == lines[offset - 1]) {
            os << "   | ";
        } else {
            os << std::setfill(' ') << std::setw(4) << lines[offset] << " ";
        }

        uint8_t instruction = code[offset];
        auto op = static_cast<OpCode>(instruction);

        switch (op) {
            case OpCode::Constant: {
                uint8_t constant_idx = code[offset + 1];
                os << std::left << std::setw(18) << "OP_CONSTANT" << " "
                   << static_cast<int>(constant_idx) << " ('"
                   << constants[constant_idx].to_string() << "')\n";
                return offset + 2;
            }
            case OpCode::Nil:
                os << "OP_NIL\n";
                return offset + 1;
            case OpCode::True:
                os << "OP_TRUE\n";
                return offset + 1;
            case OpCode::False:
                os << "OP_FALSE\n";
                return offset + 1;
            case OpCode::Add:
                os << "OP_ADD\n";
                return offset + 1;
            case OpCode::Subtract:
                os << "OP_SUBTRACT\n";
                return offset + 1;
            case OpCode::Multiply:
                os << "OP_MULTIPLY\n";
                return offset + 1;
            case OpCode::Divide:
                os << "OP_DIVIDE\n";
                return offset + 1;
            case OpCode::Modulo:
                os << "OP_MODULO\n";
                return offset + 1;
            case OpCode::Equal:
                os << "OP_EQUAL\n";
                return offset + 1;
            case OpCode::NotEqual:
                os << "OP_NOT_EQUAL\n";
                return offset + 1;
            case OpCode::Greater:
                os << "OP_GREATER\n";
                return offset + 1;
            case OpCode::GreaterEqual:
                os << "OP_GREATER_EQUAL\n";
                return offset + 1;
            case OpCode::Less:
                os << "OP_LESS\n";
                return offset + 1;
            case OpCode::LessEqual:
                os << "OP_LESS_EQUAL\n";
                return offset + 1;
            case OpCode::Not:
                os << "OP_NOT\n";
                return offset + 1;
            case OpCode::Negate:
                os << "OP_NEGATE\n";
                return offset + 1;
            case OpCode::Print:
                os << "OP_PRINT\n";
                return offset + 1;
            case OpCode::Pop:
                os << "OP_POP\n";
                return offset + 1;
            case OpCode::DefineGlobal: {
                uint8_t name_idx = code[offset + 1];
                os << std::left << std::setw(18) << "OP_DEFINE_GLOBAL" << " "
                   << static_cast<int>(name_idx) << " ('"
                   << constants[name_idx].to_string() << "')\n";
                return offset + 2;
            }
            case OpCode::GetGlobal: {
                uint8_t name_idx = code[offset + 1];
                os << std::left << std::setw(18) << "OP_GET_GLOBAL" << " "
                   << static_cast<int>(name_idx) << " ('"
                   << constants[name_idx].to_string() << "')\n";
                return offset + 2;
            }
            case OpCode::SetGlobal: {
                uint8_t name_idx = code[offset + 1];
                os << std::left << std::setw(18) << "OP_SET_GLOBAL" << " "
                   << static_cast<int>(name_idx) << " ('"
                   << constants[name_idx].to_string() << "')\n";
                return offset + 2;
            }
            case OpCode::GetLocal: {
                uint8_t slot = code[offset + 1];
                os << std::left << std::setw(18) << "OP_GET_LOCAL" << " slot "
                   << static_cast<int>(slot) << "\n";
                return offset + 2;
            }
            case OpCode::SetLocal: {
                uint8_t slot = code[offset + 1];
                os << std::left << std::setw(18) << "OP_SET_LOCAL" << " slot "
                   << static_cast<int>(slot) << "\n";
                return offset + 2;
            }
            case OpCode::JumpIfFalse: {
                uint16_t jump = (static_cast<uint16_t>(code[offset + 1]) << 8) | code[offset + 2];
                os << std::left << std::setw(18) << "OP_JUMP_IF_FALSE" << " -> "
                   << (offset + 3 + jump) << "\n";
                return offset + 3;
            }
            case OpCode::Jump: {
                uint16_t jump = (static_cast<uint16_t>(code[offset + 1]) << 8) | code[offset + 2];
                os << std::left << std::setw(18) << "OP_JUMP" << " -> "
                   << (offset + 3 + jump) << "\n";
                return offset + 3;
            }
            case OpCode::Loop: {
                uint16_t jump = (static_cast<uint16_t>(code[offset + 1]) << 8) | code[offset + 2];
                os << std::left << std::setw(18) << "OP_LOOP" << " -> "
                   << (offset + 3 - jump) << "\n";
                return offset + 3;
            }
            case OpCode::BuildArray: {
                uint8_t count = code[offset + 1];
                os << std::left << std::setw(18) << "OP_BUILD_ARRAY" << " "
                   << static_cast<int>(count) << " elements\n";
                return offset + 2;
            }
            case OpCode::IndexGet:
                os << "OP_INDEX_GET\n";
                return offset + 1;
            case OpCode::IndexSet:
                os << "OP_INDEX_SET\n";
                return offset + 1;
            case OpCode::Call: {
                uint8_t arg_count = code[offset + 1];
                os << std::left << std::setw(18) << "OP_CALL" << " "
                   << static_cast<int>(arg_count) << " args\n";
                return offset + 2;
            }
            case OpCode::Return:
                os << "OP_RETURN\n";
                return offset + 1;
        }

        os << "UNKNOWN_OP (" << static_cast<int>(instruction) << ")\n";
        return offset + 1;
    }

    std::vector<uint8_t> code;
    std::vector<Value> constants;
    std::vector<int> lines;
};

} // namespace raven
