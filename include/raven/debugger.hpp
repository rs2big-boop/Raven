#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>

#include "raven/ast.hpp"
#include "raven/interpreter.hpp"
#include "raven/lexer.hpp"
#include "raven/parser.hpp"

namespace raven {

inline size_t extract_stmt_line(const Stmt& stmt) {
    if (auto* s = dynamic_cast<const VarDeclStmt*>(&stmt)) return s->name.line;
    if (auto* s = dynamic_cast<const DestructureStmt*>(&stmt)) {
        if (!s->names.empty()) return s->names[0].line;
    }
    if (auto* s = dynamic_cast<const FunctionStmt*>(&stmt)) return s->name.line;
    if (auto* s = dynamic_cast<const StructStmt*>(&stmt)) return s->name.line;
    if (auto* s = dynamic_cast<const ForInStmt*>(&stmt)) return s->variable.line;
    if (auto* s = dynamic_cast<const ThrowStmt*>(&stmt)) return s->keyword.line;
    if (auto* s = dynamic_cast<const ImportStmt*>(&stmt)) return s->keyword.line;
    if (auto* s = dynamic_cast<const BreakStmt*>(&stmt)) return s->keyword.line;
    if (auto* s = dynamic_cast<const ContinueStmt*>(&stmt)) return s->keyword.line;
    if (auto* s = dynamic_cast<const ReturnStmt*>(&stmt)) return s->keyword.line;
    if (auto* s = dynamic_cast<const IfStmt*>(&stmt)) {
        if (s->then_branch) return extract_stmt_line(*s->then_branch);
    }
    if (auto* s = dynamic_cast<const WhileStmt*>(&stmt)) {
        if (s->body) return extract_stmt_line(*s->body);
    }
    if (auto* s = dynamic_cast<const ExpressionStmt*>(&stmt)) {
        if (s->expression) {
            if (auto* e = dynamic_cast<const CallExpr*>(s->expression.get())) return e->paren.line;
            if (auto* e = dynamic_cast<const AssignExpr*>(s->expression.get())) return e->name.line;
            if (auto* e = dynamic_cast<const BinaryExpr*>(s->expression.get())) return e->op.line;
            if (auto* e = dynamic_cast<const VariableExpr*>(s->expression.get())) return e->name.line;
        }
    }
    if (auto* s = dynamic_cast<const PrintStmt*>(&stmt)) {
        if (s->expression) {
            if (auto* e = dynamic_cast<const CallExpr*>(s->expression.get())) return e->paren.line;
            if (auto* e = dynamic_cast<const VariableExpr*>(s->expression.get())) return e->name.line;
            if (auto* e = dynamic_cast<const BinaryExpr*>(s->expression.get())) return e->op.line;
        }
    }
    return 0;
}

class Debugger {
public:
    explicit Debugger(std::string filename, std::string source)
        : filename_(std::move(filename)), source_(std::move(source)) {
        std::istringstream stream(source_);
        std::string line;
        while (std::getline(stream, line)) {
            source_lines_.push_back(line);
        }
    }

    void add_breakpoint(size_t line) {
        breakpoints_.insert(line);
    }

    void remove_breakpoint(size_t line) {
        breakpoints_.erase(line);
    }

    void set_stop_on_entry(bool stop) {
        stop_on_entry_ = stop;
    }

    const std::set<size_t>& breakpoints() const { return breakpoints_; }

    int start_interactive() {
        std::cout << "🐦 Raven Interactive Debugger (rvndb) v1.1.0\n";
        std::cout << "Target: " << filename_ << " (" << source_lines_.size() << " lines)\n";
        if (!breakpoints_.empty()) {
            std::cout << "Active breakpoints: ";
            for (size_t bp : breakpoints_) std::cout << bp << " ";
            std::cout << "\n";
        }
        std::cout << "Type 'help' or 'h' for list of commands, 'run' to start.\n\n";

        Lexer lexer(source_);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto statements = parser.parse();

        if (parser.has_error()) {
            for (const auto& err : parser.errors()) {
                std::cerr << "Syntax Error: " << err << "\n";
            }
            return 1;
        }

        Interpreter interpreter;
        stepping_ = stop_on_entry_;
        step_over_ = false;

        interpreter.set_step_callback([this](Interpreter& interp, const Stmt& stmt) {
            this->on_step(interp, stmt);
        });

        // Initial prompt before running if stop on entry is requested
        if (stop_on_entry_) {
            print_code_context(1, 5);
            if (!repl_loop(interpreter, 1)) {
                return 0;
            }
        }

        try {
            interpreter.interpret(statements);
            std::cout << "\n[Program finished execution successfully]\n";
        } catch (const RuntimeError& err) {
            std::cerr << "\n[Runtime Error on line " << err.token().line << "]: " << err.what() << "\n";
            for (auto it = err.stack_trace().rbegin(); it != err.stack_trace().rend(); ++it) {
                std::cerr << "    at " << it->function_name << " (line " << it->line << ")\n";
            }
            return 70;
        } catch (const std::exception& e) {
            std::cerr << "\n[Fatal Exception]: " << e.what() << "\n";
            return 1;
        }
        return 0;
    }

private:
    void print_code_context(size_t current_line, int window = 3) {
        if (source_lines_.empty()) return;
        int cur = static_cast<int>(current_line);
        int start = std::max(1, cur - window);
        int end = std::min(static_cast<int>(source_lines_.size()), cur + window);

        for (int i = start; i <= end; ++i) {
            bool is_cur = (i == cur);
            bool is_bp = (breakpoints_.count(i) > 0);
            std::cout << (is_cur ? " -> " : "    ")
                      << (is_bp ? "● " : "  ")
                      << i << " | " << source_lines_[i - 1] << "\n";
        }
    }

