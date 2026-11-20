#include "raven/native/native_compiler.hpp"
#include "raven/native/native_runtime.hpp"
#include "raven/lexer.hpp"
#include "raven/parser.hpp"
#include "raven/optimizer.hpp"
#include "raven/platform/filesystem.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_set>

#if !defined(_WIN32) && !defined(_WIN64)
#include <sys/stat.h>
#endif

namespace fs = std::filesystem;

namespace raven::native {

namespace {

static std::string escape_cpp_string(const std::string& s) {
    std::string out = "\"";
    for (char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '\"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            case '\0': out += "\\0"; break;
            default:
                if (static_cast<unsigned char>(c) < 32 || static_cast<unsigned char>(c) >= 127) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\x%02x", static_cast<unsigned char>(c));
                    out += buf;
                } else {
                    out += c;
                }
                break;
        }
    }
    out += "\"";
    return out;
}

static std::string var_ident(const std::string& name) {
    return "rvn_var_" + name;
}

static std::string fn_ident(const std::string& name) {
    return "rvn_fn_" + name;
}

static std::string val_ident(const std::string& name) {
    return "rvn_val_" + name;
}

static const std::unordered_set<std::string> BUILTINS = {
    "print", "len", "push", "pop", "keys", "values", "type", "str", "num",
    "range", "map", "filter", "reduce", "map_has_key", "clock", "time", "sleep",
    "env", "get_env", "set_env"
};

class NativeExprEmitter : public ExprVisitor<std::string> {
public:
    explicit NativeExprEmitter(const std::unordered_set<std::string>& known_functions)
        : known_functions_(known_functions) {}

    std::string emit(const Expr& expr) {
        return expr.accept(*this);
    }

    std::string visit(const BinaryExpr& expr) override {
        if (expr.op.type == TokenType::QuestionQuestion) {
            std::string l = emit(*expr.left);
            std::string r = emit(*expr.right);
            return "rvn_rt::coalesce(" + l + ", " + r + ")";
        }

        if (expr.op.type == TokenType::Is) {
            std::string l = emit(*expr.left);
            std::string type_name;
            if (auto* var_expr = dynamic_cast<VariableExpr*>(expr.right.get())) {
                type_name = var_expr->name.lexeme;
            } else {
                type_name = emit(*expr.right);
                return "rvn_rt::is_type(" + l + ", (" + type_name + ").to_string())";
            }
            return "rvn_rt::is_type(" + l + ", " + escape_cpp_string(type_name) + ")";
        }

        std::string l = emit(*expr.left);
        std::string r = emit(*expr.right);

        switch (expr.op.type) {
            case TokenType::Plus:         return "rvn_rt::add(" + l + ", " + r + ")";
            case TokenType::Minus:        return "rvn_rt::sub(" + l + ", " + r + ")";
            case TokenType::Star:         return "rvn_rt::mul(" + l + ", " + r + ")";
            case TokenType::Slash:        return "rvn_rt::div(" + l + ", " + r + ")";
            case TokenType::Percent:      return "rvn_rt::mod(" + l + ", " + r + ")";
            case TokenType::EqualEqual:   return "rvn_rt::eq(" + l + ", " + r + ")";
            case TokenType::BangEqual:    return "rvn_rt::neq(" + l + ", " + r + ")";
            case TokenType::Less:         return "rvn_rt::lt(" + l + ", " + r + ")";
            case TokenType::LessEqual:    return "rvn_rt::lte(" + l + ", " + r + ")";
            case TokenType::Greater:      return "rvn_rt::gt(" + l + ", " + r + ")";
            case TokenType::GreaterEqual: return "rvn_rt::gte(" + l + ", " + r + ")";
            default:                      return "Value()";
        }
    }

    std::string visit(const GroupingExpr& expr) override {
        return "(" + emit(*expr.expression) + ")";
    }

