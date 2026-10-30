#include "raven/ir/ir_node.hpp"
#include <sstream>

namespace raven::ir {

std::string ir_opcode_name(IROpCode op) {
    switch (op) {
        case IROpCode::ConstInt:      return "const_int";
        case IROpCode::ConstFloat:    return "const_float";
        case IROpCode::ConstString:   return "const_string";
        case IROpCode::ConstBool:     return "const_bool";
        case IROpCode::ConstNil:      return "const_nil";
        case IROpCode::Add:           return "add";
        case IROpCode::Subtract:      return "sub";
        case IROpCode::Multiply:      return "mul";
        case IROpCode::Divide:        return "div";
        case IROpCode::Modulo:        return "mod";
        case IROpCode::Equal:         return "eq";
        case IROpCode::NotEqual:      return "ne";
        case IROpCode::Less:          return "lt";
        case IROpCode::LessEqual:     return "le";
        case IROpCode::Greater:       return "gt";
        case IROpCode::GreaterEqual:  return "ge";
        case IROpCode::Not:           return "not";
        case IROpCode::Negate:        return "neg";
        case IROpCode::LoadVar:       return "load";
        case IROpCode::StoreVar:      return "store";
        case IROpCode::Jump:          return "jmp";
        case IROpCode::BranchIfFalse: return "br_false";
        case IROpCode::Call:          return "call";
        case IROpCode::Return:        return "ret";
        default:                      return "unknown";
    }
}

std::string IRInstruction::to_string() const {
    std::ostringstream ss;
    if (!dest.empty()) {
        ss << dest << " = ";
    }
    ss << ir_opcode_name(op);
    if (op == IROpCode::ConstInt || op == IROpCode::ConstFloat) {
        ss << " " << num_val;
    } else if (op == IROpCode::ConstString) {
        ss << " \"" << str_val << "\"";
    } else if (op == IROpCode::ConstBool) {
        ss << " " << (num_val != 0 ? "true" : "false");
    }
    for (const auto& arg : args) {
        ss << " " << arg;
    }
    return ss.str();
}

IRFunction* IRModule::find_function(const std::string& fn_name) {
    for (auto& fn : functions) {
        if (fn.name == fn_name) return &fn;
    }
    return nullptr;
}

} // namespace raven::ir
