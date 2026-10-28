#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "raven/token.hpp"
#include "raven/value.hpp"

namespace raven {

class RuntimeError : public std::runtime_error {
public:
    RuntimeError(Token token, const std::string& message)
        : std::runtime_error(message), token_(std::move(token)) {}

    const Token& token() const { return token_; }

private:
    Token token_;
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