    std::string visit(const LiteralExpr& expr) override {
        if (expr.value.is_nil()) return "Value()";
        if (expr.value.is_bool()) return expr.value.as_bool() ? "Value(true)" : "Value(false)";
        if (expr.value.is_number()) {
            double n = expr.value.as_number();
            if (std::isnan(n)) return "Value(std::nan(\"\"))";
            if (std::isinf(n)) return n > 0 ? "Value(std::numeric_limits<double>::infinity())" : "Value(-std::numeric_limits<double>::infinity())";
            std::ostringstream ss;
            ss << std::setprecision(17) << n;
            std::string s = ss.str();
            if (s.find('.') == std::string::npos && s.find('e') == std::string::npos && s.find('E') == std::string::npos) {
                s += ".0";
            }
            return "Value(" + s + ")";
        }
        if (expr.value.is_string()) {
            return "Value(std::string(" + escape_cpp_string(expr.value.as_string()) + "))";
        }
        return "Value()";
    }

    std::string visit(const UnaryExpr& expr) override {
        if (expr.op.type == TokenType::Bang) {
            return "rvn_rt::lnot(" + emit(*expr.right) + ")";
        }
        if (expr.op.type == TokenType::Minus) {
            return "rvn_rt::neg(" + emit(*expr.right) + ")";
        }
        if (expr.op.type == TokenType::Move) {
            if (auto* var_expr = dynamic_cast<const VariableExpr*>(expr.right.get())) {
                return "std::move(" + var_ident(var_expr->name.lexeme) + ")";
            }
        }
        return emit(*expr.right);
    }

    std::string visit(const VariableExpr& expr) override {
        const std::string& name = expr.name.lexeme;
        if (known_functions_.count(name)) {
            return val_ident(name);
        }
        return var_ident(name);
    }

    std::string visit(const AssignExpr& expr) override {
        return "(" + var_ident(expr.name.lexeme) + " = " + emit(*expr.value) + ")";
    }

    std::string visit(const LogicalExpr& expr) override {
        std::string l = emit(*expr.left);
        std::string r = emit(*expr.right);
        if (expr.op.type == TokenType::AmpAmp) {
            return "([&]() -> Value { Value _l = " + l + "; return _l.is_truthy() ? " + r + " : _l; })()";
        } else if (expr.op.type == TokenType::PipePipe) {
            return "([&]() -> Value { Value _l = " + l + "; return _l.is_truthy() ? _l : " + r + "; })()";
        }
        return "Value()";
    }

    std::string visit(const CallExpr& expr) override {
        std::vector<std::string> args_str;
        for (const auto& arg : expr.arguments) {
            args_str.push_back(emit(*arg));
        }

        // Direct fast path for known builtins
        if (auto* var_expr = dynamic_cast<VariableExpr*>(expr.callee.get())) {
            const std::string& fn_name = var_expr->name.lexeme;

            if (fn_name == "print") {
                std::string res = "([&]() -> Value { rvn_rt::print(std::vector<Value>{";
                for (size_t i = 0; i < args_str.size(); ++i) {
                    if (i > 0) res += ", ";
                    res += args_str[i];
                }
                res += "}, true); return Value(); })()";
                return res;
            }
            if (fn_name == "len" && args_str.size() == 1) {
                return "rvn_rt::builtin_len(" + args_str[0] + ")";
            }
            if (fn_name == "push" && args_str.size() == 2) {
                return "rvn_rt::builtin_push(" + args_str[0] + ", " + args_str[1] + ")";
            }
            if (fn_name == "pop" && args_str.size() == 1) {
                return "rvn_rt::builtin_pop(" + args_str[0] + ")";
            }
            if (fn_name == "keys" && args_str.size() == 1) {
                return "rvn_rt::builtin_keys(" + args_str[0] + ")";
            }
            if (fn_name == "values" && args_str.size() == 1) {
                return "rvn_rt::builtin_values(" + args_str[0] + ")";
            }
            if (fn_name == "map_has_key" && args_str.size() == 2) {
                return "rvn_rt::builtin_map_has_key(" + args_str[0] + ", " + args_str[1] + ")";
            }
            if (fn_name == "type" && args_str.size() == 1) {
                return "rvn_rt::builtin_type(" + args_str[0] + ")";
            }
            if (fn_name == "str" && args_str.size() == 1) {
                return "rvn_rt::builtin_str(" + args_str[0] + ")";
            }
            if (fn_name == "num" && args_str.size() == 1) {
                return "rvn_rt::builtin_num(" + args_str[0] + ")";
            }
            if (fn_name == "range" && args_str.size() == 2) {
                return "rvn_rt::builtin_range(" + args_str[0] + ", " + args_str[1] + ")";
            }
            if (fn_name == "filter" && args_str.size() == 2) {
                return "rvn_rt::builtin_filter(" + args_str[0] + ", " + args_str[1] + ")";
            }
            if (fn_name == "map" && args_str.size() == 2) {
                return "rvn_rt::builtin_map(" + args_str[0] + ", " + args_str[1] + ")";
            }
            if (fn_name == "reduce" && args_str.size() == 3) {
                return "rvn_rt::builtin_reduce(" + args_str[0] + ", " + args_str[1] + ", " + args_str[2] + ")";
            }
            if (fn_name == "time" && args_str.empty()) {
                return "rvn_rt::builtin_time()";
            }
            if (fn_name == "clock" && args_str.empty()) {
                return "rvn_rt::builtin_clock()";
            }
            if (fn_name == "sleep" && args_str.size() == 1) {
                return "rvn_rt::builtin_sleep(" + args_str[0] + ")";
            }
            if (fn_name == "get_env" && args_str.size() == 1) {
                return "rvn_rt::builtin_get_env(" + args_str[0] + ")";
            }
            if (fn_name == "set_env" && args_str.size() == 2) {
                return "rvn_rt::builtin_set_env(" + args_str[0] + ", " + args_str[1] + ")";
            }
            if (fn_name == "env") {
                std::string res = "rvn_rt::builtin_env(std::vector<Value>{";
                for (size_t i = 0; i < args_str.size(); ++i) {
                    if (i > 0) res += ", ";
                    res += args_str[i];
                }
                res += "})";
                return res;
            }

            // Direct call to known user function
            if (known_functions_.count(fn_name)) {
                std::string res = fn_ident(fn_name) + "(std::vector<Value>{";
                for (size_t i = 0; i < args_str.size(); ++i) {
                    if (i > 0) res += ", ";
                    res += args_str[i];
                }
                res += "})";
                return res;
            }
        }

        // Generic dynamic call
        std::string callee_str = emit(*expr.callee);
        std::string res = "rvn_rt::call(" + callee_str + ", std::vector<Value>{";
        for (size_t i = 0; i < args_str.size(); ++i) {
            if (i > 0) res += ", ";
            res += args_str[i];
        }
        res += "})";
        return res;
    }

