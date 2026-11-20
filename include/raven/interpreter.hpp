#pragma once

#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <unordered_set>
#include <vector>

#include "raven/ast.hpp"
#include "raven/environment.hpp"
#include "raven/value.hpp"

namespace raven {

struct SandboxConfig {
    bool enabled = false;
    bool allow_shell = false;
    bool allow_ffi = false;
    bool allow_fs_write = false;
    uint64_t max_instructions = 0;
    uint64_t timeout_ms = 0;
};

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
    ~Interpreter();

    void interpret(const std::vector<std::shared_ptr<Stmt>>& statements);
    void execute(const Stmt& stmt);
    Value evaluate(const Expr& expr);
    void execute_block(const std::vector<std::shared_ptr<Stmt>>& statements,
                       std::shared_ptr<Environment> environment);

    std::shared_ptr<Environment> globals() const { return globals_; }
    std::shared_ptr<Environment> environment() const { return environment_; }
    std::ostream& out() { return out_; }

    using StepCallback = std::function<void(Interpreter&, const Stmt&)>;
    void set_step_callback(StepCallback cb) { step_callback_ = std::move(cb); }

    void set_sandbox(const SandboxConfig& config) { sandbox_ = config; }
    const SandboxConfig& sandbox() const { return sandbox_; }

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

    void push_source_file(const std::string& path);
    void pop_source_file();

    void push_this(const Value& receiver) { this_stack_.push_back(receiver); }
    void pop_this() {
        if (!this_stack_.empty()) {
            this_stack_.pop_back();
        }
    }
    bool has_this() const { return !this_stack_.empty(); }
    Value current_this() const {
        return this_stack_.empty() ? Value() : this_stack_.back();
    }

    void push_super(const Value& super_val) { super_stack_.push_back(super_val); }
    void pop_super() {
        if (!super_stack_.empty()) {
            super_stack_.pop_back();
        }
    }
    bool has_super() const { return !super_stack_.empty(); }
    Value current_super() const {
        return super_stack_.empty() ? Value() : super_stack_.back();
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
    void visit(const ClassStmt& stmt) override;
    void visit(const BreakStmt& stmt) override;
    void visit(const ContinueStmt& stmt) override;
    void visit(const FunctionStmt& stmt) override;
    void visit(const ReturnStmt& stmt) override;
    void visit(const DestructureStmt& stmt) override;
    void visit(const UsingStmt& stmt) override;

private:
    void register_builtins();
    void check_number_operands(const Token& op, const Value& left, const Value& right);
    void check_number_operand(const Token& op, const Value& operand);

    std::ostream& out_;
    std::shared_ptr<Environment> globals_;
    std::shared_ptr<Environment> environment_;
    bool owns_globals_ = true;  // false for thread-local interpreters that share globals
    std::unordered_set<std::string> imported_modules_;
    std::vector<std::string> source_dir_stack_;
    std::vector<StackFrame> call_stack_;
    std::vector<Value> this_stack_;
    std::vector<Value> super_stack_;
    StepCallback step_callback_;
    SandboxConfig sandbox_;
    uint64_t instruction_count_ = 0;
    std::chrono::steady_clock::time_point start_time_;
};

Value bind_if_method(const Value& receiver, const Value& property);

} // namespace raven
