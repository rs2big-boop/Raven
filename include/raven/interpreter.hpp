#pragma once

#include <iostream>
#include <memory>
#include <unordered_set>
#include <vector>

#include "raven/ast.hpp"
#include "raven/environment.hpp"
#include "raven/value.hpp"

namespace raven {

struct ReturnException {
    Value value;
};

struct UserException {
    Value value;
};

struct BreakException {};
struct ContinueException {};

class Interpreter : public ExprVisitor<Value>, public StmtVisitor<void> {
public:
    explicit Interpreter(std::ostream& out = std::cout);
    Interpreter(std::shared_ptr<Environment> globals, std::ostream& out);

    void interpret(const std::vector<std::shared_ptr<Stmt>>& statements);
    void execute(const Stmt& stmt);
    Value evaluate(const Expr& expr);
    void execute_block(const std::vector<std::shared_ptr<Stmt>>& statements,
                       std::shared_ptr<Environment> environment);

    std::shared_ptr<Environment> globals() const { return globals_; }
    std::ostream& out() { return out_; }

    const std::vector<StackFrame>& call_stack() const { return call_stack_; }
    std::vector<StackFrame>& call_stack() { return call_stack_; }

    void push_frame(const std::string& name, size_t line, size_t col) {
        call_stack_.push_back(StackFrame{name, line, col});
    }

    void pop_frame() {
        if (!call_stack_.empty()) {
            call_stack_.pop_back();
        }
    }

    // ExprVisitor overrides
    Value visit(const BinaryExpr& expr) override;
    Value visit(const GroupingExpr& expr) override;
    Value visit(const LiteralExpr& expr) override;
    Value visit(const UnaryExpr& expr) override;
    Value visit(const VariableExpr& expr) override;
    Value visit(const AssignExpr& expr) override;
    Value visit(const LogicalExpr& expr) override;
    Value visit(const CallExpr& expr) override;
    Value visit(const ArrayExpr& expr) override;
    Value visit(const MapExpr& expr) override;
    Value visit(const IndexExpr& expr) override;
    Value visit(const IndexAssignExpr& expr) override;
    Value visit(const PropertyExpr& expr) override;
    Value visit(const PropertyAssignExpr& expr) override;
    Value visit(const LambdaExpr& expr) override;
    Value visit(const PipelineExpr& expr) override;
    Value visit(const MatchExpr& expr) override;

    // StmtVisitor overrides
    void visit(const ExpressionStmt& stmt) override;
    void visit(const PrintStmt& stmt) override;
    void visit(const VarDeclStmt& stmt) override;
    void visit(const BlockStmt& stmt) override;
    void visit(const IfStmt& stmt) override;
    void visit(const WhileStmt& stmt) override;
    void visit(const ForInStmt& stmt) override;
    void visit(const TryCatchStmt& stmt) override;
    void visit(const ThrowStmt& stmt) override;
    void visit(const ImportStmt& stmt) override;
    void visit(const StructStmt& stmt) override;
    void visit(const BreakStmt& stmt) override;
    void visit(const ContinueStmt& stmt) override;
    void visit(const FunctionStmt& stmt) override;
    void visit(const ReturnStmt& stmt) override;

private:
    void register_builtins();
    void check_number_operands(const Token& op, const Value& left, const Value& right);
    void check_number_operand(const Token& op, const Value& operand);

    std::ostream& out_;
    std::shared_ptr<Environment> globals_;
    std::shared_ptr<Environment> environment_;
    std::unordered_set<std::string> imported_modules_;
    std::vector<StackFrame> call_stack_;
};

} // namespace raven