    std::string visit(const ArrayExpr& expr) override {
        std::string res = "rvn_rt::make_array({";
        for (size_t i = 0; i < expr.elements.size(); ++i) {
            if (i > 0) res += ", ";
            res += emit(*expr.elements[i]);
        }
        res += "})";
        return res;
    }

    std::string visit(const MapExpr& expr) override {
        std::string res = "rvn_rt::make_map({";
        for (size_t i = 0; i < expr.entries.size(); ++i) {
            if (i > 0) res += ", ";
            const auto& [k, v] = expr.entries[i];
            std::string k_str;
            if (auto* lit = dynamic_cast<LiteralExpr*>(k.get())) {
                k_str = escape_cpp_string(lit->value.to_string());
            } else {
                k_str = "(" + emit(*k) + ").to_string()";
            }
            std::string v_str = v ? emit(*v) : "Value()";
            res += "{" + k_str + ", " + v_str + "}";
        }
        res += "})";
        return res;
    }

    std::string visit(const IndexExpr& expr) override {
        std::string obj_str = emit(*expr.object);
        if (expr.is_slice) {
            std::string s_val = expr.slice_start ? emit(*expr.slice_start) : "Value(0.0)";
            std::string e_val = expr.slice_end ? emit(*expr.slice_end) : "Value(0.0)";
            std::string has_s = expr.slice_start ? "true" : "false";
            std::string has_e = expr.slice_end ? "true" : "false";
            return "rvn_rt::slice(" + obj_str + ", " + s_val + ", " + e_val + ", " + has_s + ", " + has_e + ")";
        }
        std::string idx_str = expr.index ? emit(*expr.index) : "Value()";
        std::string opt_str = expr.is_optional ? "true" : "false";
        return "rvn_rt::index_get(" + obj_str + ", " + idx_str + ", " + opt_str + ")";
    }

    std::string visit(const IndexAssignExpr& expr) override {
        std::string obj_str = emit(*expr.object);
        std::string idx_str = emit(*expr.index);
        std::string val_str = emit(*expr.value);
        return "rvn_rt::index_set(" + obj_str + ", " + idx_str + ", " + val_str + ")";
    }

