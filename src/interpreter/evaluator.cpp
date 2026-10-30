#include "raven/interpreter/interpreter.hpp"

namespace raven {

// Helper to evaluate an expression using an interpreter instance
Value evaluate_expression(Interpreter& interp, const Expr& expr) {
    return interp.evaluate(expr);
}

// Helper to evaluate a statement list
void execute_statements(Interpreter& interp, const std::vector<std::shared_ptr<Stmt>>& stmts) {
    interp.interpret(stmts);
}

} // namespace raven
