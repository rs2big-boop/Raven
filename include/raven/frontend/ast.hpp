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
class ArrayExpr;
class MapExpr;
class IndexExpr;
class IndexAssignExpr;
class PropertyExpr;
class PropertyAssignExpr;
class LambdaExpr;
class PipelineExpr;
class MatchExpr;

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
    virtual R visit(const ArrayExpr& expr) = 0;
    virtual R visit(const MapExpr& expr) = 0;
    virtual R visit(const IndexExpr& expr) = 0;
    virtual R visit(const IndexAssignExpr& expr) = 0;
    virtual R visit(const PropertyExpr& expr) = 0;
    virtual R visit(const PropertyAssignExpr& expr) = 0;
    virtual R visit(const LambdaExpr& expr) = 0;
    virtual R visit(const PipelineExpr& expr) = 0;
    virtual R visit(const MatchExpr& expr) = 0;
};

class Expr {
public:
    virtual ~Expr() = default;
    virtual Value accept(ExprVisitor<Value>& visitor) const = 0;
    virtual std::string accept(ExprVisitor<std::string>& visitor) const = 0;
    virtual void accept(ExprVisitor<void>& visitor) const = 0;
};

class BinaryExpr : public Expr {
public:
    BinaryExpr(std::unique_ptr<Expr> left, Token op, std::unique_ptr<Expr> right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}

    Value accept(ExprVisitor<Value>& visitor) const override { return visitor.visit(*this); }
    std::string accept(ExprVisitor<std::string>& visitor) const override { return visitor.visit(*this); }
    void accept(ExprVisitor<void>& visitor) const override { visitor.visit(*this); }

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
    void accept(ExprVisitor<void>& visitor) const override { visitor.visit(*this); }

    std::unique_ptr<Expr> expression;
};

class LiteralExpr : public Expr {
public:
    explicit LiteralExpr(Value value) : value(std::move(value)) {}

    Value accept(ExprVisitor<Value>& visitor) const override { return visitor.visit(*this); }
    std::string accept(ExprVisitor<std::string>& visitor) const override { return visitor.visit(*this); }
    void accept(ExprVisitor<void>& visitor) const override { visitor.visit(*this); }

    Value value;
};

class UnaryExpr : public Expr {
public:
    UnaryExpr(Token op, std::unique_ptr<Expr> right)
        : op(std::move(op)), right(std::move(right)) {}

    Value accept(ExprVisitor<Value>& visitor) const override { return visitor.visit(*this); }
    std::string accept(ExprVisitor<std::string>& visitor) const override { return visitor.visit(*this); }
    void accept(ExprVisitor<void>& visitor) const override { visitor.visit(*this); }

    Token op;
    std::unique_ptr<Expr> right;
};

class VariableExpr : public Expr {
public:
    explicit VariableExpr(Token name) : name(std::move(name)) {}

    Value accept(ExprVisitor<Value>& visitor) const override { return visitor.visit(*this); }
    std::string accept(ExprVisitor<std::string>& visitor) const override { return visitor.visit(*this); }
    void accept(ExprVisitor<void>& visitor) const override { visitor.visit(*this); }

    Token name;
};

class AssignExpr : public Expr {
public:
    AssignExpr(Token name, std::unique_ptr<Expr> value)
        : name(std::move(name)), value(std::move(value)) {}

    Value accept(ExprVisitor<Value>& visitor) const override { return visitor.visit(*this); }
    std::string accept(ExprVisitor<std::string>& visitor) const override { return visitor.visit(*this); }
    void accept(ExprVisitor<void>& visitor) const override { visitor.visit(*this); }

    Token name;
    std::unique_ptr<Expr> value;
};

class LogicalExpr : public Expr {
public:
    LogicalExpr(std::unique_ptr<Expr> left, Token op, std::unique_ptr<Expr> right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}

    Value accept(ExprVisitor<Value>& visitor) const override { return visitor.visit(*this); }
    std::string accept(ExprVisitor<std::string>& visitor) const override { return visitor.visit(*this); }
    void accept(ExprVisitor<void>& visitor) const override { visitor.visit(*this); }

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
    void accept(ExprVisitor<void>& visitor) const override { visitor.visit(*this); }

