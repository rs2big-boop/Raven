#include "raven/linter.hpp"
#include <sstream>

namespace raven {

std::string LintIssue::to_string() const {
    std::ostringstream ss;
    ss << "[line " << line << ", col " << column << "] ";
    switch (severity) {
        case LintSeverity::Warning: ss << "Warning: "; break;
        case LintSeverity::Error: ss << "Error: "; break;
        case LintSeverity::Info: ss << "Info: "; break;
    }
    ss << message;
    return ss.str();
}

std::vector<LintIssue> Linter::analyze(const std::vector<std::shared_ptr<Stmt>>& statements) {
    issues_.clear();
    declared_vars_.clear();
    used_vars_.clear();
    var_tokens_.clear();

    push_scope();

    bool has_terminator = false;
    for (const auto& stmt : statements) {
        if (!stmt) continue;
        if (has_terminator) {
            issues_.push_back(LintIssue{
                LintSeverity::Warning,
                "Unreachable code detected after terminating statement.",
                1, 1
            });
            break;
        }

        stmt->accept(*this);

        if (dynamic_cast<ReturnStmt*>(stmt.get()) ||
            dynamic_cast<ThrowStmt*>(stmt.get()) ||
            dynamic_cast<BreakStmt*>(stmt.get()) ||
            dynamic_cast<ContinueStmt*>(stmt.get())) {
            has_terminator = true;
        }
    }

    pop_scope();
    return issues_;
}

void Linter::push_scope() {
    declared_vars_.emplace_back();
    used_vars_.emplace_back();
    var_tokens_.emplace_back();
}

void Linter::pop_scope() {
    if (declared_vars_.empty()) return;

    const auto& used = used_vars_.back();
    const auto& tokens = var_tokens_.back();

    for (const auto& [name, tok] : tokens) {
        // If not used and not starting with underscore
        if (!name.empty() && name[0] != '_' && used.find(name) == used.end()) {
            issues_.push_back(LintIssue{
                LintSeverity::Warning,
                "Unused variable '" + name + "' (prefix with '_' to ignore).",
                tok.line,
                tok.column
            });
        }
    }

    declared_vars_.pop_back();
    used_vars_.pop_back();
    var_tokens_.pop_back();
}

void Linter::visit(const ExpressionStmt& stmt) {
    if (stmt.expression) stmt.expression->accept(*this);
}

void Linter::visit(const PrintStmt& stmt) {
    if (stmt.expression) stmt.expression->accept(*this);
    for (const auto& expr : stmt.expressions) {
        if (expr) expr->accept(*this);
    }
}

void Linter::visit(const VarDeclStmt& stmt) {
    if (stmt.initializer) stmt.initializer->accept(*this);

    if (!declared_vars_.empty()) {
        declared_vars_.back().insert(stmt.name.lexeme);
        var_tokens_.back().push_back({stmt.name.lexeme, stmt.name});
    }
}

void Linter::visit(const DestructureStmt& stmt) {
    if (stmt.initializer) stmt.initializer->accept(*this);
    if (!declared_vars_.empty()) {
        for (const auto& name : stmt.names) {
            declared_vars_.back().insert(name.lexeme);
            var_tokens_.back().push_back({name.lexeme, name});
        }
    }
}

void Linter::visit(const BlockStmt& stmt) {
    push_scope();
    bool has_term = false;
    for (const auto& s : stmt.statements) {
        if (!s) continue;
        if (has_term) {
            issues_.push_back(LintIssue{
                LintSeverity::Warning,
                "Unreachable code detected in block.",
                1, 1
            });
            break;
        }
        s->accept(*this);
        if (dynamic_cast<ReturnStmt*>(s.get()) ||
            dynamic_cast<ThrowStmt*>(s.get()) ||
            dynamic_cast<BreakStmt*>(s.get()) ||
            dynamic_cast<ContinueStmt*>(s.get())) {
            has_term = true;
        }
    }
    pop_scope();
}

void Linter::visit(const IfStmt& stmt) {
    if (stmt.condition) stmt.condition->accept(*this);
    if (stmt.then_branch) stmt.then_branch->accept(*this);
    if (stmt.else_branch) stmt.else_branch->accept(*this);
}

void Linter::visit(const WhileStmt& stmt) {
    if (stmt.condition) stmt.condition->accept(*this);
    if (stmt.body) stmt.body->accept(*this);
}

void Linter::visit(const ForInStmt& stmt) {
    if (stmt.iterable) stmt.iterable->accept(*this);
    push_scope();
    if (!declared_vars_.empty()) {
        declared_vars_.back().insert(stmt.variable.lexeme);
        used_vars_.back().insert(stmt.variable.lexeme); // loop variable is automatically used
    }
    if (stmt.body) stmt.body->accept(*this);
    pop_scope();
}

void Linter::visit(const TryCatchStmt& stmt) {
    if (stmt.try_block) stmt.try_block->accept(*this);
    push_scope();
    if (!declared_vars_.empty()) {
        declared_vars_.back().insert(stmt.error_var.lexeme);
        used_vars_.back().insert(stmt.error_var.lexeme);
    }
    if (stmt.catch_block) stmt.catch_block->accept(*this);
    pop_scope();
}

void Linter::visit(const ThrowStmt& stmt) {
    if (stmt.value) stmt.value->accept(*this);
}

void Linter::visit(const ImportStmt&) {
}

void Linter::visit(const StructStmt&) {
}

void Linter::visit(const ClassStmt& stmt) {
    for (const auto& member : stmt.members) {
        for (const auto& body_stmt : member.body) {
            if (body_stmt) body_stmt->accept(*this);
        }
    }
}

void Linter::visit(const BreakStmt&) {
}

void Linter::visit(const ContinueStmt&) {
}

void Linter::visit(const FunctionStmt& stmt) {
    push_scope();
    for (const auto& param : stmt.params) {
        declared_vars_.back().insert(param.lexeme);
        used_vars_.back().insert(param.lexeme); // parameters count as used
    }
    for (const auto& s : stmt.body) {
        if (s) s->accept(*this);
    }
    pop_scope();
}

void Linter::visit(const ReturnStmt& stmt) {
    if (stmt.value) stmt.value->accept(*this);
}

void Linter::visit(const BinaryExpr& expr) {
    if (expr.left) expr.left->accept(*this);
    if (expr.right) expr.right->accept(*this);
}

void Linter::visit(const GroupingExpr& expr) {
    if (expr.expression) expr.expression->accept(*this);
}

void Linter::visit(const LiteralExpr&) {
}

void Linter::visit(const UnaryExpr& expr) {
    if (expr.right) expr.right->accept(*this);
}

void Linter::visit(const VariableExpr& expr) {
    // Mark as used in whatever scope it belongs to
    for (auto it = used_vars_.rbegin(); it != used_vars_.rend(); ++it) {
        it->insert(expr.name.lexeme);
    }
}

void Linter::visit(const AssignExpr& expr) {
    if (expr.value) expr.value->accept(*this);
    for (auto it = used_vars_.rbegin(); it != used_vars_.rend(); ++it) {
        it->insert(expr.name.lexeme);
    }
}

void Linter::visit(const LogicalExpr& expr) {
    if (expr.left) expr.left->accept(*this);
    if (expr.right) expr.right->accept(*this);
}

void Linter::visit(const CallExpr& expr) {
    if (expr.callee) expr.callee->accept(*this);
    for (const auto& arg : expr.arguments) {
        if (arg) arg->accept(*this);
    }
}

void Linter::visit(const ArrayExpr& expr) {
    for (const auto& el : expr.elements) {
        if (el) el->accept(*this);
    }
}

void Linter::visit(const MapExpr& expr) {
    for (const auto& [k, v] : expr.entries) {
        if (v) v->accept(*this);
    }
}

void Linter::visit(const IndexExpr& expr) {
    if (expr.object) expr.object->accept(*this);
    if (expr.index) expr.index->accept(*this);
    if (expr.slice_start) expr.slice_start->accept(*this);
    if (expr.slice_end) expr.slice_end->accept(*this);
}

void Linter::visit(const IndexAssignExpr& expr) {
    if (expr.object) expr.object->accept(*this);
    if (expr.index) expr.index->accept(*this);
    if (expr.value) expr.value->accept(*this);
}

void Linter::visit(const PropertyExpr& expr) {
    if (expr.object) expr.object->accept(*this);
}

void Linter::visit(const PropertyAssignExpr& expr) {
    if (expr.object) expr.object->accept(*this);
    if (expr.value) expr.value->accept(*this);
}

void Linter::visit(const LambdaExpr& expr) {
    push_scope();
    for (const auto& p : expr.params) {
        declared_vars_.back().insert(p.lexeme);
        used_vars_.back().insert(p.lexeme);
    }
    for (const auto& s : expr.body) {
        if (s) s->accept(*this);
    }
    pop_scope();
}

void Linter::visit(const PipelineExpr& expr) {
    if (expr.left) expr.left->accept(*this);
    if (expr.right) expr.right->accept(*this);
}

void Linter::visit(const MatchExpr& expr) {
    if (expr.value) expr.value->accept(*this);
    for (const auto& arm : expr.arms) {
        if (arm.pattern) arm.pattern->accept(*this);
        if (arm.body) arm.body->accept(*this);
    }
}

} // namespace raven
