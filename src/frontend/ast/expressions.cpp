// expressions.cpp — AST expression node helpers
#include "raven/ast.hpp"

namespace raven {

std::string get_expr_node_type(const Expr* expr) {
    if (!expr) return "Null";
    if (dynamic_cast<const BinaryExpr*>(expr)) return "BinaryExpr";
    if (dynamic_cast<const GroupingExpr*>(expr)) return "GroupingExpr";
    if (dynamic_cast<const LiteralExpr*>(expr)) return "LiteralExpr";
    if (dynamic_cast<const UnaryExpr*>(expr)) return "UnaryExpr";
    if (dynamic_cast<const VariableExpr*>(expr)) return "VariableExpr";
    if (dynamic_cast<const AssignExpr*>(expr)) return "AssignExpr";
    if (dynamic_cast<const LogicalExpr*>(expr)) return "LogicalExpr";
    if (dynamic_cast<const CallExpr*>(expr)) return "CallExpr";
    if (dynamic_cast<const ArrayExpr*>(expr)) return "ArrayExpr";
    if (dynamic_cast<const MapExpr*>(expr)) return "MapExpr";
    if (dynamic_cast<const IndexExpr*>(expr)) return "IndexExpr";
    if (dynamic_cast<const IndexAssignExpr*>(expr)) return "IndexAssignExpr";
    if (dynamic_cast<const PropertyExpr*>(expr)) return "PropertyExpr";
    if (dynamic_cast<const PropertyAssignExpr*>(expr)) return "PropertyAssignExpr";
    if (dynamic_cast<const LambdaExpr*>(expr)) return "LambdaExpr";
    if (dynamic_cast<const PipelineExpr*>(expr)) return "PipelineExpr";
    if (dynamic_cast<const MatchExpr*>(expr)) return "MatchExpr";
    return "UnknownExpr";
}

} // namespace raven