    std::unique_ptr<Expr> callee;
    Token paren;
    std::vector<std::unique_ptr<Expr>> arguments;
};

class ArrayExpr : public Expr {
public:
    explicit ArrayExpr(std::vector<std::unique_ptr<Expr>> elements)
        : elements(std::move(elements)) {}

    Value accept(ExprVisitor<Value>& visitor) const override { return visitor.visit(*this); }
    std::string accept(ExprVisitor<std::string>& visitor) const override { return visitor.visit(*this); }
    void accept(ExprVisitor<void>& visitor) const override { visitor.visit(*this); }

    std::vector<std::unique_ptr<Expr>> elements;
};

class MapExpr : public Expr {
public:
    explicit MapExpr(std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>> entries)
        : entries(std::move(entries)) {}

    Value accept(ExprVisitor<Value>& visitor) const override { return visitor.visit(*this); }
    std::string accept(ExprVisitor<std::string>& visitor) const override { return visitor.visit(*this); }
    void accept(ExprVisitor<void>& visitor) const override { visitor.visit(*this); }

    std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>> entries;
};

class IndexExpr : public Expr {
public:
    IndexExpr(std::unique_ptr<Expr> object, Token bracket, std::unique_ptr<Expr> index, bool is_optional = false)
        : object(std::move(object)), bracket(std::move(bracket)), index(std::move(index)), is_optional(is_optional) {}

    IndexExpr(std::unique_ptr<Expr> object, Token bracket, std::unique_ptr<Expr> start, std::unique_ptr<Expr> end, bool is_optional = false)
        : object(std::move(object)), bracket(std::move(bracket)), index(nullptr), is_optional(is_optional),
          is_slice(true), slice_start(std::move(start)), slice_end(std::move(end)) {}

    Value accept(ExprVisitor<Value>& visitor) const override { return visitor.visit(*this); }
    std::string accept(ExprVisitor<std::string>& visitor) const override { return visitor.visit(*this); }
    void accept(ExprVisitor<void>& visitor) const override { visitor.visit(*this); }

    std::unique_ptr<Expr> object;
    Token bracket;
    std::unique_ptr<Expr> index;
    bool is_optional = false;
    bool is_slice = false;
    std::unique_ptr<Expr> slice_start = nullptr;
    std::unique_ptr<Expr> slice_end = nullptr;
};

class IndexAssignExpr : public Expr {
public:
    IndexAssignExpr(std::unique_ptr<Expr> object, Token bracket, std::unique_ptr<Expr> index, std::unique_ptr<Expr> value)
        : object(std::move(object)), bracket(std::move(bracket)), index(std::move(index)), value(std::move(value)) {}

    Value accept(ExprVisitor<Value>& visitor) const override { return visitor.visit(*this); }
    std::string accept(ExprVisitor<std::string>& visitor) const override { return visitor.visit(*this); }
    void accept(ExprVisitor<void>& visitor) const override { visitor.visit(*this); }

    std::unique_ptr<Expr> object;
    Token bracket;
    std::unique_ptr<Expr> index;
    std::unique_ptr<Expr> value;
};

class PropertyExpr : public Expr {
public:
    PropertyExpr(std::unique_ptr<Expr> object, Token name, bool is_optional = false)
        : object(std::move(object)), name(std::move(name)), is_optional(is_optional) {}

    Value accept(ExprVisitor<Value>& visitor) const override { return visitor.visit(*this); }
    std::string accept(ExprVisitor<std::string>& visitor) const override { return visitor.visit(*this); }
    void accept(ExprVisitor<void>& visitor) const override { visitor.visit(*this); }

    std::unique_ptr<Expr> object;
    Token name;
    bool is_optional = false;
};

class PropertyAssignExpr : public Expr {
public:
    PropertyAssignExpr(std::unique_ptr<Expr> object, Token name, std::unique_ptr<Expr> value)
        : object(std::move(object)), name(std::move(name)), value(std::move(value)) {}

    Value accept(ExprVisitor<Value>& visitor) const override { return visitor.visit(*this); }
    std::string accept(ExprVisitor<std::string>& visitor) const override { return visitor.visit(*this); }
    void accept(ExprVisitor<void>& visitor) const override { visitor.visit(*this); }