    std::string visit(const PropertyExpr& expr) override {
        std::string obj_str = emit(*expr.object);
        std::string prop = escape_cpp_string(expr.name.lexeme);
        std::string opt = expr.is_optional ? "true" : "false";
        return "rvn_rt::property_get(" + obj_str + ", " + prop + ", " + opt + ")";
    }

    std::string visit(const PropertyAssignExpr& expr) override {
        std::string obj_str = emit(*expr.object);
        std::string prop = escape_cpp_string(expr.name.lexeme);
        std::string val_str = emit(*expr.value);
        return "rvn_rt::property_set(" + obj_str + ", " + prop + ", " + val_str + ")";
    }

    std::string visit(const LambdaExpr& expr) override {
        std::ostringstream ss;
        int arity = static_cast<int>(expr.params.size());
        ss << "rvn_rt::make_callable(\"\", " << arity << ", [=](const std::vector<Value>& rvn_args) mutable -> Value {\n";
        for (size_t i = 0; i < expr.params.size(); ++i) {
            ss << "    Value " << var_ident(expr.params[i].lexeme)
               << " = rvn_args.size() > " << i << " ? rvn_args[" << i << "] : Value();\n";
        }
        NativeExprEmitter inner_emitter(known_functions_);
        for (const auto& s : expr.body) {
            if (auto* ret = dynamic_cast<ReturnStmt*>(s.get())) {
                if (ret->value) {
                    ss << "    return " << inner_emitter.emit(*ret->value) << ";\n";
                } else {
                    ss << "    return Value();\n";
                }
            } else if (auto* expr_st = dynamic_cast<ExpressionStmt*>(s.get())) {
                ss << "    " << inner_emitter.emit(*expr_st->expression) << ";\n";
            }
        }
        ss << "    return Value();\n";
        ss << "})";
        return ss.str();
    }

    std::string visit(const PipelineExpr& expr) override {
        std::string l = emit(*expr.left);
        std::string r = emit(*expr.right);
        return "rvn_rt::call(" + r + ", {" + l + "})";
    }

    std::string visit(const MatchExpr& expr) override {
        std::string val_str = emit(*expr.value);
        std::ostringstream ss;
        ss << "([&]() -> Value {\n"
           << "    Value _m = " << val_str << ";\n";
        for (const auto& arm : expr.arms) {
            if (arm.is_wildcard) {
                ss << "    return " << emit(*arm.body) << ";\n";
            } else if (arm.pattern) {
                ss << "    if ((rvn_rt::eq(_m, " << emit(*arm.pattern) << ")).is_truthy()) {\n"
                   << "        return " << emit(*arm.body) << ";\n"
                   << "    }\n";
            }
        }
        ss << "    return Value();\n"
           << "})()";
        return ss.str();
    }

private:
    const std::unordered_set<std::string>& known_functions_;
};

class NativeStmtEmitter : public StmtVisitor<void> {
public:
    NativeStmtEmitter(std::ostringstream& out,
                      const std::unordered_set<std::string>& known_functions,
                      int indent_level = 1)
        : out_(out), known_functions_(known_functions), expr_emitter_(known_functions), indent_level_(indent_level) {}

    void emit(const Stmt& stmt) {
        stmt.accept(*this);
    }

    void visit(const ExpressionStmt& stmt) override {
        indent();
        out_ << expr_emitter_.emit(*stmt.expression) << ";\n";
    }

    void visit(const PrintStmt& stmt) override {
        indent();
        out_ << "rvn_rt::print(std::vector<Value>{";
        if (!stmt.expressions.empty()) {
            for (size_t i = 0; i < stmt.expressions.size(); ++i) {
                if (i > 0) out_ << ", ";
                out_ << expr_emitter_.emit(*stmt.expressions[i]);
            }
        } else if (stmt.expression) {
            out_ << expr_emitter_.emit(*stmt.expression);
        }
        out_ << "}, " << (stmt.newline ? "true" : "false") << ");\n";
    }

    void visit(const VarDeclStmt& stmt) override {
        indent();
        std::string init = stmt.initializer ? expr_emitter_.emit(*stmt.initializer) : "Value()";
        out_ << "Value " << var_ident(stmt.name.lexeme) << " = " << init << ";\n";
    }

