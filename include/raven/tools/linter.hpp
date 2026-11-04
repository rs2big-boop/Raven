#pragma once

#include <string>
#include <vector>
#include <memory>
#include <set>
#include "raven/ast.hpp"

namespace raven {

enum class LintSeverity {
    Warning,
    Error,
    Info
};

struct LintIssue {
    LintSeverity severity;
    std::string message;
    int line;
    int column;

    std::string to_string() const;
};

class Linter : public ExprVisitor<void>, public StmtVisitor<void> {
public:
    std::vector<LintIssue> analyze(const std::vector<std::shared_ptr<Stmt>>& statements);

    // StmtVisitor
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
    void visit(const ClassStmt& stmt) override;
    void visit(const BreakStmt& stmt) override;
    void visit(const ContinueStmt& stmt) override;
    void visit(const FunctionStmt& stmt) override;
    void visit(const ReturnStmt& stmt) override;
    void visit(const DestructureStmt& stmt) override;

    // ExprVisitor
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
    std::vector<LintIssue> issues_;
    std::vector<std::set<std::string>> declared_vars_;
    std::vector<std::set<std::string>> used_vars_;
    std::vector<std::vector<std::pair<std::string, Token>>> var_tokens_;

    void push_scope();
    void pop_scope();
};

} // namespace raven
