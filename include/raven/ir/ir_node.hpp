#pragma once
#include <string>
#include <vector>
#include <memory>

namespace raven::ir {

enum class IROpCode {
    ConstInt,
    ConstFloat,
    ConstString,
    ConstBool,
    ConstNil,
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Not,
    Negate,
    LoadVar,
    StoreVar,
    Jump,
    BranchIfFalse,
    Call,
    Return
};

std::string ir_opcode_name(IROpCode op);

struct IRInstruction {
    IROpCode op;
    std::string dest;
    std::vector<std::string> args;
    double num_val = 0.0;
    std::string str_val;

    std::string to_string() const;
};

struct IRBasicBlock {
    std::string label;
    std::vector<IRInstruction> instructions;

    void add(IRInstruction inst) { instructions.push_back(std::move(inst)); }
};

struct IRFunction {
    std::string name;
    std::vector<std::string> params;
    std::vector<IRBasicBlock> blocks;

    IRBasicBlock& entry_block() {
        if (blocks.empty()) {
            blocks.push_back(IRBasicBlock{"entry", {}});
        }
        return blocks.front();
    }
};

struct IRModule {
    std::string name;
    std::vector<IRFunction> functions;

    IRFunction* find_function(const std::string& fn_name);
};

} // namespace raven::ir
