#pragma once
#include "scopes.hpp"
#include "raven/ast.hpp"
#include <vector>
#include <string>

namespace raven::semantic {

struct ResolverError {
    std::string message;
    int line;
};

class Resolver : public StmtVisitor<void>, public ExprVisitor<void> {
public:
    Resolver();

    bool resolve(const std::vector<std::shared_ptr<Stmt>>& statements);

    const std::vector<ResolverError>& errors() const { return errors_; }
    bool has_errors() const { return !errors_.empty(); }

    // StmtVisitor<void>
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
    void visit(const DestructureStmt& stmt) override;

    // ExprVisitor<void>
    void visit(const BinaryExpr& expr) override;
    void visit(const GroupingExpr& expr) override;
    void visit(const LiteralExpr& expr) override;
    void visit(const UnaryExpr& expr) override;
    void visit(const VariableExpr& expr) override;
    void visit(const AssignExpr& expr) override;
    void visit(const LogicalExpr& expr) override;
    void visit(const CallExpr& expr) override;
    void visit(const ArrayExpr& expr) override;
    void visit(const MapExpr& expr) override;
    void visit(const IndexExpr& expr) override;
    void visit(const IndexAssignExpr& expr) override;
    void visit(const PropertyExpr& expr) override;
    void visit(const PropertyAssignExpr& expr) override;
    void visit(const LambdaExpr& expr) override;
    void visit(const PipelineExpr& expr) override;
    void visit(const MatchExpr& expr) override;

private:
    void resolve(const Stmt& stmt);
    void resolve(const Expr& expr);
    void error(const std::string& msg, int line = 0);

    ScopeChain scopes_;
    std::vector<ResolverError> errors_;
    int function_depth_ = 0;
    int loop_depth_     = 0;
};

} // namespace raven::semantic
