#pragma once

#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace minilang {

class Interpreter;

struct Nil {};

class Callable;

using ValueBase = std::variant<
    Nil,
    bool,
    double,
    std::string,
    std::shared_ptr<Callable>
>;

class Value {
public:
    Value() : data_(Nil{}) {}
    Value(Nil) : data_(Nil{}) {}
    Value(bool b) : data_(b) {}
    Value(double d) : data_(d) {}
    Value(std::string s) : data_(std::move(s)) {}
    Value(const char* s) : data_(std::string(s)) {}
    Value(std::shared_ptr<Callable> c) : data_(std::move(c)) {}

    bool is_nil() const { return std::holds_alternative<Nil>(data_); }
    bool is_bool() const { return std::holds_alternative<bool>(data_); }
    bool is_number() const { return std::holds_alternative<double>(data_); }
    bool is_string() const { return std::holds_alternative<std::string>(data_); }
    bool is_callable() const { return std::holds_alternative<std::shared_ptr<Callable>>(data_); }

    bool as_bool() const { return std::get<bool>(data_); }
    double as_number() const { return std::get<double>(data_); }
    const std::string& as_string() const { return std::get<std::string>(data_); }
    std::shared_ptr<Callable> as_callable() const { return std::get<std::shared_ptr<Callable>>(data_); }

    bool is_truthy() const {
        if (is_nil()) return false;
        if (is_bool()) return as_bool();
        if (is_number()) return as_number() != 0.0;
        if (is_string()) return !as_string().empty();
        return true;
    }

    std::string to_string() const {
        if (is_nil()) return "nil";
        if (is_bool()) return as_bool() ? "true" : "false";
        if (is_number()) {
            double n = as_number();
            if (std::floor(n) == n && !std::isinf(n)) {
                return std::to_string(static_cast<long long>(n));
            }
            std::string s = std::to_string(n);
            // Trim trailing zeros for decimals
            s.erase(s.find_last_not_of('0') + 1, std::string::npos);
            if (s.back() == '.') s.pop_back();
            return s;
        }
        if (is_string()) return as_string();
        return "<callable>";
    }

    bool operator==(const Value& other) const {
        if (data_.index() != other.data_.index()) return false;
        if (is_nil()) return true;
        if (is_bool()) return as_bool() == other.as_bool();
        if (is_number()) return as_number() == other.as_number();
        if (is_string()) return as_string() == other.as_string();
        return false;
    }

    bool operator!=(const Value& other) const {
        return !(*this == other);
    }

private:
    ValueBase data_;
};

class Callable {
public:
    virtual ~Callable() = default;
    virtual int arity() const = 0;
    virtual Value call(Interpreter& interpreter, const std::vector<Value>& arguments) = 0;
    virtual std::string to_string() const = 0;
};

} // namespace minilang
