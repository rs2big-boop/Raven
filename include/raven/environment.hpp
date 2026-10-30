#pragma once

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

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

inline int levenshtein_distance(const std::string& s1, const std::string& s2) {
    const size_t m = s1.size();
    const size_t n = s2.size();
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1, 0));
    for (size_t i = 0; i <= m; ++i) dp[i][0] = static_cast<int>(i);
    for (size_t j = 0; j <= n; ++j) dp[0][j] = static_cast<int>(j);

    for (size_t i = 1; i <= m; ++i) {
        for (size_t j = 1; j <= n; ++j) {
            if (s1[i - 1] == s2[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1];
            } else {
                dp[i][j] = 1 + std::min({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]});
            }
        }
    }
    return dp[m][n];
}

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

        // Variable not found in any scope; find closest suggestion
        std::string suggestion = find_closest_name(name.lexeme);
        std::string msg = "Undefined variable '" + name.lexeme + "'.";
        if (!suggestion.empty()) {
            msg += " Did you mean '" + suggestion + "'?";
        }
        throw RuntimeError(name, msg);
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

        std::string suggestion = find_closest_name(name.lexeme);
        std::string msg = "Undefined variable '" + name.lexeme + "'.";
        if (!suggestion.empty()) {
            msg += " Did you mean '" + suggestion + "'?";
        }
        throw RuntimeError(name, msg);
    }

    std::shared_ptr<Environment> enclosing() const { return enclosing_; }

    void collect_names(std::vector<std::string>& out) const {
        for (const auto& [k, _] : values_) {
            out.push_back(k);
        }
        if (enclosing_ != nullptr) {
            enclosing_->collect_names(out);
        }
    }

    std::string find_closest_name(const std::string& target) const {
        std::vector<std::string> all_names;
        collect_names(all_names);
        std::string best_match;
        int min_dist = 4; // Threshold: only suggest if edit distance <= 3
        for (const auto& candidate : all_names) {
            int d = levenshtein_distance(target, candidate);
            if (d < min_dist) {
                min_dist = d;
                best_match = candidate;
            }
        }
        return best_match;
    }

private:
    std::unordered_map<std::string, Value> values_;
    std::shared_ptr<Environment> enclosing_ = nullptr;
};

} // namespace raven