    void visit(const BlockStmt& stmt) override {
        indent();
        out_ << "{\n";
        NativeStmtEmitter inner(out_, known_functions_, indent_level_ + 1);
        for (const auto& s : stmt.statements) {
            inner.emit(*s);
        }
        indent();
        out_ << "}\n";
    }

    void visit(const IfStmt& stmt) override {
        indent();
        out_ << "if ((" << expr_emitter_.emit(*stmt.condition) << ").is_truthy()) {\n";
        {
            NativeStmtEmitter inner(out_, known_functions_, indent_level_ + 1);
            if (stmt.then_branch) inner.emit(*stmt.then_branch);
        }
        if (stmt.else_branch) {
            indent();
            out_ << "} else {\n";
            NativeStmtEmitter inner(out_, known_functions_, indent_level_ + 1);
            inner.emit(*stmt.else_branch);
        }
        indent();
        out_ << "}\n";
    }

    void visit(const WhileStmt& stmt) override {
        indent();
        out_ << "while ((" << expr_emitter_.emit(*stmt.condition) << ").is_truthy()) {\n";
        {
            NativeStmtEmitter inner(out_, known_functions_, indent_level_ + 1);
            if (stmt.body) inner.emit(*stmt.body);
        }
        indent();
        out_ << "}\n";
    }

    void visit(const ForInStmt& stmt) override {
        indent();
        out_ << "for (const auto& " << var_ident(stmt.variable.lexeme)
             << " : rvn_rt::for_in_iterable(" << expr_emitter_.emit(*stmt.iterable) << ")) {\n";
        {
            NativeStmtEmitter inner(out_, known_functions_, indent_level_ + 1);
            if (stmt.body) inner.emit(*stmt.body);
        }
        indent();
        out_ << "}\n";
    }

    void visit(const BreakStmt&) override {
        indent();
        out_ << "break;\n";
    }

    void visit(const ContinueStmt&) override {
        indent();
        out_ << "continue;\n";
    }

    void visit(const ReturnStmt& stmt) override {
        indent();
        if (stmt.value) {
            out_ << "return " << expr_emitter_.emit(*stmt.value) << ";\n";
        } else {
            out_ << "return Value();\n";
        }
    }

    void visit(const TryCatchStmt& stmt) override {
        indent();
        out_ << "try {\n";
        {
            NativeStmtEmitter inner(out_, known_functions_, indent_level_ + 1);
            if (stmt.try_block) inner.emit(*stmt.try_block);
        }
        indent();
        out_ << "} catch (const std::exception& _err) {\n";
        {
            indent_level_++;
            indent();
            out_ << "Value " << var_ident(stmt.error_var.lexeme) << "(std::string(_err.what()));\n";
            NativeStmtEmitter inner(out_, known_functions_, indent_level_);
            if (stmt.catch_block) inner.emit(*stmt.catch_block);
            indent_level_--;
        }
        indent();
        out_ << "}\n";
    }

    void visit(const ThrowStmt& stmt) override {
        indent();
        out_ << "throw std::runtime_error((" << expr_emitter_.emit(*stmt.value) << ").to_string());\n";
    }

    void visit(const DestructureStmt& stmt) override {
        indent();
        std::string init = expr_emitter_.emit(*stmt.initializer);
        std::string tmp_name = "_destruct_tmp_" + std::to_string(tmp_counter_++);
        out_ << "Value " << tmp_name << " = " << init << ";\n";
        for (size_t i = 0; i < stmt.names.size(); ++i) {
            indent();
            if (stmt.kind == DestructureKind::Array) {
                out_ << "Value " << var_ident(stmt.names[i].lexeme)
                     << " = rvn_rt::destructure_array_elem(" << tmp_name << ", " << i << ");\n";
            } else {
                out_ << "Value " << var_ident(stmt.names[i].lexeme)
                     << " = rvn_rt::destructure_map_elem(" << tmp_name << ", "
                     << escape_cpp_string(stmt.names[i].lexeme) << ");\n";
            }
        }
    }

