#include "raven/semantic/resolver.hpp"

namespace raven::semantic {

Resolver::Resolver() = default;

bool Resolver::resolve(const std::vector<std::shared_ptr<Stmt>>& statements) {
    for (const auto& stmt : statements) {
        if (stmt) stmt->accept(*this);
    }
    return !has_errors();
}

void Resolver::error(const std::string& msg, int line) {
    errors_.push_back({msg, line});
}

void Resolver::resolve(const Stmt& stmt) {
    const_cast<Stmt&>(stmt).accept(*this);
}

void Resolver::resolve(const Expr& expr) {
    const_cast<Expr&>(expr).accept(*this);
}

// StmtVisitor
void Resolver::visit(const ExpressionStmt& stmt) {
    if (stmt.expression) resolve(*stmt.expression);
}

void Resolver::visit(const PrintStmt& stmt) {
    if (stmt.expression) resolve(*stmt.expression);
    for (const auto& expr : stmt.expressions) {
        if (expr) resolve(*expr);
    }
}

void Resolver::visit(const VarDeclStmt& stmt) {
    if (stmt.initializer) {
        resolve(*stmt.initializer);
    }
    Symbol sym(stmt.name.lexeme, SymbolKind::Variable, scopes_.current_depth(), 0);
    scopes_.define(sym);
}

void Resolver::visit(const DestructureStmt& stmt) {
    if (stmt.initializer) {
        resolve(*stmt.initializer);
    }
    for (const auto& name : stmt.names) {
        Symbol sym(name.lexeme, SymbolKind::Variable, scopes_.current_depth(), 0);
        scopes_.define(sym);
    }
}

void Resolver::visit(const BlockStmt& stmt) {
    scopes_.push_scope(false);
    for (const auto& s : stmt.statements) {
        if (s) s->accept(*this);
    }
    scopes_.pop_scope();
}

void Resolver::visit(const IfStmt& stmt) {
    if (stmt.condition) resolve(*stmt.condition);
    if (stmt.then_branch) stmt.then_branch->accept(*this);
    if (stmt.else_branch) stmt.else_branch->accept(*this);
}

void Resolver::visit(const WhileStmt& stmt) {
    if (stmt.condition) resolve(*stmt.condition);
    loop_depth_++;
    if (stmt.body) stmt.body->accept(*this);
    loop_depth_--;
}

void Resolver::visit(const ForInStmt& stmt) {
    if (stmt.iterable) resolve(*stmt.iterable);
    scopes_.push_scope(false);
    Symbol sym(stmt.variable.lexeme, SymbolKind::Variable, scopes_.current_depth(), 0);
    scopes_.define(sym);
    loop_depth_++;
    if (stmt.body) stmt.body->accept(*this);
    loop_depth_--;
    scopes_.pop_scope();
}

void Resolver::visit(const TryCatchStmt& stmt) {
    if (stmt.try_block) stmt.try_block->accept(*this);
    scopes_.push_scope(false);
    Symbol err_sym(stmt.error_var.lexeme, SymbolKind::Variable, scopes_.current_depth(), 0);
    scopes_.define(err_sym);
    if (stmt.catch_block) stmt.catch_block->accept(*this);
    scopes_.pop_scope();
}

void Resolver::visit(const ThrowStmt& stmt) {
    if (stmt.value) resolve(*stmt.value);
}

void Resolver::visit(const ImportStmt&) {
    // Dynamic import, nothing to resolve at compile time
}

void Resolver::visit(const StructStmt& stmt) {
    Symbol sym(stmt.name.lexeme, SymbolKind::Variable, scopes_.current_depth(), 0);
    scopes_.define(sym);
}

void Resolver::visit(const ClassStmt& stmt) {
    Symbol sym(stmt.name.lexeme, SymbolKind::Variable, scopes_.current_depth(), 0);
    scopes_.define(sym);
    for (const auto& member : stmt.members) {
        for (const auto& body_stmt : member.body) {
            if (body_stmt) resolve(*body_stmt);
        }
        for (const auto& def : member.defaults) {
            if (def) resolve(*def);
        }
    }
}

void Resolver::visit(const BreakStmt& stmt) {
    if (loop_depth_ == 0) {
        error("Cannot use 'break' outside of a loop.", stmt.keyword.line);
    }
}

void Resolver::visit(const ContinueStmt& stmt) {
    if (loop_depth_ == 0) {
        error("Cannot use 'continue' outside of a loop.", stmt.keyword.line);
    }
}

void Resolver::visit(const FunctionStmt& stmt) {
    Symbol fn_sym(stmt.name.lexeme, SymbolKind::Function, scopes_.current_depth(), 0);
    scopes_.define(fn_sym);

    scopes_.push_scope(true);
    function_depth_++;

    for (const auto& param : stmt.params) {
        Symbol param_sym(param.lexeme, SymbolKind::Parameter, scopes_.current_depth(), 0);
        scopes_.define(param_sym);
    }

    for (const auto& s : stmt.body) {
        if (s) s->accept(*this);
    }

    function_depth_--;
    scopes_.pop_scope();
}

void Resolver::visit(const ReturnStmt& stmt) {
    if (function_depth_ == 0) {
        error("Cannot use 'return' outside of a function.", stmt.keyword.line);
    }
    if (stmt.value) resolve(*stmt.value);
}

// ExprVisitor
void Resolver::visit(const BinaryExpr& expr) {
    if (expr.left) resolve(*expr.left);
    if (expr.right) resolve(*expr.right);
}

void Resolver::visit(const GroupingExpr& expr) {
    if (expr.expression) resolve(*expr.expression);
}

void Resolver::visit(const LiteralExpr&) {
    // Literals need no resolution
}

void Resolver::visit(const UnaryExpr& expr) {
    if (expr.right) resolve(*expr.right);
}

void Resolver::visit(const VariableExpr& expr) {
    auto sym = scopes_.lookup(expr.name.lexeme);
    (void)sym;
}

void Resolver::visit(const AssignExpr& expr) {
    if (expr.value) resolve(*expr.value);
}

void Resolver::visit(const LogicalExpr& expr) {
    if (expr.left) resolve(*expr.left);
    if (expr.right) resolve(*expr.right);
}

void Resolver::visit(const CallExpr& expr) {
    if (expr.callee) resolve(*expr.callee);
    for (const auto& arg : expr.arguments) {
        if (arg) resolve(*arg);
    }
}

void Resolver::visit(const ArrayExpr& expr) {
    for (const auto& elem : expr.elements) {
        if (elem) resolve(*elem);
    }
}

void Resolver::visit(const MapExpr& expr) {
    for (const auto& [k, v] : expr.entries) {
        if (k) resolve(*k);
        if (v) resolve(*v);
    }
}

void Resolver::visit(const IndexExpr& expr) {
    if (expr.object) resolve(*expr.object);
    if (expr.index) resolve(*expr.index);
    if (expr.slice_start) resolve(*expr.slice_start);
    if (expr.slice_end) resolve(*expr.slice_end);
}

void Resolver::visit(const IndexAssignExpr& expr) {
    if (expr.object) resolve(*expr.object);
    if (expr.index) resolve(*expr.index);
    if (expr.value) resolve(*expr.value);
}

void Resolver::visit(const PropertyExpr& expr) {
    if (expr.object) resolve(*expr.object);
}

void Resolver::visit(const PropertyAssignExpr& expr) {
    if (expr.object) resolve(*expr.object);
    if (expr.value) resolve(*expr.value);
}

void Resolver::visit(const LambdaExpr& expr) {
    scopes_.push_scope(true);
    function_depth_++;

    for (const auto& param : expr.params) {
        Symbol param_sym(param.lexeme, SymbolKind::Parameter, scopes_.current_depth(), 0);
        scopes_.define(param_sym);
    }

    for (const auto& s : expr.body) {
        if (s) s->accept(*this);
    }

    function_depth_--;
    scopes_.pop_scope();
}

void Resolver::visit(const PipelineExpr& expr) {
    if (expr.left) resolve(*expr.left);
    if (expr.right) resolve(*expr.right);
}

void Resolver::visit(const MatchExpr& expr) {
    if (expr.value) resolve(*expr.value);
    for (const auto& arm : expr.arms) {
        if (arm.pattern) resolve(*arm.pattern);
        if (arm.body) resolve(*arm.body);
    }
}

} // namespace raven::semantic
