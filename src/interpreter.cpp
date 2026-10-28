#include "raven/interpreter.hpp"

#include <chrono>
#include <functional>

namespace raven {

class BuiltinFunction : public Callable {
public:
    using NativeFn = std::function<Value(Interpreter&, const std::vector<Value>&)>;

    BuiltinFunction(std::string name, int arity, NativeFn fn)
        : name_(std::move(name)), arity_(arity), fn_(std::move(fn)) {}

    int arity() const override { return arity_; }

    Value call(Interpreter& interp, const std::vector<Value>& args) override {
        return fn_(interp, args);
    }

    std::string to_string() const override {
        return "<native fn " + name_ + ">";
    }

private:
    std::string name_;
    int arity_;
    NativeFn fn_;
};

class UserFunction : public Callable {
public:
    UserFunction(Token name, std::vector<Token> params, std::vector<std::shared_ptr<Stmt>> body, std::shared_ptr<Environment> closure)
        : name_(std::move(name)), params_(std::move(params)), body_(std::move(body)), closure_(std::move(closure)) {}

    int arity() const override {
        return static_cast<int>(params_.size());
    }

    Value call(Interpreter& interpreter, const std::vector<Value>& arguments) override {
        auto env = std::make_shared<Environment>(closure_);
        for (std::size_t i = 0; i < params_.size(); ++i) {
            env->define(params_[i].lexeme, arguments[i]);
        }

        try {
            interpreter.execute_block(body_, env);
        } catch (const ReturnException& ret) {
            return ret.value;
        }

        return Value();
    }

    std::string to_string() const override {
        return "<fn " + name_.lexeme + ">";
    }

private:
    Token name_;
    std::vector<Token> params_;
    std::vector<std::shared_ptr<Stmt>> body_;
    std::shared_ptr<Environment> closure_;
};

Interpreter::Interpreter(std::ostream& out)
    : out_(out),
      globals_(std::make_shared<Environment>()),
      environment_(globals_) {
    register_builtins();
}

void Interpreter::register_builtins() {
    auto def = [this](const std::string& name, int arity, BuiltinFunction::NativeFn fn) {
        globals_->define(name, Value(std::make_shared<BuiltinFunction>(name, arity, std::move(fn))));
    };

    def("len", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        const auto& arg = args[0];
        if (arg.is_string()) return Value(static_cast<double>(arg.as_string().size()));
        if (arg.is_array()) return Value(static_cast<double>(arg.as_array()->size()));
        if (arg.is_map()) return Value(static_cast<double>(arg.as_map()->size()));
        throw RuntimeError(Token{TokenType::Identifier, "len", 0, 0}, "len() expects string, array, or map.");
    });

