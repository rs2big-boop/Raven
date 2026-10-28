#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "raven/token.hpp"
#include "raven/value.hpp"

namespace raven {

struct StackFrame {
    std::string function_name;
    size_t line = 0;
    size_t column = 0;
};

class RuntimeError : public std::runtime_error {
public:
    RuntimeError(Token token, const std::string& message, std::vector<StackFrame> stack_trace = {})
        : std::runtime_error(message), token_(std::move(token)), stack_trace_(std::move(stack_trace)) {}

    const Token& token() const { return token_; }
    const std::vector<StackFrame>& stack_trace() const { return stack_trace_; }
    void set_stack_trace(std::vector<StackFrame> trace) { stack_trace_ = std::move(trace); }

private:
    Token token_;
    std::vector<StackFrame> stack_trace_;
};

class Environment : public std::enable_shared_from_this<Environment> {
public:
    Environment() = default;
    explicit Environment(std::shared_ptr<Environment> enclosing)
        : enclosing_(std::move(enclosing)) {}

    void define(const std::string& name, Value value) {
        values_[name] = std::move(value);
    }

    Value get(const Token& name) {
        auto it = values_.find(name.lexeme);
        if (it != values_.end()) {
            return it->second;
        }

        if (enclosing_ != nullptr) {
            return enclosing_->get(name);
        }

        throw RuntimeError(name, "Undefined variable '" + name.lexeme + "'.");
    }

    void assign(const Token& name, Value value) {
        auto it = values_.find(name.lexeme);
        if (it != values_.end()) {
            it->second = std::move(value);
            return;
        }

        if (enclosing_ != nullptr) {
            enclosing_->assign(name, std::move(value));
            return;
        }

        throw RuntimeError(name, "Undefined variable '" + name.lexeme + "'.");
    }

    std::shared_ptr<Environment> enclosing() const { return enclosing_; }

private:
    std::unordered_map<std::string, Value> values_;
    std::shared_ptr<Environment> enclosing_ = nullptr;
};

} // namespace raven
