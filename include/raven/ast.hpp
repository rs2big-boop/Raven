#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "raven/token.hpp"
#include "raven/value.hpp"

namespace raven {

// Forward declarations of Expressions
class BinaryExpr;
class GroupingExpr;
class LiteralExpr;
class UnaryExpr;
class VariableExpr;
class AssignExpr;
class LogicalExpr;
class CallExpr;

template <typename R>
class ExprVisitor {
public:
    virtual ~ExprVisitor() = default;
    virtual R visit(const BinaryExpr& expr) = 0;
    virtual R visit(const GroupingExpr& expr) = 0;
    virtual R visit(const LiteralExpr& expr) = 0;
    virtual R visit(const UnaryExpr& expr) = 0;
    virtual R visit(const VariableExpr& expr) = 0;
    virtual R visit(const AssignExpr& expr) = 0;
    virtual R visit(const LogicalExpr& expr) = 0;
    virtual R visit(const CallExpr& expr) = 0;
};

class Expr {
public:
    virtual ~Expr() = default;
    virtual Value accept(ExprVisitor<Value>& visitor) const = 0;
    virtual std::string accept(ExprVisitor<std::string>& visitor) const = 0;
};

class BinaryExpr : public Expr {
public:
    BinaryExpr(std::unique_ptr<Expr> left, Token op, std::unique_ptr<Expr> right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}

    Value accept(ExprVisitor<Value>& visitor) const override { return visitor.visit(*this); }
    std::string accept(ExprVisitor<std::string>& visitor) const override { return visitor.visit(*this); }

    std::unique_ptr<Expr> left;
    Token op;
    std::unique_ptr<Expr> right;
};

class GroupingExpr : public Expr {
public:
    explicit GroupingExpr(std::unique_ptr<Expr> expression)
        : expression(std::move(expression)) {}

    Value accept(ExprVisitor<Value>& visitor) const override { return visitor.visit(*this); }
    std::string accept(ExprVisitor<std::string>& visitor) const override { return visitor.visit(*this); }

    std::unique_ptr<Expr> expression;
};

class LiteralExpr : public Expr {
public:
    explicit LiteralExpr(Value value) : value(std::move(value)) {}

    Value accept(ExprVisitor<Value>& visitor) const override { return visitor.visit(*this); }
    std::string accept(ExprVisitor<std::string>& visitor) const override { return visitor.visit(*this); }

    Value value;
};

class UnaryExpr : public Expr {
public:
    UnaryExpr(Token op, std::unique_ptr<Expr> right)
        : op(std::move(op)), right(std::move(right)) {}

    Value accept(ExprVisitor<Value>& visitor) const override { return visitor.visit(*this); }
    std::string accept(ExprVisitor<std::string>& visitor) const override { return visitor.visit(*this); }

    Token op;
    std::unique_ptr<Expr> right;
};

class VariableExpr : public Expr {
public:
    explicit VariableExpr(Token name) : name(std::move(name)) {}

    Value accept(ExprVisitor<Value>& visitor) const override { return visitor.visit(*this); }
    std::string accept(ExprVisitor<std::string>& visitor) const override { return visitor.visit(*this); }

    Token name;
};

class AssignExpr : public Expr {
public:
    AssignExpr(Token name, std::unique_ptr<Expr> value)
        : name(std::move(name)), value(std::move(value)) {}

    Value accept(ExprVisitor<Value>& visitor) const override { return visitor.visit(*this); }
    std::string accept(ExprVisitor<std::string>& visitor) const override { return visitor.visit(*this); }

    Token name;
    std::unique_ptr<Expr> value;
};

class LogicalExpr : public Expr {
public:
    LogicalExpr(std::unique_ptr<Expr> left, Token op, std::unique_ptr<Expr> right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}

    Value accept(ExprVisitor<Value>& visitor) const override { return visitor.visit(*this); }
    std::string accept(ExprVisitor<std::string>& visitor) const override { return visitor.visit(*this); }

    std::unique_ptr<Expr> left;
    Token op;
    std::unique_ptr<Expr> right;
};

class CallExpr : public Expr {
public:
    CallExpr(std::unique_ptr<Expr> callee, Token paren, std::vector<std::unique_ptr<Expr>> arguments)
        : callee(std::move(callee)), paren(std::move(paren)), arguments(std::move(arguments)) {}