    def("push", 2, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_array()) {
            throw RuntimeError(Token{TokenType::Identifier, "push", 0, 0}, "push() expects array as first argument.");
        }
        args[0].as_array()->push_back(args[1]);
        return args[1];
    });

    def("pop", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_array()) {
            throw RuntimeError(Token{TokenType::Identifier, "pop", 0, 0}, "pop() expects array as argument.");
        }
        auto& arr = *args[0].as_array();
        if (arr.empty()) return Value();
        Value val = arr.back();
        arr.pop_back();
        return val;
    });

    def("keys", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_map()) {
            throw RuntimeError(Token{TokenType::Identifier, "keys", 0, 0}, "keys() expects map as argument.");
        }
        auto res = std::make_shared<std::vector<Value>>();
        for (const auto& [k, v] : *args[0].as_map()) {
            res->push_back(Value(k));
        }
        return Value(res);
    });

    def("values", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_map()) {
            throw RuntimeError(Token{TokenType::Identifier, "values", 0, 0}, "values() expects map as argument.");
        }
        auto res = std::make_shared<std::vector<Value>>();
        for (const auto& [k, v] : *args[0].as_map()) {
            res->push_back(v);
        }
        return Value(res);
    });

    def("range", 2, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_number() || !args[1].is_number()) {
            throw RuntimeError(Token{TokenType::Identifier, "range", 0, 0}, "range() expects two numbers.");
        }
        auto res = std::make_shared<std::vector<Value>>();
        double start = args[0].as_number();
        double end = args[1].as_number();
        for (double i = start; i < end; i += 1.0) {
            res->push_back(Value(i));
        }
        return Value(res);
    });

    def("map", 2, [](Interpreter& interp, const std::vector<Value>& args) -> Value {
        if (!args[0].is_array() || !args[1].is_callable()) {
            throw RuntimeError(Token{TokenType::Identifier, "map", 0, 0}, "map() expects array and function.");
        }
        auto res = std::make_shared<std::vector<Value>>();
        auto fn = args[1].as_callable();
        for (const auto& item : *args[0].as_array()) {
            res->push_back(fn->call(interp, {item}));
        }
        return Value(res);
    });

    def("filter", 2, [](Interpreter& interp, const std::vector<Value>& args) -> Value {
        if (!args[0].is_array() || !args[1].is_callable()) {
            throw RuntimeError(Token{TokenType::Identifier, "filter", 0, 0}, "filter() expects array and function.");
        }
        auto res = std::make_shared<std::vector<Value>>();
        auto fn = args[1].as_callable();
        for (const auto& item : *args[0].as_array()) {
            if (fn->call(interp, {item}).is_truthy()) {
                res->push_back(item);
            }
        }
        return Value(res);
    });

    def("reduce", 3, [](Interpreter& interp, const std::vector<Value>& args) -> Value {
        if (!args[0].is_array() || !args[1].is_callable()) {
            throw RuntimeError(Token{TokenType::Identifier, "reduce", 0, 0}, "reduce() expects array, function, and initial value.");
        }
        auto fn = args[1].as_callable();
        Value acc = args[2];
        for (const auto& item : *args[0].as_array()) {
            acc = fn->call(interp, {acc, item});
        }
        return acc;
    });

    def("type", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        return Value(args[0].type_name());
    });

    def("time", 0, [](Interpreter&, const std::vector<Value>&) -> Value {
        auto now = std::chrono::system_clock::now().time_since_epoch();
        double ms = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
        return Value(ms);
    });

    def("assert", 2, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_truthy()) {
            throw RuntimeError(Token{TokenType::Identifier, "assert", 0, 0}, "Assertion failed: " + args[1].to_string());
        }
        return Value(true);
    });

    def("str", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        return Value(args[0].to_string());
    });

    def("num", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (args[0].is_number()) return args[0];
        if (args[0].is_string()) {
            try {
                return Value(std::stod(args[0].as_string()));
            } catch (...) {
                return Value(0.0);
            }
        }
        return Value(0.0);
    });
}

void Interpreter::interpret(const std::vector<std::shared_ptr<Stmt>>& statements) {
    for (const auto& stmt : statements) {
        if (stmt) {
            execute(*stmt);
        }
    }
}

void Interpreter::execute(const Stmt& stmt) {
    stmt.accept(*this);
}

Value Interpreter::evaluate(const Expr& expr) {
    return expr.accept(*this);
}

void Interpreter::execute_block(const std::vector<std::shared_ptr<Stmt>>& statements,
                                std::shared_ptr<Environment> environment) {
    auto previous = this->environment_;
    try {
        this->environment_ = environment;
        for (const auto& stmt : statements) {
            execute(*stmt);
        }
        this->environment_ = previous;
    } catch (...) {
        this->environment_ = previous;
        throw;
    }
}

