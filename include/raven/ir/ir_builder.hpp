#pragma once
#include "ir_node.hpp"
#include <string>

namespace raven::ir {

class IRBuilder {
public:
    IRBuilder();

    void set_current_function(IRFunction* fn);
    void set_current_block(IRBasicBlock* block);

    std::string create_temp();
    std::string emit_const_number(double val);
    std::string emit_const_string(const std::string& val);
    std::string emit_const_bool(bool val);
    std::string emit_const_nil();

    std::string emit_binop(IROpCode op, const std::string& lhs, const std::string& rhs);
    std::string emit_unop(IROpCode op, const std::string& operand);
    void emit_store(const std::string& var_name, const std::string& src);
    std::string emit_load(const std::string& var_name);
    void emit_return(const std::string& val = "");
    void emit_jump(const std::string& target_block_label);
    void emit_branch_if_false(const std::string& cond, const std::string& target_block_label);

private:
    IRFunction* current_fn_ = nullptr;
    IRBasicBlock* current_block_ = nullptr;
    size_t temp_counter_ = 0;
};

} // namespace raven::ir
