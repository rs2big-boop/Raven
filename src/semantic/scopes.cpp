#include "raven/semantic/scopes.hpp"

namespace raven::semantic {

bool Scope::define(Symbol sym) {
    if (symbols_.count(sym.name)) return false;
    symbols_.emplace(sym.name, std::move(sym));
    return true;
}

std::optional<Symbol> Scope::lookup(const std::string& name) const {
    auto it = symbols_.find(name);
    if (it != symbols_.end()) return it->second;
    if (parent_) return parent_->lookup(name);
    return std::nullopt;
}

std::optional<Symbol> Scope::lookup_local(const std::string& name) const {
    auto it = symbols_.find(name);
    if (it != symbols_.end()) return it->second;
    return std::nullopt;
}

ScopeChain::ScopeChain() {
    scopes_.push_back(std::make_shared<Scope>(nullptr, false));
}

void ScopeChain::push_scope(bool is_function) {
    auto parent = scopes_.empty() ? nullptr : scopes_.back();
    scopes_.push_back(std::make_shared<Scope>(parent, is_function));
}

void ScopeChain::pop_scope() {
    if (scopes_.size() > 1) scopes_.pop_back();
}

bool ScopeChain::define(Symbol sym) {
    return scopes_.back()->define(std::move(sym));
}

std::optional<Symbol> ScopeChain::lookup(const std::string& name) const {
    return scopes_.back()->lookup(name);
}

} // namespace raven::semantic