    void visit(const UsingStmt& stmt) override {
        indent();
        std::string init = expr_emitter_.emit(*stmt.initializer);
        out_ << "{\n";
        indent_level_++;
        indent();
        out_ << "Value " << var_ident(stmt.name.lexeme) << " = " << init << ";\n";
        indent();
        out_ << "try {\n";
        {
            NativeStmtEmitter inner(out_, known_functions_, indent_level_ + 1);
            for (const auto& s : stmt.body) inner.emit(*s);
        }
        indent();
        out_ << "} catch (...) {\n";
        indent();
        out_ << "    rvn_rt::close_resource(" << var_ident(stmt.name.lexeme) << ");\n";
        indent();
        out_ << "    throw;\n";
        indent();
        out_ << "}\n";
        indent();
        out_ << "rvn_rt::close_resource(" << var_ident(stmt.name.lexeme) << ");\n";
        indent_level_--;
        indent();
        out_ << "}\n";
    }

    void visit(const StructStmt& stmt) override {
        // Struct constructor emitted at file level
        (void)stmt;
    }

    void visit(const ClassStmt& stmt) override {
        (void)stmt;
    }

    void visit(const FunctionStmt& stmt) override {
        // Functions emitted at file level
        (void)stmt;
    }

    void visit(const ImportStmt& stmt) override {
        indent();
        out_ << "// import \"" << stmt.module_path << "\"\n";
        out_ << "rvn_rt::get_interpreter().interpret_module(" << escape_cpp_string(stmt.module_path) << ");\n";
    }

private:
    void indent() {
        for (int i = 0; i < indent_level_; ++i) {
            out_ << "    ";
        }
    }

    std::ostringstream& out_;
    const std::unordered_set<std::string>& known_functions_;
    NativeExprEmitter expr_emitter_;
    int indent_level_;
    static inline int tmp_counter_ = 0;
};

} // namespace

std::string NativeCompiler::compile_to_c(const std::vector<std::shared_ptr<Stmt>>& statements,
                                        const std::string& module_name) {
    (void)module_name;
    std::ostringstream out;

    out << "// Generated by Raven AOT Native Compiler\n";
    out << "#include \"raven/native/native_runtime.hpp\"\n\n";
    out << "using namespace raven;\n";
    out << "namespace rvn_rt = raven::native::rt;\n\n";

    // 1. Collect all top-level functions and structs
    std::unordered_set<std::string> known_functions;
    std::vector<const FunctionStmt*> functions;
    std::vector<const StructStmt*> structs;
    std::vector<const Stmt*> top_level_stmts;

    for (const auto& stmt : statements) {
        if (!stmt) continue;
        if (auto* fn = dynamic_cast<const FunctionStmt*>(stmt.get())) {
            known_functions.insert(fn->name.lexeme);
            functions.push_back(fn);
        } else if (auto* st = dynamic_cast<const StructStmt*>(stmt.get())) {
            known_functions.insert(st->name.lexeme);
            structs.push_back(st);
        } else {
            top_level_stmts.push_back(stmt.get());
        }
    }

    // 2. Forward declare functions and value wrappers
    out << "// --- Forward Declarations ---\n";
    for (const auto* fn : functions) {
        out << "Value " << fn_ident(fn->name.lexeme) << "(const std::vector<Value>& rvn_args);\n";
        out << "extern Value " << val_ident(fn->name.lexeme) << ";\n";
    }
    for (const auto* st : structs) {
        out << "Value " << fn_ident(st->name.lexeme) << "(const std::vector<Value>& rvn_args);\n";
        out << "extern Value " << val_ident(st->name.lexeme) << ";\n";
    }
    out << "\n";

    // 3. Define Struct Constructors
    for (const auto* st : structs) {
        out << "Value " << fn_ident(st->name.lexeme) << "(const std::vector<Value>& rvn_args) {\n";
        out << "    auto m = std::make_shared<std::unordered_map<std::string, Value>>();\n";
        out << "    (*m)[\"__type__\"] = Value(" << escape_cpp_string(st->name.lexeme) << ");\n";
        for (size_t i = 0; i < st->fields.size(); ++i) {
            out << "    (*m)[" << escape_cpp_string(st->fields[i].lexeme)
                << "] = rvn_args.size() > " << i << " ? rvn_args[" << i << "] : Value();\n";
        }
        out << "    return Value(m);\n";
        out << "}\n";
        out << "Value " << val_ident(st->name.lexeme) << ";\n\n";
    }

    // 4. Define Function Implementations
    NativeExprEmitter expr_emitter(known_functions);
    for (const auto* fn : functions) {
        out << "Value " << fn_ident(fn->name.lexeme) << "(const std::vector<Value>& rvn_args) {\n";
        for (size_t i = 0; i < fn->params.size(); ++i) {
            std::string def_val = "Value()";
            if (i < fn->param_defaults.size() && fn->param_defaults[i]) {
                def_val = expr_emitter.emit(*fn->param_defaults[i]);
            }
            if (fn->is_variadic && i + 1 == fn->params.size()) {
                out << "    auto rvn_rest_vec = std::make_shared<std::vector<Value>>();\n";
                out << "    for (size_t _idx = " << i << "; _idx < rvn_args.size(); ++_idx) {\n";
                out << "        rvn_rest_vec->push_back(rvn_args[_idx]);\n";
                out << "    }\n";
                out << "    Value " << var_ident(fn->params[i].lexeme) << "(rvn_rest_vec);\n";
            } else {
                out << "    Value " << var_ident(fn->params[i].lexeme)
                    << " = rvn_args.size() > " << i << " ? rvn_args[" << i << "] : " << def_val << ";\n";
            }
        }

        NativeStmtEmitter stmt_emitter(out, known_functions, 1);
        for (const auto& body_stmt : fn->body) {
            stmt_emitter.emit(*body_stmt);
        }
        out << "    return Value();\n";
        out << "}\n";
        out << "Value " << val_ident(fn->name.lexeme) << ";\n\n";
    }

    // 5. Generate main()
    out << "int main(int argc, char* argv[]) {\n";
    out << "    rvn_rt::init(argc, argv);\n\n";

    // Bind function callable wrappers
    for (const auto* fn : functions) {
        int arity = static_cast<int>(fn->params.size());
        out << "    " << val_ident(fn->name.lexeme) << " = rvn_rt::make_callable("
            << escape_cpp_string(fn->name.lexeme) << ", " << arity
            << ", [](const std::vector<Value>& args) { return "
            << fn_ident(fn->name.lexeme) << "(args); });\n";
    }
    for (const auto* st : structs) {
        int arity = static_cast<int>(st->fields.size());
        out << "    " << val_ident(st->name.lexeme) << " = rvn_rt::make_callable("
            << escape_cpp_string(st->name.lexeme) << ", " << arity
            << ", [](const std::vector<Value>& args) { return "
            << fn_ident(st->name.lexeme) << "(args); });\n";
    }
    out << "\n";

    // Top-level statements inside try-catch block
    out << "    try {\n";
    {
        NativeStmtEmitter main_emitter(out, known_functions, 2);
        for (const auto* stmt : top_level_stmts) {
            main_emitter.emit(*stmt);
        }
    }
    out << "    } catch (const std::exception& e) {\n";
    out << "        std::cerr << \"Runtime error: \" << e.what() << \"\\n\";\n";
    out << "        return 70;\n";
    out << "    }\n\n";
    out << "    return 0;\n";
    out << "}\n";

    return out.str();
}

