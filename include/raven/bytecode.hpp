#pragma once

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "raven/value.hpp"

namespace raven {

class Chunk;

class BytecodeFunction : public Callable {
public:
    BytecodeFunction(std::string name, int arity, size_t entry_ip, const Chunk* chunk = nullptr)
        : name_(std::move(name)), arity_(arity), entry_ip_(entry_ip), chunk_(chunk) {}

    int arity() const override { return arity_; }
    size_t entry_ip() const { return entry_ip_; }
    const std::string& name() const { return name_; }
    const Chunk* chunk() const { return chunk_; }
    void set_chunk(const Chunk* ch) { chunk_ = ch; }

    Value call(Interpreter&, const std::vector<Value>&) override;

    std::string to_string() const override {
        return "<vm fn " + name_ + ">";
    }

private:
    std::string name_;
    int arity_;
    size_t entry_ip_;
    const Chunk* chunk_ = nullptr;
};

enum class OpCode : uint8_t {
    Constant,
    ConstantLong,  // 3-byte index (big-endian 24-bit) — supports up to 16M constants
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
    DefineGlobalLong,
    GetGlobal,
    GetGlobalLong,
    SetGlobal,
    SetGlobalLong,
    GetLocal,
    SetLocal,

    JumpIfFalse,
    JumpIfNotNil,
    Jump,
    Loop,

    BuildArray,
    BuildMap,
    IndexGet,
    IndexSet,
    Length,

    // Array / Collection / Conversion builtins
    ArrayPush,
    ArrayPop,
    ArrayReverse,
    MapKeys,
    MapValues,
    MapHasKey,
    TypeOf,
    ToString,
    ToNumber,
    Slice,

    CheckType,
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
            case OpCode::ConstantLong: {
                uint32_t constant_idx = (static_cast<uint32_t>(code[offset + 1]) << 16)
                                      | (static_cast<uint32_t>(code[offset + 2]) << 8)
                                      |  static_cast<uint32_t>(code[offset + 3]);
                os << std::left << std::setw(18) << "OP_CONSTANT_LONG" << " "
                   << constant_idx << " ('"
                   << constants[constant_idx].to_string() << "')\n";
                return offset + 4;
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
            case OpCode::DefineGlobalLong: {
                uint32_t name_idx = (static_cast<uint32_t>(code[offset + 1]) << 16)
                                  | (static_cast<uint32_t>(code[offset + 2]) << 8)
                                  |  static_cast<uint32_t>(code[offset + 3]);
                os << std::left << std::setw(18) << "OP_DEFINE_GLOBAL_LONG" << " "
                   << name_idx << " ('"
                   << constants[name_idx].to_string() << "')\n";
                return offset + 4;
            }
            case OpCode::GetGlobal: {
                uint8_t name_idx = code[offset + 1];
                os << std::left << std::setw(18) << "OP_GET_GLOBAL" << " "
                   << static_cast<int>(name_idx) << " ('"
                   << constants[name_idx].to_string() << "')\n";
                return offset + 2;
            }
            case OpCode::GetGlobalLong: {
                uint32_t name_idx = (static_cast<uint32_t>(code[offset + 1]) << 16)
                                  | (static_cast<uint32_t>(code[offset + 2]) << 8)
                                  |  static_cast<uint32_t>(code[offset + 3]);
                os << std::left << std::setw(18) << "OP_GET_GLOBAL_LONG" << " "
                   << name_idx << " ('"
                   << constants[name_idx].to_string() << "')\n";
                return offset + 4;
            }
            case OpCode::SetGlobal: {
                uint8_t name_idx = code[offset + 1];
                os << std::left << std::setw(18) << "OP_SET_GLOBAL" << " "
                   << static_cast<int>(name_idx) << " ('"
                   << constants[name_idx].to_string() << "')\n";
                return offset + 2;
            }
            case OpCode::SetGlobalLong: {
                uint32_t name_idx = (static_cast<uint32_t>(code[offset + 1]) << 16)
                                  | (static_cast<uint32_t>(code[offset + 2]) << 8)
                                  |  static_cast<uint32_t>(code[offset + 3]);
                os << std::left << std::setw(18) << "OP_SET_GLOBAL_LONG" << " "
                   << name_idx << " ('"
                   << constants[name_idx].to_string() << "')\n";
                return offset + 4;
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
            case OpCode::JumpIfNotNil: {
                uint16_t jump = (static_cast<uint16_t>(code[offset + 1]) << 8) | code[offset + 2];
                os << std::left << std::setw(18) << "OP_JUMP_IF_NOT_NIL" << " -> "
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
            case OpCode::BuildMap: {
                uint8_t count = code[offset + 1];
                os << std::left << std::setw(18) << "OP_BUILD_MAP" << " "
                   << static_cast<int>(count) << " pairs\n";
                return offset + 2;
            }
            case OpCode::IndexGet:
                os << "OP_INDEX_GET\n";
                return offset + 1;
            case OpCode::IndexSet:
                os << "OP_INDEX_SET\n";
                return offset + 1;
            case OpCode::Length:
                os << "OP_LENGTH\n";
                return offset + 1;
            case OpCode::ArrayPush:
                os << "OP_ARRAY_PUSH\n";
                return offset + 1;
            case OpCode::ArrayPop:
                os << "OP_ARRAY_POP\n";
                return offset + 1;
            case OpCode::ArrayReverse:
                os << "OP_ARRAY_REVERSE\n";
                return offset + 1;
            case OpCode::MapKeys:
                os << "OP_MAP_KEYS\n";
                return offset + 1;
            case OpCode::MapValues:
                os << "OP_MAP_VALUES\n";
                return offset + 1;
            case OpCode::MapHasKey:
                os << "OP_MAP_HAS_KEY\n";
                return offset + 1;
            case OpCode::TypeOf:
                os << "OP_TYPE_OF\n";
                return offset + 1;
            case OpCode::ToString:
                os << "OP_TO_STRING\n";
                return offset + 1;
            case OpCode::ToNumber:
                os << "OP_TO_NUMBER\n";
                return offset + 1;
            case OpCode::Slice:
                os << "OP_SLICE\n";
                return offset + 1;
            case OpCode::CheckType: {
                uint8_t constant = code[offset + 1];
                os << std::left << std::setw(18) << "OP_CHECK_TYPE" << " "
                   << static_cast<int>(constant) << " ('"
                   << constants[constant].to_string() << "')\n";
                return offset + 2;
            }
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
