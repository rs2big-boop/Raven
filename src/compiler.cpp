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

void Compiler::visit(const VariableExpr& expr) {
    uint8_t name = make_constant(Value(expr.name.lexeme));
    emit_op(OpCode::GetGlobal, expr.name.line);
    emit_byte(name, expr.name.line);
}

void Compiler::visit(const AssignExpr& expr) {
    expr.value->accept(*this);
    uint8_t name = make_constant(Value(expr.name.lexeme));
    emit_op(OpCode::SetGlobal, expr.name.line);
    emit_byte(name, expr.name.line);
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

void Compiler::visit(const MapExpr& /*expr*/) {}

void Compiler::visit(const IndexExpr& expr) {
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

void Compiler::visit(const PropertyExpr& /*expr*/) {}
void Compiler::visit(const PropertyAssignExpr& /*expr*/) {}
void Compiler::visit(const LambdaExpr& /*expr*/) {}

void Compiler::visit(const PipelineExpr& expr) {
    expr.right->accept(*this);
    expr.left->accept(*this);
    emit_op(OpCode::Call);
    emit_byte(1);
}

void Compiler::visit(const MatchExpr& /*expr*/) {}

void Compiler::visit(const ExpressionStmt& stmt) {
    stmt.expression->accept(*this);
    emit_op(OpCode::Pop);
}

void Compiler::visit(const PrintStmt& stmt) {
    stmt.expression->accept(*this);
    emit_op(OpCode::Print);
}

void Compiler::visit(const VarDeclStmt& stmt) {
    if (stmt.initializer) {
        stmt.initializer->accept(*this);
    } else {
        emit_op(OpCode::Nil);
    }
    uint8_t name = make_constant(Value(stmt.name.lexeme));
    emit_op(OpCode::DefineGlobal, stmt.name.line);
    emit_byte(name, stmt.name.line);
}

void Compiler::visit(const BlockStmt& stmt) {
    for (const auto& s : stmt.statements) {
        s->accept(*this);
    }
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
    stmt.condition->accept(*this);

    int exit_jump = emit_jump(OpCode::JumpIfFalse);
    emit_op(OpCode::Pop);

    stmt.body->accept(*this);
    emit_loop(loop_start);

    patch_jump(exit_jump);
    emit_op(OpCode::Pop);
}

void Compiler::visit(const ForInStmt& /*stmt*/) {}
void Compiler::visit(const TryCatchStmt& /*stmt*/) {}
void Compiler::visit(const ThrowStmt& /*stmt*/) {}
void Compiler::visit(const ImportStmt& /*stmt*/) {}
void Compiler::visit(const FunctionStmt& /*stmt*/) {}
void Compiler::visit(const ReturnStmt& stmt) {
    if (stmt.value) {
        stmt.value->accept(*this);
    } else {
        emit_op(OpCode::Nil);
    }
    emit_op(OpCode::Return);
}

} // namespace raven