    void on_step(Interpreter& interp, const Stmt& stmt) {
        size_t line = extract_stmt_line(stmt);
        if (line == 0) return;

        bool is_breakpoint = breakpoints_.count(line) > 0;
        bool should_pause = is_breakpoint || stepping_;

        if (step_over_) {
            if (interp.call_stack().size() <= step_over_depth_) {
                should_pause = true;
                step_over_ = false;
            } else {
                should_pause = is_breakpoint;
            }
        }

        if (should_pause) {
            if (is_breakpoint) {
                std::cout << "\n🛑 Hit Breakpoint at line " << line << ":\n";
            } else {
                std::cout << "\n📍 Stopped at line " << line << ":\n";
            }
            print_code_context(line, 2);
            repl_loop(interp, line);
        }
    }

    bool repl_loop(Interpreter& interp, size_t current_line) {
        std::string cmd;
        while (true) {
            std::cout << "(rvndb) ";
            std::cout.flush();
            if (!std::getline(std::cin, cmd)) {
                return false;
            }

            // Trim
            size_t start = cmd.find_first_not_of(" \t");
            if (start == std::string::npos) continue;
            cmd = cmd.substr(start);

            std::istringstream iss(cmd);
            std::string action;
            iss >> action;

            if (action == "c" || action == "continue" || action == "r" || action == "run") {
                stepping_ = false;
                step_over_ = false;
                return true;
            } else if (action == "s" || action == "step") {
                stepping_ = true;
                step_over_ = false;
                return true;
            } else if (action == "n" || action == "next") {
                stepping_ = false;
                step_over_ = true;
                step_over_depth_ = interp.call_stack().size();
                return true;
            } else if (action == "b" || action == "break") {
                int bp_line = 0;
                if (iss >> bp_line && bp_line > 0) {
                    add_breakpoint(static_cast<size_t>(bp_line));
                    std::cout << "Breakpoint set at line " << bp_line << "\n";
                } else {
                    std::cout << "Usage: break <line_number>\n";
                }
            } else if (action == "del" || action == "delete") {
                int bp_line = 0;
                if (iss >> bp_line && bp_line > 0) {
                    remove_breakpoint(static_cast<size_t>(bp_line));
                    std::cout << "Breakpoint removed from line " << bp_line << "\n";
                } else {
                    std::cout << "Usage: delete <line_number>\n";
                }
            } else if (action == "info") {
                std::string sub;
                iss >> sub;
                if (sub == "break" || sub == "b") {
                    std::cout << "Active Breakpoints:\n";
                    for (size_t bp : breakpoints_) {
                        std::cout << "  - Line " << bp << "\n";
                    }
                    if (breakpoints_.empty()) std::cout << "  (none)\n";
                }
            } else if (action == "p" || action == "print") {
                std::string expr_str;
                std::getline(iss, expr_str);
                size_t expr_start = expr_str.find_first_not_of(" \t");
                if (expr_start != std::string::npos) {
                    expr_str = expr_str.substr(expr_start);
                    // Evaluate variable or expression
                    Lexer l(expr_str + ";");
                    auto tok = l.tokenize();
                    Parser p(tok);
                    auto s = p.parse();
                    if (!p.has_error() && !s.empty()) {
                        if (auto* es = dynamic_cast<ExpressionStmt*>(s[0].get())) {
                            try {
                                Value res = interp.evaluate(*es->expression);
                                std::cout << "=> " << res.to_string() << "\n";
                            } catch (const std::exception& e) {
                                std::cout << "Error: " << e.what() << "\n";
                            }
                        }
                    } else {
                        std::cout << "Cannot parse expression.\n";
                    }
                }
            } else if (action == "locals") {
                std::cout << "Local Variables:\n";
                if (interp.environment()) {
                    for (const auto& [k, v] : interp.environment()->values()) {
                        std::cout << "  " << k << " = " << v.to_string() << " (" << v.type_name() << ")\n";
                    }
                }
            } else if (action == "bt" || action == "stack" || action == "backtrace") {
                std::cout << "Call Stack:\n";
                const auto& stack = interp.call_stack();
                for (size_t i = 0; i < stack.size(); ++i) {
                    std::cout << "  #" << i << " " << stack[i].function_name
                              << " at line " << stack[i].line << "\n";
                }
                std::cout << "  #" << stack.size() << " <main> at line " << current_line << "\n";
            } else if (action == "l" || action == "list") {
                print_code_context(current_line, 5);
            } else if (action == "q" || action == "quit") {
                std::cout << "Debugger session terminated.\n";
                std::exit(0);
            } else if (action == "h" || action == "help") {
                std::cout << "Raven Debugger (rvndb) Commands:\n"
                          << "  r, run, c, continue     Continue program execution\n"
                          << "  s, step                 Step into next statement\n"
                          << "  n, next                 Step over statement\n"
                          << "  b, break <line>         Set breakpoint at specified line\n"
                          << "  del, delete <line>      Delete breakpoint at specified line\n"
                          << "  info break              List all active breakpoints\n"
                          << "  p, print <expr>         Evaluate and print expression in current context\n"
                          << "  locals                  Print all variables in current environment\n"
                          << "  bt, backtrace, stack    Show execution call stack trace\n"
                          << "  l, list                 List source code around current line\n"
                          << "  q, quit                 Quit the debugger\n"
                          << "  h, help                 Show this command guide\n";
            } else {
                std::cout << "Unknown command: '" << action << "'. Type 'help' for available commands.\n";
            }
        }
    }

    std::string filename_;
    std::string source_;
    std::vector<std::string> source_lines_;
    std::set<size_t> breakpoints_;
    bool stepping_ = false;
    bool step_over_ = false;
    size_t step_over_depth_ = 0;
    bool stop_on_entry_ = true;
};

} // namespace raven
