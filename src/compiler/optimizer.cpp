#include "raven/optimizer.hpp"

#include <cmath>

namespace raven {

std::vector<std::shared_ptr<Stmt>> Optimizer::optimize(const std::vector<std::shared_ptr<Stmt>>& statements) {
    std::vector<std::shared_ptr<Stmt>> result;
    for (const auto& stmt : statements) {
        if (stmt) {
            auto opt = optimize_statement(stmt);
            if (opt) {
                result.push_back(std::move(opt));
            }
        }
    }
    return result;
}

std::unique_ptr<Expr> Optimizer::optimize_expression(std::unique_ptr<Expr> expr) {
    if (!expr) return nullptr;

    if (auto* group = dynamic_cast<GroupingExpr*>(expr.get())) {
        group->expression = optimize_expression(std::move(group->expression));
        if (dynamic_cast<LiteralExpr*>(group->expression.get())) {
            return std::move(group->expression);
        }
        return expr;
    }

    if (auto* unary = dynamic_cast<UnaryExpr*>(expr.get())) {
        unary->right = optimize_expression(std::move(unary->right));
        if (auto* lit = dynamic_cast<LiteralExpr*>(unary->right.get())) {
            if (unary->op.type == TokenType::Minus && lit->value.is_number()) {
                return std::make_unique<LiteralExpr>(Value(-lit->value.as_number()));
            }
            if (unary->op.type == TokenType::Bang) {
                return std::make_unique<LiteralExpr>(Value(!lit->value.is_truthy()));
            }
        }
        return expr;
    }

    if (auto* bin = dynamic_cast<BinaryExpr*>(expr.get())) {
        bin->left = optimize_expression(std::move(bin->left));
        bin->right = optimize_expression(std::move(bin->right));

        auto* left_lit = dynamic_cast<LiteralExpr*>(bin->left.get());
        auto* right_lit = dynamic_cast<LiteralExpr*>(bin->right.get());

        if (left_lit && right_lit) {
            const auto& l = left_lit->value;
            const auto& r = right_lit->value;

            switch (bin->op.type) {
                case TokenType::Plus:
                    if (l.is_number() && r.is_number()) {
                        return std::make_unique<LiteralExpr>(Value(l.as_number() + r.as_number()));
                    }
                    if (l.is_string() || r.is_string()) {
                        return std::make_unique<LiteralExpr>(Value(l.to_string() + r.to_string()));
                    }
                    break;
                case TokenType::Minus:
                    if (l.is_number() && r.is_number()) {
                        return std::make_unique<LiteralExpr>(Value(l.as_number() - r.as_number()));
                    }
                    break;
                case TokenType::Star:
                    if (l.is_number() && r.is_number()) {
                        return std::make_unique<LiteralExpr>(Value(l.as_number() * r.as_number()));
                    }
                    break;
                case TokenType::Slash:
                    if (l.is_number() && r.is_number() && r.as_number() != 0.0) {
                        return std::make_unique<LiteralExpr>(Value(l.as_number() / r.as_number()));
                    }
                    break;
                case TokenType::EqualEqual:
                    return std::make_unique<LiteralExpr>(Value(l == r));
                case TokenType::BangEqual:
                    return std::make_unique<LiteralExpr>(Value(l != r));
                case TokenType::Greater:
                    if (l.is_number() && r.is_number()) return std::make_unique<LiteralExpr>(Value(l.as_number() > r.as_number()));
                    break;
                case TokenType::GreaterEqual:
                    if (l.is_number() && r.is_number()) return std::make_unique<LiteralExpr>(Value(l.as_number() >= r.as_number()));
                    break;
                case TokenType::Less:
                    if (l.is_number() && r.is_number()) return std::make_unique<LiteralExpr>(Value(l.as_number() < r.as_number()));
                    break;
                case TokenType::LessEqual:
                    if (l.is_number() && r.is_number()) return std::make_unique<LiteralExpr>(Value(l.as_number() <= r.as_number()));
                    break;
                default:
                    break;
            }
        }
        return expr;
    }

    if (auto* arr = dynamic_cast<ArrayExpr*>(expr.get())) {
        for (auto& elem : arr->elements) {
            elem = optimize_expression(std::move(elem));
        }
        return expr;
    }

    return expr;
}

std::shared_ptr<Stmt> Optimizer::optimize_statement(const std::shared_ptr<Stmt>& stmt) {
    if (!stmt) return nullptr;

    if (auto* expr_stmt = dynamic_cast<ExpressionStmt*>(stmt.get())) {
        expr_stmt->expression = optimize_expression(std::move(expr_stmt->expression));
        return stmt;
    }

    if (auto* print_stmt = dynamic_cast<PrintStmt*>(stmt.get())) {
        print_stmt->expression = optimize_expression(std::move(print_stmt->expression));
        return stmt;
    }

    if (auto* var_stmt = dynamic_cast<VarDeclStmt*>(stmt.get())) {
        if (var_stmt->initializer) {
            var_stmt->initializer = optimize_expression(std::move(var_stmt->initializer));
        }
        return stmt;
    }

    if (auto* block = dynamic_cast<BlockStmt*>(stmt.get())) {
        block->statements = optimize(block->statements);
        return stmt;
    }

    if (auto* if_stmt = dynamic_cast<IfStmt*>(stmt.get())) {
        if_stmt->condition = optimize_expression(std::move(if_stmt->condition));
        if_stmt->then_branch = optimize_statement(if_stmt->then_branch);
        if (if_stmt->else_branch) {
            if_stmt->else_branch = optimize_statement(if_stmt->else_branch);
        }

        // Dead-branch elimination
        if (auto* lit = dynamic_cast<LiteralExpr*>(if_stmt->condition.get())) {
            if (lit->value.is_truthy()) {
                return if_stmt->then_branch;
            } else if (if_stmt->else_branch) {
                return if_stmt->else_branch;
            } else {
                return std::make_shared<BlockStmt>(std::vector<std::shared_ptr<Stmt>>{});
            }
        }
        return stmt;
    }

    if (auto* while_stmt = dynamic_cast<WhileStmt*>(stmt.get())) {
        while_stmt->condition = optimize_expression(std::move(while_stmt->condition));
        while_stmt->body = optimize_statement(while_stmt->body);
        return stmt;
    }

    return stmt;
}

} // namespace raven
