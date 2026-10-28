#include "raven/interpreter.hpp"

namespace raven {

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
      environment_(globals_) {}

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
            throw RuntimeError(expr.op, "Operands must be two numbers or strings.");

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
            if (left.is_number() && right.is_number()) {
                return Value(left.as_number() > right.as_number());
            }
            if (left.is_string() && right.is_string()) {
                return Value(left.as_string() > right.as_string());
            }
            throw RuntimeError(expr.op, "Operands must be two numbers or two strings.");

        case TokenType::GreaterEqual:
            if (left.is_number() && right.is_number()) {
                return Value(left.as_number() >= right.as_number());
            }
            if (left.is_string() && right.is_string()) {
                return Value(left.as_string() >= right.as_string());
            }
            throw RuntimeError(expr.op, "Operands must be two numbers or two strings.");

        case TokenType::Less:
            if (left.is_number() && right.is_number()) {
                return Value(left.as_number() < right.as_number());
            }
            if (left.is_string() && right.is_string()) {
                return Value(left.as_string() < right.as_string());
            }
            throw RuntimeError(expr.op, "Operands must be two numbers or two strings.");

        case TokenType::LessEqual:
            if (left.is_number() && right.is_number()) {
                return Value(left.as_number() <= right.as_number());
            }
            if (left.is_string() && right.is_string()) {
                return Value(left.as_string() <= right.as_string());
            }
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
    if (arguments.size() != static_cast<std::size_t>(function->arity())) {
        throw RuntimeError(expr.paren, "Expected " +
            std::to_string(function->arity()) + " arguments but got " +
            std::to_string(arguments.size()) + ".");
    }

    return function->call(*this, arguments);
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
