#include "raven/compiler.hpp"

namespace raven {

Compiler::Compiler(Chunk& chunk) : chunk_(chunk) {}

bool Compiler::compile(const std::vector<std::shared_ptr<Stmt>>& statements) {
    for (const auto& stmt : statements) {
        if (stmt) {
            stmt->accept(*this);
        }
    }
    emit_op(OpCode::Return);
    return true;
}

void Compiler::emit_byte(uint8_t byte, int line) {
    chunk_.write(byte, line == 0 ? current_line_ : line);
}

void Compiler::emit_op(OpCode op, int line) {
    chunk_.write_op(op, line == 0 ? current_line_ : line);
}

int Compiler::emit_jump(OpCode op, int line) {
    emit_op(op, line);
    emit_byte(0xff, line);
    emit_byte(0xff, line);
    return static_cast<int>(chunk_.code.size()) - 2;
}

void Compiler::patch_jump(int offset) {
    int jump = static_cast<int>(chunk_.code.size()) - offset - 2;
    chunk_.code[offset] = static_cast<uint8_t>((jump >> 8) & 0xff);
    chunk_.code[offset + 1] = static_cast<uint8_t>(jump & 0xff);
}

void Compiler::emit_loop(int loop_start, int line) {
    emit_op(OpCode::Loop, line);
    int jump = static_cast<int>(chunk_.code.size()) - loop_start + 2;
    emit_byte(static_cast<uint8_t>((jump >> 8) & 0xff), line);
    emit_byte(static_cast<uint8_t>(jump & 0xff), line);
}

uint8_t Compiler::make_constant(Value value) {
    int constant = chunk_.add_constant(std::move(value));
    return static_cast<uint8_t>(constant);
}

void Compiler::visit(const BinaryExpr& expr) {
    if (expr.op.type == TokenType::QuestionQuestion) {
        expr.left->accept(*this);
        int end_jump = emit_jump(OpCode::JumpIfNotNil);
        emit_op(OpCode::Pop);
        expr.right->accept(*this);
        patch_jump(end_jump);
        return;
    }

    if (expr.op.type == TokenType::Is) {
        expr.left->accept(*this);
        std::string type_name;
        if (auto* var = dynamic_cast<const VariableExpr*>(expr.right.get())) {
            type_name = var->name.lexeme;
        } else {
            type_name = "unknown";
        }
        uint8_t const_idx = make_constant(Value(type_name));
        emit_op(OpCode::CheckType);
        emit_byte(const_idx);
        return;
    }

    expr.left->accept(*this);
    expr.right->accept(*this);

    current_line_ = expr.op.line;
    switch (expr.op.type) {
        case TokenType::Plus: emit_op(OpCode::Add); break;
        case TokenType::Minus: emit_op(OpCode::Subtract); break;
        case TokenType::Star: emit_op(OpCode::Multiply); break;
        case TokenType::Slash: emit_op(OpCode::Divide); break;
        case TokenType::Percent: emit_op(OpCode::Modulo); break;
        case TokenType::EqualEqual: emit_op(OpCode::Equal); break;
        case TokenType::BangEqual: emit_op(OpCode::NotEqual); break;
        case TokenType::Greater: emit_op(OpCode::Greater); break;
        case TokenType::GreaterEqual: emit_op(OpCode::GreaterEqual); break;
        case TokenType::Less: emit_op(OpCode::Less); break;
        case TokenType::LessEqual: emit_op(OpCode::LessEqual); break;
        default: break;
    }
}

void Compiler::visit(const GroupingExpr& expr) {
    expr.expression->accept(*this);
}

void Compiler::visit(const LiteralExpr& expr) {
    if (expr.value.is_nil()) {
        emit_op(OpCode::Nil);
    } else if (expr.value.is_bool()) {
        emit_op(expr.value.as_bool() ? OpCode::True : OpCode::False);
    } else {
        uint8_t idx = make_constant(expr.value);
        emit_op(OpCode::Constant);
        emit_byte(idx);
    }
}

void Compiler::visit(const UnaryExpr& expr) {
    expr.right->accept(*this);
    current_line_ = expr.op.line;
    if (expr.op.type == TokenType::Minus) {
        emit_op(OpCode::Negate);
    } else if (expr.op.type == TokenType::Bang) {
        emit_op(OpCode::Not);
    }
}

int Compiler::resolve_local(const Token& name) {
    for (int i = static_cast<int>(locals_.size()) - 1; i >= 0; --i) {
        if (locals_[i].name.lexeme == name.lexeme) {
            return i;
        }
    }
    return -1;
}

void Compiler::visit(const VariableExpr& expr) {
    int arg = resolve_local(expr.name);
    if (arg != -1) {
        emit_op(OpCode::GetLocal, expr.name.line);
        emit_byte(static_cast<uint8_t>(arg), expr.name.line);
    } else {
        uint8_t name = make_constant(Value(expr.name.lexeme));
        emit_op(OpCode::GetGlobal, expr.name.line);
        emit_byte(name, expr.name.line);
    }
}

void Compiler::visit(const AssignExpr& expr) {
    expr.value->accept(*this);
    int arg = resolve_local(expr.name);
    if (arg != -1) {
        emit_op(OpCode::SetLocal, expr.name.line);
        emit_byte(static_cast<uint8_t>(arg), expr.name.line);
    } else {
        uint8_t name = make_constant(Value(expr.name.lexeme));
        emit_op(OpCode::SetGlobal, expr.name.line);
        emit_byte(name, expr.name.line);
    }
}

void Compiler::visit(const LogicalExpr& expr) {
    expr.left->accept(*this);
    if (expr.op.type == TokenType::PipePipe) {
        // If left is truthy, skip right
        int else_jump = emit_jump(OpCode::JumpIfFalse);
        int end_jump = emit_jump(OpCode::Jump);
        patch_jump(else_jump);
        emit_op(OpCode::Pop);
        expr.right->accept(*this);
        patch_jump(end_jump);
    } else if (expr.op.type == TokenType::AmpAmp) {
        // If left is falsy, skip right
        int end_jump = emit_jump(OpCode::JumpIfFalse);
        emit_op(OpCode::Pop);
        expr.right->accept(*this);
        patch_jump(end_jump);
    }
}

void Compiler::visit(const CallExpr& expr) {
    expr.callee->accept(*this);
    for (const auto& arg : expr.arguments) {
        arg->accept(*this);
    }
    emit_op(OpCode::Call, expr.paren.line);
    emit_byte(static_cast<uint8_t>(expr.arguments.size()), expr.paren.line);
}

void Compiler::visit(const ArrayExpr& expr) {
    for (const auto& elem : expr.elements) {
        elem->accept(*this);
    }
    emit_op(OpCode::BuildArray);
    emit_byte(static_cast<uint8_t>(expr.elements.size()));
}

void Compiler::visit(const MapExpr& expr) {
    for (const auto& entry : expr.entries) {
        entry.first->accept(*this);
        entry.second->accept(*this);
    }
    emit_op(OpCode::BuildMap);
    emit_byte(static_cast<uint8_t>(expr.entries.size()));
}

void Compiler::visit(const IndexExpr& expr) {
    if (expr.is_slice) {
        // Compiler fallback: evaluate object, slice start/end
        expr.object->accept(*this);
        if (expr.slice_start) expr.slice_start->accept(*this);
        else emit_op(OpCode::Nil);
        if (expr.slice_end) expr.slice_end->accept(*this);
        else emit_op(OpCode::Nil);
        emit_op(OpCode::Pop);
        emit_op(OpCode::Pop);
        return;
    }
    expr.object->accept(*this);
    expr.index->accept(*this);
    emit_op(OpCode::IndexGet);
}

void Compiler::visit(const IndexAssignExpr& expr) {
    expr.object->accept(*this);
    expr.index->accept(*this);
    expr.value->accept(*this);
    emit_op(OpCode::IndexSet);
}

void Compiler::visit(const PropertyExpr& expr) {
    expr.object->accept(*this);
    uint8_t idx = make_constant(Value(expr.name.lexeme));
    emit_op(OpCode::Constant);
    emit_byte(idx);
    emit_op(OpCode::IndexGet);
}

void Compiler::visit(const PropertyAssignExpr& expr) {
    expr.object->accept(*this);
    uint8_t idx = make_constant(Value(expr.name.lexeme));
    emit_op(OpCode::Constant);
    emit_byte(idx);
    expr.value->accept(*this);
    emit_op(OpCode::IndexSet);
}

void Compiler::visit(const LambdaExpr& expr) {
    int jump_over = emit_jump(OpCode::Jump);
    size_t entry_ip = chunk_.code.size();

    scope_depth_++;
    for (const auto& param : expr.params) {
        locals_.push_back(Local{param, scope_depth_});
    }

    for (const auto& s : expr.body) {
        if (s) s->accept(*this);
    }

    emit_op(OpCode::Nil);
    emit_op(OpCode::Return);

    while (!locals_.empty() && locals_.back().depth >= scope_depth_) {
        locals_.pop_back();
    }
    scope_depth_--;

    patch_jump(jump_over);

    auto fn_obj = std::make_shared<BytecodeFunction>("<lambda>", static_cast<int>(expr.params.size()), entry_ip);
    uint8_t fn_const = make_constant(Value(fn_obj));
    emit_op(OpCode::Constant);
    emit_byte(fn_const);
}

void Compiler::visit(const PipelineExpr& expr) {
    expr.right->accept(*this);
    expr.left->accept(*this);
    emit_op(OpCode::Call);
    emit_byte(1);
}

void Compiler::visit(const MatchExpr& expr) {
    expr.value->accept(*this);
    std::string temp_name = "__match_val_" + std::to_string(chunk_.code.size());
    Token temp_token{TokenType::Identifier, temp_name, 0, 0};
    locals_.push_back(Local{temp_token, scope_depth_});
    uint8_t temp_slot = static_cast<uint8_t>(locals_.size() - 1);

    std::vector<int> end_jumps;

    for (const auto& arm : expr.arms) {
        if (arm.is_wildcard) {
            arm.body->accept(*this);
            end_jumps.push_back(emit_jump(OpCode::Jump));
            break;
        }

        emit_op(OpCode::GetLocal);
        emit_byte(temp_slot);
        arm.pattern->accept(*this);
        emit_op(OpCode::Equal);

        int next_arm_jump = emit_jump(OpCode::JumpIfFalse);
        emit_op(OpCode::Pop);

        arm.body->accept(*this);
        end_jumps.push_back(emit_jump(OpCode::Jump));

        patch_jump(next_arm_jump);
        emit_op(OpCode::Pop);
    }

    emit_op(OpCode::Nil);

    for (int j : end_jumps) {
        patch_jump(j);
    }

    locals_.pop_back();
}

void Compiler::visit(const ExpressionStmt& stmt) {
    stmt.expression->accept(*this);
    emit_op(OpCode::Pop);
}

void Compiler::visit(const PrintStmt& stmt) {
    if (stmt.expressions.empty()) {
        if (stmt.expression) {
            stmt.expression->accept(*this);
            emit_op(OpCode::Print);
        }
        return;
    }
    for (const auto& expr : stmt.expressions) {
        expr->accept(*this);
        emit_op(OpCode::Print);
    }
}

void Compiler::visit(const VarDeclStmt& stmt) {
    if (stmt.initializer) {
        stmt.initializer->accept(*this);
    } else {
        emit_op(OpCode::Nil);
    }
    if (scope_depth_ > 0) {
        locals_.push_back(Local{stmt.name, scope_depth_});
    } else {
        uint8_t name = make_constant(Value(stmt.name.lexeme));
        emit_op(OpCode::DefineGlobal, stmt.name.line);
        emit_byte(name, stmt.name.line);
    }
}

void Compiler::visit(const DestructureStmt& stmt) {
    if (stmt.initializer) {
        stmt.initializer->accept(*this);
    } else {
        emit_op(OpCode::Nil);
    }
    emit_op(OpCode::Pop);
    for (const auto& name : stmt.names) {
        emit_op(OpCode::Nil, name.line);
        if (scope_depth_ > 0) {
            locals_.push_back(Local{name, scope_depth_});
        } else {
            uint8_t name_const = make_constant(Value(name.lexeme));
            emit_op(OpCode::DefineGlobal, name.line);
            emit_byte(name_const, name.line);
        }
    }
}

void Compiler::visit(const BlockStmt& stmt) {
    scope_depth_++;
    for (const auto& s : stmt.statements) {
        if (s) s->accept(*this);
    }
    while (!locals_.empty() && locals_.back().depth >= scope_depth_) {
        emit_op(OpCode::Pop);
        locals_.pop_back();
    }
    scope_depth_--;
}

void Compiler::visit(const IfStmt& stmt) {
    stmt.condition->accept(*this);
    int then_jump = emit_jump(OpCode::JumpIfFalse);
    emit_op(OpCode::Pop);

    stmt.then_branch->accept(*this);
    int else_jump = emit_jump(OpCode::Jump);

    patch_jump(then_jump);
    emit_op(OpCode::Pop);

    if (stmt.else_branch) {
        stmt.else_branch->accept(*this);
    }
    patch_jump(else_jump);
}

void Compiler::visit(const WhileStmt& stmt) {
    int loop_start = static_cast<int>(chunk_.code.size());
    loop_stack_.push_back(LoopContext{loop_start, scope_depth_, {}});

    stmt.condition->accept(*this);

    int exit_jump = emit_jump(OpCode::JumpIfFalse);
    emit_op(OpCode::Pop);

    stmt.body->accept(*this);
    emit_loop(loop_start);

    patch_jump(exit_jump);
    emit_op(OpCode::Pop);

    for (int break_jump : loop_stack_.back().break_jumps) {
        patch_jump(break_jump);
    }
    loop_stack_.pop_back();
}

void Compiler::visit(const ForInStmt& stmt) {
    scope_depth_++;

    stmt.iterable->accept(*this);
    std::string iter_name = "__for_iter_" + std::to_string(chunk_.code.size());
    Token iter_token{TokenType::Identifier, iter_name, stmt.variable.line, 0};
    locals_.push_back(Local{iter_token, scope_depth_});
    uint8_t iter_slot = static_cast<uint8_t>(locals_.size() - 1);

    uint8_t zero_idx = make_constant(Value(0.0));
    emit_op(OpCode::Constant);
    emit_byte(zero_idx);
    std::string idx_name = "__for_idx_" + std::to_string(chunk_.code.size());
    Token idx_token{TokenType::Identifier, idx_name, stmt.variable.line, 0};
    locals_.push_back(Local{idx_token, scope_depth_});
    uint8_t idx_slot = static_cast<uint8_t>(locals_.size() - 1);

    emit_op(OpCode::GetLocal);
    emit_byte(iter_slot);
    emit_op(OpCode::Length);
    std::string len_name = "__for_len_" + std::to_string(chunk_.code.size());
    Token len_token{TokenType::Identifier, len_name, stmt.variable.line, 0};
    locals_.push_back(Local{len_token, scope_depth_});
    uint8_t len_slot = static_cast<uint8_t>(locals_.size() - 1);

    emit_op(OpCode::Nil);
    locals_.push_back(Local{stmt.variable, scope_depth_});
    uint8_t var_slot = static_cast<uint8_t>(locals_.size() - 1);

    int loop_start = static_cast<int>(chunk_.code.size());
    loop_stack_.push_back(LoopContext{loop_start, scope_depth_, {}});

    emit_op(OpCode::GetLocal);
    emit_byte(idx_slot);
    emit_op(OpCode::GetLocal);
    emit_byte(len_slot);
    emit_op(OpCode::Less);

    int exit_jump = emit_jump(OpCode::JumpIfFalse);
    emit_op(OpCode::Pop);

    emit_op(OpCode::GetLocal);
    emit_byte(iter_slot);
    emit_op(OpCode::GetLocal);
    emit_byte(idx_slot);
    emit_op(OpCode::IndexGet);
    emit_op(OpCode::SetLocal);
    emit_byte(var_slot);
    emit_op(OpCode::Pop);

    stmt.body->accept(*this);

    emit_op(OpCode::GetLocal);
    emit_byte(idx_slot);
    uint8_t one_idx = make_constant(Value(1.0));
    emit_op(OpCode::Constant);
    emit_byte(one_idx);
    emit_op(OpCode::Add);
    emit_op(OpCode::SetLocal);
    emit_byte(idx_slot);
    emit_op(OpCode::Pop);

    emit_loop(loop_start);

    patch_jump(exit_jump);
    emit_op(OpCode::Pop);

    for (int break_jump : loop_stack_.back().break_jumps) {
        patch_jump(break_jump);
    }
    loop_stack_.pop_back();

    while (!locals_.empty() && locals_.back().depth >= scope_depth_) {
        emit_op(OpCode::Pop);
        locals_.pop_back();
    }
    scope_depth_--;
}
void Compiler::visit(const TryCatchStmt& /*stmt*/) {}
void Compiler::visit(const ThrowStmt& /*stmt*/) {}
void Compiler::visit(const ImportStmt& /*stmt*/) {}
void Compiler::visit(const StructStmt& /*stmt*/) {}
void Compiler::visit(const BreakStmt& /*stmt*/) {
    if (loop_stack_.empty()) return;
    int pop_count = 0;
    for (auto it = locals_.rbegin(); it != locals_.rend(); ++it) {
        if (it->depth > loop_stack_.back().scope_depth) pop_count++;
        else break;
    }
    for (int i = 0; i < pop_count; ++i) emit_op(OpCode::Pop);
    int jump = emit_jump(OpCode::Jump);
    loop_stack_.back().break_jumps.push_back(jump);
}
void Compiler::visit(const ContinueStmt& /*stmt*/) {
    if (loop_stack_.empty()) return;
    int pop_count = 0;
    for (auto it = locals_.rbegin(); it != locals_.rend(); ++it) {
        if (it->depth > loop_stack_.back().scope_depth) pop_count++;
        else break;
    }
    for (int i = 0; i < pop_count; ++i) emit_op(OpCode::Pop);
    emit_loop(loop_stack_.back().start_ip);
}
void Compiler::visit(const FunctionStmt& stmt) {
    int jump_over = emit_jump(OpCode::Jump);
    size_t entry_ip = chunk_.code.size();

    scope_depth_++;
    for (const auto& param : stmt.params) {
        locals_.push_back(Local{param, scope_depth_});
    }

    for (const auto& s : stmt.body) {
        if (s) s->accept(*this);
    }

    emit_op(OpCode::Nil);
    emit_op(OpCode::Return);

    while (!locals_.empty() && locals_.back().depth >= scope_depth_) {
        locals_.pop_back();
    }
    scope_depth_--;

    patch_jump(jump_over);

    auto fn_obj = std::make_shared<BytecodeFunction>(stmt.name.lexeme, static_cast<int>(stmt.params.size()), entry_ip);
    uint8_t fn_const = make_constant(Value(fn_obj));
    emit_op(OpCode::Constant);
    emit_byte(fn_const);

    uint8_t name_const = make_constant(Value(stmt.name.lexeme));
    emit_op(OpCode::DefineGlobal);
    emit_byte(name_const);
}
void Compiler::visit(const ReturnStmt& stmt) {
    if (stmt.value) {
        stmt.value->accept(*this);
    } else {
        emit_op(OpCode::Nil);
    }
    emit_op(OpCode::Return);
}

} // namespace raven
