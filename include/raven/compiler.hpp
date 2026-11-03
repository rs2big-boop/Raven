#pragma once

#include <memory>
#include <vector>

#include "raven/ast.hpp"
#include "raven/bytecode.hpp"

namespace raven {

class Compiler : public ExprVisitor<void>, public StmtVisitor<void> {
public:
    explicit Compiler(Chunk& chunk);

    bool compile(const std::vector<std::shared_ptr<Stmt>>& statements);

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
    void visit(const BreakStmt& stmt) override;
    void visit(const ContinueStmt& stmt) override;
    void visit(const FunctionStmt& stmt) override;
    void visit(const ReturnStmt& stmt) override;
    void visit(const DestructureStmt& stmt) override;

private:
    struct Local {
        Token name;
        int depth;
    };

    struct LoopContext {
        int start_ip;
        int scope_depth;
        std::vector<int> break_jumps;
    };

    void emit_byte(uint8_t byte, int line = 0);
    void emit_op(OpCode op, int line = 0);
    int emit_jump(OpCode op, int line = 0);
    void patch_jump(int offset);
    void emit_loop(int loop_start, int line = 0);
    // emit_constant handles both Constant (index < 256) and ConstantLong (index >= 256).
    // Prefer emit_constant over make_constant + manual emit_op/emit_byte.
    void emit_constant(Value value, int line = 0);
    void emit_define_global(uint32_t name_idx, int line = 0);
    void emit_get_global(uint32_t name_idx, int line = 0);
    void emit_set_global(uint32_t name_idx, int line = 0);
    // make_constant adds to constant pool and returns the index; callers are
    // responsible for emitting the appropriate opcode + operand bytes.
    uint32_t make_constant(Value value);

    int resolve_local(const Token& name);

    Chunk& chunk_;
    int current_line_ = 1;
    std::vector<Local> locals_;
    std::vector<LoopContext> loop_stack_;
    int scope_depth_ = 0;
};

} // namespace raven