    std::unique_ptr<Expr> object;
    Token name;
    std::unique_ptr<Expr> value;
};

class LambdaExpr : public Expr {
public:
    LambdaExpr(std::vector<Token> params, std::vector<std::shared_ptr<class Stmt>> body,
               std::vector<std::shared_ptr<Expr>> param_defaults = {},
               bool is_variadic = false)
        : params(std::move(params)), body(std::move(body)),
          param_defaults(std::move(param_defaults)), is_variadic(is_variadic) {}

    Value accept(ExprVisitor<Value>& visitor) const override { return visitor.visit(*this); }
    std::string accept(ExprVisitor<std::string>& visitor) const override { return visitor.visit(*this); }
    void accept(ExprVisitor<void>& visitor) const override { visitor.visit(*this); }

    std::vector<Token> params;
    std::vector<std::shared_ptr<class Stmt>> body;
    std::vector<std::shared_ptr<Expr>> param_defaults;
    bool is_variadic = false;
};

class PipelineExpr : public Expr {
public:
    PipelineExpr(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right)
        : left(std::move(left)), right(std::move(right)) {}

    Value accept(ExprVisitor<Value>& visitor) const override { return visitor.visit(*this); }
    std::string accept(ExprVisitor<std::string>& visitor) const override { return visitor.visit(*this); }
    void accept(ExprVisitor<void>& visitor) const override { visitor.visit(*this); }

    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
};

struct MatchArm {
    std::unique_ptr<Expr> pattern;
    bool is_wildcard = false;
    std::unique_ptr<Expr> body;
};

class MatchExpr : public Expr {
public:
    MatchExpr(std::unique_ptr<Expr> value, std::vector<MatchArm> arms)
        : value(std::move(value)), arms(std::move(arms)) {}

    Value accept(ExprVisitor<Value>& visitor) const override { return visitor.visit(*this); }
    std::string accept(ExprVisitor<std::string>& visitor) const override { return visitor.visit(*this); }
    void accept(ExprVisitor<void>& visitor) const override { visitor.visit(*this); }

    std::unique_ptr<Expr> value;
    std::vector<MatchArm> arms;
};

// Forward declarations of Statements
class ExpressionStmt;
class PrintStmt;
class VarDeclStmt;
class BlockStmt;
class IfStmt;
class WhileStmt;
class ForInStmt;
class TryCatchStmt;
class ThrowStmt;
class ImportStmt;
class StructStmt;
class BreakStmt;
class ContinueStmt;
class FunctionStmt;
class ReturnStmt;
class DestructureStmt;

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
    virtual R visit(const ForInStmt& stmt) = 0;
    virtual R visit(const TryCatchStmt& stmt) = 0;
    virtual R visit(const ThrowStmt& stmt) = 0;
    virtual R visit(const ImportStmt& stmt) = 0;
    virtual R visit(const StructStmt& stmt) = 0;
    virtual R visit(const BreakStmt& stmt) = 0;
    virtual R visit(const ContinueStmt& stmt) = 0;
    virtual R visit(const FunctionStmt& stmt) = 0;
    virtual R visit(const ReturnStmt& stmt) = 0;
    virtual R visit(const DestructureStmt& stmt) = 0;
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
        : expression(nullptr) {
        if (expression) {
            expressions.push_back(std::move(expression));
        }
    }

    explicit PrintStmt(std::vector<std::unique_ptr<Expr>> exprs)
        : expression(nullptr), expressions(std::move(exprs)) {}

    void accept(StmtVisitor<void>& visitor) const override { visitor.visit(*this); }

    std::unique_ptr<Expr> expression;
    std::vector<std::unique_ptr<Expr>> expressions;
};

class VarDeclStmt : public Stmt {
public:
    VarDeclStmt(Token name, std::unique_ptr<Expr> initializer, std::string type_annotation = "")
        : name(std::move(name)), initializer(std::move(initializer)), type_annotation(std::move(type_annotation)) {}

    void accept(StmtVisitor<void>& visitor) const override { visitor.visit(*this); }

