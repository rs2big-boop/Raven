#include "raven/ir/ir_builder.hpp"

namespace raven::ir {

IRBuilder::IRBuilder() = default;

void IRBuilder::set_current_function(IRFunction* fn) {
    current_fn_ = fn;
    if (fn) {
        current_block_ = &fn->entry_block();
    } else {
        current_block_ = nullptr;
    }
}

void IRBuilder::set_current_block(IRBasicBlock* block) {
    current_block_ = block;
}

std::string IRBuilder::create_temp() {
    return "%t" + std::to_string(temp_counter_++);
}

std::string IRBuilder::emit_const_number(double val) {
    std::string t = create_temp();
    if (current_block_) {
        IRInstruction inst;
        inst.op = IROpCode::ConstFloat;
        inst.dest = t;
        inst.num_val = val;
        current_block_->add(std::move(inst));
    }
    return t;
}

std::string IRBuilder::emit_const_string(const std::string& val) {
    std::string t = create_temp();
    if (current_block_) {
        IRInstruction inst;
        inst.op = IROpCode::ConstString;
        inst.dest = t;
        inst.str_val = val;
        current_block_->add(std::move(inst));
    }
    return t;
}

std::string IRBuilder::emit_const_bool(bool val) {
    std::string t = create_temp();
    if (current_block_) {
        IRInstruction inst;
        inst.op = IROpCode::ConstBool;
        inst.dest = t;
        inst.num_val = val ? 1.0 : 0.0;
        current_block_->add(std::move(inst));
    }
    return t;
}

std::string IRBuilder::emit_const_nil() {
    std::string t = create_temp();
    if (current_block_) {
        IRInstruction inst;
        inst.op = IROpCode::ConstNil;
        inst.dest = t;
        current_block_->add(std::move(inst));
    }
    return t;
}

std::string IRBuilder::emit_binop(IROpCode op, const std::string& lhs, const std::string& rhs) {
    std::string t = create_temp();
    if (current_block_) {
        IRInstruction inst;
        inst.op = op;
        inst.dest = t;
        inst.args = {lhs, rhs};
        current_block_->add(std::move(inst));
    }
    return t;
}

std::string IRBuilder::emit_unop(IROpCode op, const std::string& operand) {
    std::string t = create_temp();
    if (current_block_) {
        IRInstruction inst;
        inst.op = op;
        inst.dest = t;
        inst.args = {operand};
        current_block_->add(std::move(inst));
    }
    return t;
}

void IRBuilder::emit_store(const std::string& var_name, const std::string& src) {
    if (current_block_) {
        IRInstruction inst;
        inst.op = IROpCode::StoreVar;
        inst.dest = var_name;
        inst.args = {src};
        current_block_->add(std::move(inst));
    }
}

std::string IRBuilder::emit_load(const std::string& var_name) {
    std::string t = create_temp();
    if (current_block_) {
        IRInstruction inst;
        inst.op = IROpCode::LoadVar;
        inst.dest = t;
        inst.args = {var_name};
        current_block_->add(std::move(inst));
    }
    return t;
}

void IRBuilder::emit_return(const std::string& val) {
    if (current_block_) {
        IRInstruction inst;
        inst.op = IROpCode::Return;
        if (!val.empty()) inst.args = {val};
        current_block_->add(std::move(inst));
    }
}

void IRBuilder::emit_jump(const std::string& target_block_label) {
    if (current_block_) {
        IRInstruction inst;
        inst.op = IROpCode::Jump;
        inst.args = {target_block_label};
        current_block_->add(std::move(inst));
    }
}

void IRBuilder::emit_branch_if_false(const std::string& cond, const std::string& target_block_label) {
    if (current_block_) {
        IRInstruction inst;
        inst.op = IROpCode::BranchIfFalse;
        inst.args = {cond, target_block_label};
        current_block_->add(std::move(inst));
    }
}

} // namespace raven::ir