std::string NativeCompiler::find_host_compiler() {
    if (const char* env_cxx = std::getenv("CXX")) {
        if (std::string(env_cxx).find("g++") != std::string::npos ||
            std::string(env_cxx).find("clang++") != std::string::npos) {
            return env_cxx;
        }
    }

    std::vector<std::string> candidates = {"g++", "clang++", "c++"};
    for (const auto& comp : candidates) {
        std::string cmd = "which " + comp + " > /dev/null 2>&1";
        if (std::system(cmd.c_str()) == 0) {
            return comp;
        }
    }
    return "g++";
}

std::string NativeCompiler::find_raven_lib() {
    if (const char* env_lib = std::getenv("RAVEN_LIB_DIR")) {
        fs::path p = fs::path(env_lib) / "libraven_core.a";
        if (fs::exists(p)) return p.string();
    }

    std::vector<fs::path> candidates = {
        fs::current_path() / "build" / "libraven_core.a",
        fs::current_path() / "build" / "Release" / "libraven_core.a",
        fs::current_path() / "lib" / "libraven_core.a",
        fs::path("/usr/local/lib/libraven_core.a")
    };

    std::string exe_dir = platform::fs_executable_dir();
    if (!exe_dir.empty()) {
        candidates.push_back(fs::path(exe_dir) / "libraven_core.a");
        candidates.push_back(fs::path(exe_dir) / ".." / "lib" / "libraven_core.a");
        candidates.push_back(fs::path(exe_dir) / "build" / "libraven_core.a");
    }

    for (const auto& p : candidates) {
        if (fs::exists(p) && !fs::is_directory(p)) {
            return p.string();
        }
    }
    return "";
}