    Value accept(ExprVisitor<Value>& visitor) const override { return visitor.visit(*this); }
    std::string accept(ExprVisitor<std::string>& visitor) const override { return visitor.visit(*this); }

    std::unique_ptr<Expr> callee;
    Token paren;
    std::vector<std::unique_ptr<Expr>> arguments;
};

// Forward declarations of Statements
class ExpressionStmt;
class PrintStmt;
class VarDeclStmt;
class BlockStmt;
class IfStmt;
class WhileStmt;
class FunctionStmt;
class ReturnStmt;

template <typename R>
class StmtVisitor {
public:
    virtual ~StmtVisitor() = default;
    virtual R visit(const ExpressionStmt& stmt) = 0;
    virtual R visit(const PrintStmt& stmt) = 0;
    virtual R visit(const VarDeclStmt& stmt) = 0;
    virtual R visit(const BlockStmt& stmt) = 0;
    virtual R visit(const IfStmt& stmt) = 0;
    virtual R visit(const WhileStmt& stmt) = 0;
    virtual R visit(const FunctionStmt& stmt) = 0;
    virtual R visit(const ReturnStmt& stmt) = 0;
};

class Stmt {
public:
    virtual ~Stmt() = default;
    virtual void accept(StmtVisitor<void>& visitor) const = 0;
};

class ExpressionStmt : public Stmt {
public:
    explicit ExpressionStmt(std::unique_ptr<Expr> expression)
        : expression(std::move(expression)) {}

    void accept(StmtVisitor<void>& visitor) const override { visitor.visit(*this); }

    std::unique_ptr<Expr> expression;
};

class PrintStmt : public Stmt {
public:
    explicit PrintStmt(std::unique_ptr<Expr> expression)
        : expression(std::move(expression)) {}

    void accept(StmtVisitor<void>& visitor) const override { visitor.visit(*this); }

    std::unique_ptr<Expr> expression;
};

class VarDeclStmt : public Stmt {
public:
    VarDeclStmt(Token name, std::unique_ptr<Expr> initializer)
        : name(std::move(name)), initializer(std::move(initializer)) {}

    void accept(StmtVisitor<void>& visitor) const override { visitor.visit(*this); }

    Token name;
    std::unique_ptr<Expr> initializer;
};

class BlockStmt : public Stmt {
public:
    explicit BlockStmt(std::vector<std::shared_ptr<Stmt>> statements)
        : statements(std::move(statements)) {}

    void accept(StmtVisitor<void>& visitor) const override { visitor.visit(*this); }

    std::vector<std::shared_ptr<Stmt>> statements;
};

class IfStmt : public Stmt {
public:
    IfStmt(std::unique_ptr<Expr> condition,
           std::shared_ptr<Stmt> then_branch,
           std::shared_ptr<Stmt> else_branch)
        : condition(std::move(condition)),
          then_branch(std::move(then_branch)),
          else_branch(std::move(else_branch)) {}

    void accept(StmtVisitor<void>& visitor) const override { visitor.visit(*this); }

    std::unique_ptr<Expr> condition;
    std::shared_ptr<Stmt> then_branch;
    std::shared_ptr<Stmt> else_branch;
};

class WhileStmt : public Stmt {
public:
    WhileStmt(std::unique_ptr<Expr> condition, std::shared_ptr<Stmt> body)
        : condition(std::move(condition)), body(std::move(body)) {}

    void accept(StmtVisitor<void>& visitor) const override { visitor.visit(*this); }

    std::unique_ptr<Expr> condition;
    std::shared_ptr<Stmt> body;
};

class FunctionStmt : public Stmt {
public:
    FunctionStmt(Token name, std::vector<Token> params, std::vector<std::shared_ptr<Stmt>> body)
        : name(std::move(name)), params(std::move(params)), body(std::move(body)) {}

    void accept(StmtVisitor<void>& visitor) const override { visitor.visit(*this); }

    Token name;
    std::vector<Token> params;
    std::vector<std::shared_ptr<Stmt>> body;
};

class ReturnStmt : public Stmt {
public:
    ReturnStmt(Token keyword, std::unique_ptr<Expr> value)
        : keyword(std::move(keyword)), value(std::move(value)) {}

    void accept(StmtVisitor<void>& visitor) const override { visitor.visit(*this); }

    Token keyword;
    std::unique_ptr<Expr> value;
};

} // namespace raven
