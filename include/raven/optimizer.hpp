#pragma once

#include <memory>
#include <vector>

#include "raven/ast.hpp"

namespace raven {

class Optimizer {
public:
    static std::vector<std::shared_ptr<Stmt>> optimize(const std::vector<std::shared_ptr<Stmt>>& statements);

    static std::shared_ptr<Stmt> optimize_statement(const std::shared_ptr<Stmt>& stmt);
    static std::unique_ptr<Expr> optimize_expression(std::unique_ptr<Expr> expr);
};

} // namespace raven