std::string NativeCompiler::find_raven_include_dir() {
    if (const char* env_inc = std::getenv("RAVEN_INCLUDE_DIR")) {
        if (fs::exists(env_inc)) return env_inc;
    }

    std::vector<fs::path> candidates = {
        fs::current_path() / "include",
        fs::current_path() / ".." / "include",
        fs::path("/usr/local/include/raven"),
        fs::path("/usr/local/include")
    };

    std::string exe_dir = platform::fs_executable_dir();
    if (!exe_dir.empty()) {
        candidates.push_back(fs::path(exe_dir) / "include");
        candidates.push_back(fs::path(exe_dir) / ".." / "include");
        candidates.push_back(fs::path(exe_dir) / ".." / ".." / "include");
    }

    for (const auto& p : candidates) {
        if (fs::exists(p / "raven" / "value.hpp")) {
            return p.string();
        }
    }
    return "";
}

bool NativeCompiler::compile_to_file(const std::string& input_path,
                                    const std::string& output_path,
                                    const NativeCompilerOptions& options) {
    std::ifstream file(input_path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open source file '" << input_path << "'\n";
        return false;
    }

    std::ostringstream ss;
    ss << file.rdbuf();

    Lexer lexer(ss.str());
    auto tokens = lexer.tokenize();

    Parser parser(tokens);
    auto statements = parser.parse();

    if (parser.has_error()) {
        for (const auto& err : parser.errors()) {
            std::cerr << err << "\n";
        }
        return false;
    }

    if (options.optimize) {
        statements = Optimizer::optimize(statements);
    }

    std::string cpp_code = compile_to_c(statements, fs::path(input_path).stem().string());

    if (options.emit_c) {
        std::string c_out = options.c_output_path.empty() ? output_path : options.c_output_path;
        if (c_out.empty()) {
            std::cout << cpp_code;
            return true;
        }
        std::ofstream out(c_out);
        if (!out.is_open()) {
            std::cerr << "Error: Failed to write C++ source to '" << c_out << "'\n";
            return false;
        }
        out << cpp_code;
        return true;
    }

    std::string host_comp = !options.host_compiler.empty() ? options.host_compiler : find_host_compiler();
    std::string raven_lib = find_raven_lib();
    std::string raven_inc = find_raven_include_dir();

    if (raven_lib.empty()) {
        std::cerr << "Error: Native compilation requires 'libraven_core.a'. Run CMake build first.\n";
        return false;
    }

    // Write generated C++ code to temporary file
    fs::path tmp_cpp = fs::temp_directory_path() / ("raven_native_" + std::to_string(std::rand()) + ".cpp");
    {
        std::ofstream out(tmp_cpp);
        if (!out.is_open()) {
            std::cerr << "Error: Failed to create temporary file for C++ generation.\n";
            return false;
        }
        out << cpp_code;
    }

    std::ostringstream cmd;
    cmd << host_comp << " -std=c++20 ";
    cmd << (options.optimize ? "-O3 " : "-O0 -g ");
    cmd << tmp_cpp.string() << " ";
    if (!raven_inc.empty()) {
        cmd << "-I\"" << raven_inc << "\" ";
    }
    for (const auto& inc : options.extra_include_dirs) {
        cmd << "-I\"" << inc << "\" ";
    }
    cmd << "\"" << raven_lib << "\" ";
    for (const auto& lib : options.extra_lib_dirs) {
        cmd << "-L\"" << lib << "\" ";
    }
    for (const auto& flag : options.extra_cxx_flags) {
        cmd << flag << " ";
    }
    cmd << "-lssl -lcrypto -lpthread -o \"" << output_path << "\"";

    if (options.verbose) {
        std::cout << "Executing: " << cmd.str() << "\n";
    }

    int ret = std::system(cmd.str().c_str());

    // Clean up temporary file
    std::error_code ec;
    fs::remove(tmp_cpp, ec);

    if (ret != 0) {
        std::cerr << "Error: Host C++ compilation failed with code " << ret << "\n";
        return false;
    }

#if !defined(_WIN32) && !defined(_WIN64)
    chmod(output_path.c_str(), 0755);
#endif

    return true;
}

} // namespace raven::native
