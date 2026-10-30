#pragma once
#include "symbols.hpp"
#include <memory>
#include <unordered_map>
#include <vector>
#include <optional>

namespace raven::semantic {

class Scope {
public:
    explicit Scope(std::shared_ptr<Scope> parent = nullptr, bool is_function = false)
        : parent_(std::move(parent)), is_function_scope_(is_function) {}

    bool define(Symbol sym);
    std::optional<Symbol> lookup(const std::string& name) const;
    std::optional<Symbol> lookup_local(const std::string& name) const;

    bool is_function_scope() const { return is_function_scope_; }
    const std::shared_ptr<Scope>& parent() const { return parent_; }
    const std::unordered_map<std::string, Symbol>& symbols() const { return symbols_; }
    size_t symbol_count() const { return symbols_.size(); }

private:
    std::shared_ptr<Scope> parent_;
    std::unordered_map<std::string, Symbol> symbols_;
    bool is_function_scope_;
};

class ScopeChain {
public:
    ScopeChain();

    void push_scope(bool is_function = false);
    void pop_scope();

    bool define(Symbol sym);
    std::optional<Symbol> lookup(const std::string& name) const;

    int current_depth() const { return static_cast<int>(scopes_.size()) - 1; }
    const Scope& current() const { return *scopes_.back(); }

private:
    std::vector<std::shared_ptr<Scope>> scopes_;
};

} // namespace raven::semantic
