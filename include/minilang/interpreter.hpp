#pragma once

#include <iostream>
#include <memory>
#include <vector>

#include "minilang/ast.hpp"
#include "minilang/environment.hpp"
#include "minilang/value.hpp"

namespace minilang {

struct ReturnException {
    Value value;
};

class Interpreter : public ExprVisitor<Value>, public StmtVisitor<void> {
public:
    explicit Interpreter(std::ostream& out = std::cout);

    void interpret(const std::vector<std::shared_ptr<Stmt>>& statements);
    void execute(const Stmt& stmt);
    Value evaluate(const Expr& expr);
    void execute_block(const std::vector<std::shared_ptr<Stmt>>& statements,
                       std::shared_ptr<Environment> environment);

    std::shared_ptr<Environment> globals() const { return globals_; }

    // ExprVisitor overrides
    Value visit(const BinaryExpr& expr) override;
    Value visit(const GroupingExpr& expr) override;
    Value visit(const LiteralExpr& expr) override;
    Value visit(const UnaryExpr& expr) override;
    Value visit(const VariableExpr& expr) override;
    Value visit(const AssignExpr& expr) override;
    Value visit(const LogicalExpr& expr) override;
    Value visit(const CallExpr& expr) override;

    // StmtVisitor overrides
    void visit(const ExpressionStmt& stmt) override;
    void visit(const PrintStmt& stmt) override;
    void visit(const VarDeclStmt& stmt) override;
    void visit(const BlockStmt& stmt) override;
    void visit(const IfStmt& stmt) override;
    void visit(const WhileStmt& stmt) override;
    void visit(const FunctionStmt& stmt) override;
    void visit(const ReturnStmt& stmt) override;

private:
    void check_number_operands(const Token& op, const Value& left, const Value& right);
    void check_number_operand(const Token& op, const Value& operand);

    std::ostream& out_;
    std::shared_ptr<Environment> globals_;
    std::shared_ptr<Environment> environment_;
};

} // namespace minilang
