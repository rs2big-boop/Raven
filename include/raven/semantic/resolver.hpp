#pragma once
#include "scopes.hpp"
#include "raven/ast.hpp"
#include <vector>
#include <string>
#include <unordered_set>

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
    void visit(const ClassStmt& stmt) override;
    void visit(const BreakStmt& stmt) override;
    void visit(const ContinueStmt& stmt) override;
    void visit(const FunctionStmt& stmt) override;
    void visit(const ReturnStmt& stmt) override;
    void visit(const DestructureStmt& stmt) override;
    void visit(const UsingStmt& stmt) override;

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

    // --- Move-Only tracking ---
    // Stack of per-scope sets: each set tracks variable names that have been moved
    // in the current scope. We push/pop along with scopes_.
    std::vector<std::unordered_set<std::string>> moved_scopes_;

    void push_scope(bool is_function = false) {
        scopes_.push_scope(is_function);
        moved_scopes_.emplace_back();
    }
    void pop_scope() {
        scopes_.pop_scope();
        if (!moved_scopes_.empty()) moved_scopes_.pop_back();
    }

    void push_moved_scope() { moved_scopes_.emplace_back(); }
    void pop_moved_scope()  { if (!moved_scopes_.empty()) moved_scopes_.pop_back(); }

    // Mark a variable as moved in the innermost scope that contains it
    void mark_moved(const std::string& name, int line);
    // Check if a variable is moved — scans from innermost scope outward
    bool is_moved(const std::string& name) const;
    // Un-mark a variable (reassignment revives it)
    void unmark_moved(const std::string& name);
};

} // namespace raven::semantic