Value Interpreter::visit(const BinaryExpr& expr) {
    Value left = evaluate(*expr.left);
    Value right = evaluate(*expr.right);

    switch (expr.op.type) {
        case TokenType::Plus:
            if (left.is_number() && right.is_number()) {
                return Value(left.as_number() + right.as_number());
            }
            if (left.is_string() || right.is_string()) {
                return Value(left.to_string() + right.to_string());
            }
            if (left.is_array() && right.is_array()) {
                auto combined = std::make_shared<std::vector<Value>>(*left.as_array());
                combined->insert(combined->end(), right.as_array()->begin(), right.as_array()->end());
                return Value(combined);
            }
            throw RuntimeError(expr.op, "Operands must be two numbers, strings, or arrays.");

        case TokenType::Minus:
            check_number_operands(expr.op, left, right);
            return Value(left.as_number() - right.as_number());

        case TokenType::Star:
            check_number_operands(expr.op, left, right);
            return Value(left.as_number() * right.as_number());

        case TokenType::Slash:
            check_number_operands(expr.op, left, right);
            if (right.as_number() == 0.0) {
                throw RuntimeError(expr.op, "Division by zero.");
            }
            return Value(left.as_number() / right.as_number());

        case TokenType::Percent:
            check_number_operands(expr.op, left, right);
            if (right.as_number() == 0.0) {
                throw RuntimeError(expr.op, "Modulo by zero.");
            }
            return Value(std::fmod(left.as_number(), right.as_number()));

        case TokenType::Greater:
            if (left.is_number() && right.is_number()) return Value(left.as_number() > right.as_number());
            if (left.is_string() && right.is_string()) return Value(left.as_string() > right.as_string());
            throw RuntimeError(expr.op, "Operands must be two numbers or two strings.");

        case TokenType::GreaterEqual:
            if (left.is_number() && right.is_number()) return Value(left.as_number() >= right.as_number());
            if (left.is_string() && right.is_string()) return Value(left.as_string() >= right.as_string());
            throw RuntimeError(expr.op, "Operands must be two numbers or two strings.");

        case TokenType::Less:
            if (left.is_number() && right.is_number()) return Value(left.as_number() < right.as_number());
            if (left.is_string() && right.is_string()) return Value(left.as_string() < right.as_string());
            throw RuntimeError(expr.op, "Operands must be two numbers or two strings.");

        case TokenType::LessEqual:
            if (left.is_number() && right.is_number()) return Value(left.as_number() <= right.as_number());
            if (left.is_string() && right.is_string()) return Value(left.as_string() <= right.as_string());
            throw RuntimeError(expr.op, "Operands must be two numbers or two strings.");

        case TokenType::EqualEqual:
            return Value(left == right);

        case TokenType::BangEqual:
            return Value(left != right);

        default:
            break;
    }

    return Value();
}

Value Interpreter::visit(const GroupingExpr& expr) {
    return evaluate(*expr.expression);
}

Value Interpreter::visit(const LiteralExpr& expr) {
    return expr.value;
}

Value Interpreter::visit(const UnaryExpr& expr) {
    Value right = evaluate(*expr.right);

    switch (expr.op.type) {
        case TokenType::Bang:
            return Value(!right.is_truthy());
        case TokenType::Minus:
            check_number_operand(expr.op, right);
            return Value(-right.as_number());
        default:
            break;
    }

    return Value();
}

Value Interpreter::visit(const VariableExpr& expr) {
    return environment_->get(expr.name);
}

Value Interpreter::visit(const AssignExpr& expr) {
    Value value = evaluate(*expr.value);
    environment_->assign(expr.name, value);
    return value;
}

Value Interpreter::visit(const LogicalExpr& expr) {
    Value left = evaluate(*expr.left);

    if (expr.op.type == TokenType::PipePipe) {
        if (left.is_truthy()) return left;
    } else if (expr.op.type == TokenType::AmpAmp) {
        if (!left.is_truthy()) return left;
    }

    return evaluate(*expr.right);
}

Value Interpreter::visit(const CallExpr& expr) {
    Value callee = evaluate(*expr.callee);

    std::vector<Value> arguments;
    for (const auto& argument : expr.arguments) {
        arguments.push_back(evaluate(*argument));
    }

    if (!callee.is_callable()) {
        throw RuntimeError(expr.paren, "Can only call functions.");
    }

    auto function = callee.as_callable();
    if (function->arity() >= 0 && arguments.size() != static_cast<std::size_t>(function->arity())) {
        throw RuntimeError(expr.paren, "Expected " +
            std::to_string(function->arity()) + " arguments but got " +
            std::to_string(arguments.size()) + ".");
    }

    return function->call(*this, arguments);
}

