// statements.cpp — AST statement node helpers
#include "raven/ast.hpp"

namespace raven {

std::string get_stmt_node_type(const Stmt* stmt) {
    if (!stmt) return "Null";
    if (dynamic_cast<const ExpressionStmt*>(stmt)) return "ExpressionStmt";
    if (dynamic_cast<const PrintStmt*>(stmt)) return "PrintStmt";
    if (dynamic_cast<const VarDeclStmt*>(stmt)) return "VarDeclStmt";
    if (dynamic_cast<const BlockStmt*>(stmt)) return "BlockStmt";
    if (dynamic_cast<const IfStmt*>(stmt)) return "IfStmt";
    if (dynamic_cast<const WhileStmt*>(stmt)) return "WhileStmt";
    if (dynamic_cast<const ForInStmt*>(stmt)) return "ForInStmt";
    if (dynamic_cast<const TryCatchStmt*>(stmt)) return "TryCatchStmt";
    if (dynamic_cast<const ThrowStmt*>(stmt)) return "ThrowStmt";
    if (dynamic_cast<const ImportStmt*>(stmt)) return "ImportStmt";
    if (dynamic_cast<const StructStmt*>(stmt)) return "StructStmt";
    if (dynamic_cast<const ClassStmt*>(stmt)) return "ClassStmt";
    if (dynamic_cast<const BreakStmt*>(stmt)) return "BreakStmt";
    if (dynamic_cast<const ContinueStmt*>(stmt)) return "ContinueStmt";
    if (dynamic_cast<const FunctionStmt*>(stmt)) return "FunctionStmt";
    if (dynamic_cast<const ReturnStmt*>(stmt)) return "ReturnStmt";
    return "UnknownStmt";
}

} // namespace raven