    Token name;
    std::unique_ptr<Expr> initializer;
    std::string type_annotation;
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

class ForInStmt : public Stmt {
public:
    ForInStmt(Token variable, std::unique_ptr<Expr> iterable, std::shared_ptr<Stmt> body)
        : variable(std::move(variable)), iterable(std::move(iterable)), body(std::move(body)) {}

    void accept(StmtVisitor<void>& visitor) const override { visitor.visit(*this); }

    Token variable;
    std::unique_ptr<Expr> iterable;
    std::shared_ptr<Stmt> body;
};

class TryCatchStmt : public Stmt {
public:
    TryCatchStmt(std::shared_ptr<Stmt> try_block, Token error_var, std::shared_ptr<Stmt> catch_block)
        : try_block(std::move(try_block)), error_var(std::move(error_var)), catch_block(std::move(catch_block)) {}

    void accept(StmtVisitor<void>& visitor) const override { visitor.visit(*this); }

    std::shared_ptr<Stmt> try_block;
    Token error_var;
    std::shared_ptr<Stmt> catch_block;
};

class ThrowStmt : public Stmt {
public:
    ThrowStmt(Token keyword, std::unique_ptr<Expr> value)
        : keyword(std::move(keyword)), value(std::move(value)) {}

    void accept(StmtVisitor<void>& visitor) const override { visitor.visit(*this); }

    Token keyword;
    std::unique_ptr<Expr> value;
};

class ImportStmt : public Stmt {
public:
    ImportStmt(Token keyword, std::string module_path)
        : keyword(std::move(keyword)), module_path(std::move(module_path)) {}

    void accept(StmtVisitor<void>& visitor) const override { visitor.visit(*this); }

    Token keyword;
    std::string module_path;
};

class StructStmt : public Stmt {
public:
    StructStmt(Token name, std::vector<Token> fields)
        : name(std::move(name)), fields(std::move(fields)) {}

    void accept(StmtVisitor<void>& visitor) const override { visitor.visit(*this); }

    Token name;
    std::vector<Token> fields;
};

class BreakStmt : public Stmt {
public:
    explicit BreakStmt(Token keyword) : keyword(std::move(keyword)) {}
    void accept(StmtVisitor<void>& visitor) const override { visitor.visit(*this); }
    Token keyword;
};

class ContinueStmt : public Stmt {
public:
    explicit ContinueStmt(Token keyword) : keyword(std::move(keyword)) {}
    void accept(StmtVisitor<void>& visitor) const override { visitor.visit(*this); }
    Token keyword;
};

class FunctionStmt : public Stmt {
public:
    FunctionStmt(Token name, std::vector<Token> params, std::vector<std::shared_ptr<Stmt>> body,
                 std::vector<std::shared_ptr<Expr>> param_defaults = {},
                 bool is_variadic = false)
        : name(std::move(name)), params(std::move(params)), body(std::move(body)),
          param_defaults(std::move(param_defaults)), is_variadic(is_variadic) {}

    void accept(StmtVisitor<void>& visitor) const override { visitor.visit(*this); }

    Token name;
    std::vector<Token> params;
    std::vector<std::shared_ptr<Stmt>> body;
    std::vector<std::shared_ptr<Expr>> param_defaults;
    bool is_variadic = false;
};

class ReturnStmt : public Stmt {
public:
    ReturnStmt(Token keyword, std::unique_ptr<Expr> value)
        : keyword(std::move(keyword)), value(std::move(value)) {}

    void accept(StmtVisitor<void>& visitor) const override { visitor.visit(*this); }

    Token keyword;
    std::unique_ptr<Expr> value;
};

enum class DestructureKind {
    Array,
    Map
};

class DestructureStmt : public Stmt {
public:
    DestructureStmt(DestructureKind kind, std::vector<Token> names, std::unique_ptr<Expr> initializer)
        : kind(kind), names(std::move(names)), initializer(std::move(initializer)) {}

    void accept(StmtVisitor<void>& visitor) const override { visitor.visit(*this); }

    DestructureKind kind;
    std::vector<Token> names;
    std::unique_ptr<Expr> initializer;
};

} // namespace raven