Value Interpreter::visit(const ArrayExpr& expr) {
    auto list = std::make_shared<std::vector<Value>>();
    for (const auto& elem : expr.elements) {
        list->push_back(evaluate(*elem));
    }
    return Value(list);
}

Value Interpreter::visit(const MapExpr& expr) {
    auto map = std::make_shared<std::unordered_map<std::string, Value>>();
    for (const auto& [k, v] : expr.entries) {
        Value key_val = evaluate(*k);
        Value val_val = evaluate(*v);
        (*map)[key_val.to_string()] = val_val;
    }
    return Value(map);
}

Value Interpreter::visit(const IndexExpr& expr) {
    Value obj = evaluate(*expr.object);
    Value idx = evaluate(*expr.index);

    if (obj.is_array()) {
        if (!idx.is_number()) {
            throw RuntimeError(expr.bracket, "Array index must be a number.");
        }
        int i = static_cast<int>(idx.as_number());
        auto& arr = *obj.as_array();
        if (i < 0) i += static_cast<int>(arr.size());
        if (i < 0 || i >= static_cast<int>(arr.size())) {
            throw RuntimeError(expr.bracket, "Index out of bounds: " + std::to_string(i));
        }
        return arr[i];
    }

    if (obj.is_map()) {
        std::string key = idx.is_string() ? idx.as_string() : idx.to_string();
        auto& m = *obj.as_map();
        auto it = m.find(key);
        if (it == m.end()) return Value();
        return it->second;
    }

    if (obj.is_string()) {
        if (!idx.is_number()) {
            throw RuntimeError(expr.bracket, "String index must be a number.");
        }
        int i = static_cast<int>(idx.as_number());
        const auto& s = obj.as_string();
        if (i < 0) i += static_cast<int>(s.size());
        if (i < 0 || i >= static_cast<int>(s.size())) {
            throw RuntimeError(expr.bracket, "Index out of bounds: " + std::to_string(i));
        }
        return Value(std::string(1, s[i]));
    }

    throw RuntimeError(expr.bracket, "Only arrays, maps, and strings can be indexed.");
}

Value Interpreter::visit(const IndexAssignExpr& expr) {
    Value obj = evaluate(*expr.object);
    Value idx = evaluate(*expr.index);
    Value val = evaluate(*expr.value);

    if (obj.is_array()) {
        if (!idx.is_number()) {
            throw RuntimeError(expr.bracket, "Array index must be a number.");
        }
        int i = static_cast<int>(idx.as_number());
        auto& arr = *obj.as_array();
        if (i < 0) i += static_cast<int>(arr.size());
        if (i < 0 || i >= static_cast<int>(arr.size())) {
            throw RuntimeError(expr.bracket, "Index out of bounds: " + std::to_string(i));
        }
        arr[i] = val;
        return val;
    }

    if (obj.is_map()) {
        std::string key = idx.is_string() ? idx.as_string() : idx.to_string();
        (*obj.as_map())[key] = val;
        return val;
    }

    throw RuntimeError(expr.bracket, "Only arrays and maps support index assignment.");
}

Value Interpreter::visit(const PropertyExpr& expr) {
    Value obj = evaluate(*expr.object);
    if (obj.is_map()) {
        auto& m = *obj.as_map();
        auto it = m.find(expr.name.lexeme);
        if (it == m.end()) return Value();
        return it->second;
    }
    throw RuntimeError(expr.name, "Property access requires a map/object.");
}

Value Interpreter::visit(const PropertyAssignExpr& expr) {
    Value obj = evaluate(*expr.object);
    Value val = evaluate(*expr.value);
    if (obj.is_map()) {
        (*obj.as_map())[expr.name.lexeme] = val;
        return val;
    }
    throw RuntimeError(expr.name, "Property assignment requires a map/object.");
}

Value Interpreter::visit(const LambdaExpr& expr) {
    return Value(std::make_shared<UserFunction>(
        Token{TokenType::Identifier, "<lambda>", 0, 0},
        expr.params,
        expr.body,
        environment_
    ));
}

Value Interpreter::visit(const PipelineExpr& expr) {
    Value left = evaluate(*expr.left);
    Value right = evaluate(*expr.right);

    if (!right.is_callable()) {
        throw RuntimeError(Token{TokenType::PipeGreater, "|>", 0, 0}, "Right side of pipeline must be a callable.");
    }

    return right.as_callable()->call(*this, {left});
}

void Interpreter::visit(const ExpressionStmt& stmt) {
    evaluate(*stmt.expression);
}

void Interpreter::visit(const PrintStmt& stmt) {
    Value value = evaluate(*stmt.expression);
    out_ << value.to_string() << "\n";
}

void Interpreter::visit(const VarDeclStmt& stmt) {
    Value value;
    if (stmt.initializer != nullptr) {
        value = evaluate(*stmt.initializer);
    }
    environment_->define(stmt.name.lexeme, value);
}

void Interpreter::visit(const BlockStmt& stmt) {
    execute_block(stmt.statements, std::make_shared<Environment>(environment_));
}

void Interpreter::visit(const IfStmt& stmt) {
    if (evaluate(*stmt.condition).is_truthy()) {
        execute(*stmt.then_branch);
    } else if (stmt.else_branch != nullptr) {
        execute(*stmt.else_branch);
    }
}

void Interpreter::visit(const WhileStmt& stmt) {
    while (evaluate(*stmt.condition).is_truthy()) {
        execute(*stmt.body);
    }
}

void Interpreter::visit(const ForInStmt& stmt) {
    Value iterable = evaluate(*stmt.iterable);

    if (iterable.is_array()) {
        const auto& arr = *iterable.as_array();
        for (const auto& item : arr) {
            auto loop_env = std::make_shared<Environment>(environment_);
            loop_env->define(stmt.variable.lexeme, item);
            execute_block({stmt.body}, loop_env);
        }
    } else if (iterable.is_map()) {
        const auto& m = *iterable.as_map();
        for (const auto& [k, v] : m) {
            auto loop_env = std::make_shared<Environment>(environment_);
            loop_env->define(stmt.variable.lexeme, Value(k));
            execute_block({stmt.body}, loop_env);
        }
    } else {
        throw RuntimeError(stmt.variable, "Can only iterate over arrays or maps.");
    }
}

void Interpreter::visit(const TryCatchStmt& stmt) {
    try {
        execute(*stmt.try_block);
    } catch (const UserException& ue) {
        auto catch_env = std::make_shared<Environment>(environment_);
        catch_env->define(stmt.error_var.lexeme, ue.value);
        execute_block({stmt.catch_block}, catch_env);
    } catch (const RuntimeError& re) {
        auto catch_env = std::make_shared<Environment>(environment_);
        catch_env->define(stmt.error_var.lexeme, Value(re.what()));
        execute_block({stmt.catch_block}, catch_env);
    }
}

void Interpreter::visit(const ThrowStmt& stmt) {
    Value val = evaluate(*stmt.value);
    throw UserException{val};
}

void Interpreter::visit(const FunctionStmt& stmt) {
    auto function = std::make_shared<UserFunction>(
        stmt.name,
        stmt.params,
        stmt.body,
        environment_
    );
    environment_->define(stmt.name.lexeme, Value(function));
}

void Interpreter::visit(const ReturnStmt& stmt) {
    Value value;
    if (stmt.value != nullptr) {
        value = evaluate(*stmt.value);
    }
    throw ReturnException{value};
}

void Interpreter::check_number_operand(const Token& op, const Value& operand) {
    if (operand.is_number()) return;
    throw RuntimeError(op, "Operand must be a number.");
}

void Interpreter::check_number_operands(const Token& op, const Value& left, const Value& right) {
    if (left.is_number() && right.is_number()) return;
    throw RuntimeError(op, "Operands must be numbers.");
}

} // namespace raven
