#pragma once

#include <cmath>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace raven {

class Interpreter;

struct Nil {};

class Callable;
class Value;

using ArrayPtr = std::shared_ptr<std::vector<Value>>;
using MapPtr = std::shared_ptr<std::unordered_map<std::string, Value>>;

using ValueBase = std::variant<
    Nil,
    bool,
    double,
    std::string,
    std::shared_ptr<Callable>,
    ArrayPtr,
    MapPtr
>;

class Value {
public:
    Value() : data_(Nil{}) {}
    Value(Nil) : data_(Nil{}) {}
    Value(bool b) : data_(b) {}
    Value(double d) : data_(d) {}
    Value(int i) : data_(static_cast<double>(i)) {}
    Value(std::string s) : data_(std::move(s)) {}
    Value(const char* s) : data_(std::string(s)) {}
    Value(std::shared_ptr<Callable> c) : data_(std::move(c)) {}
    Value(ArrayPtr arr) : data_(std::move(arr)) {}
    Value(MapPtr m) : data_(std::move(m)) {}

    bool is_nil() const { return std::holds_alternative<Nil>(data_); }
    bool is_bool() const { return std::holds_alternative<bool>(data_); }
    bool is_number() const { return std::holds_alternative<double>(data_); }
    bool is_string() const { return std::holds_alternative<std::string>(data_); }
    bool is_callable() const { return std::holds_alternative<std::shared_ptr<Callable>>(data_); }
    bool is_array() const { return std::holds_alternative<ArrayPtr>(data_); }
    bool is_map() const { return std::holds_alternative<MapPtr>(data_); }

    bool as_bool() const { return std::get<bool>(data_); }
    double as_number() const { return std::get<double>(data_); }
    const std::string& as_string() const { return std::get<std::string>(data_); }
    std::shared_ptr<Callable> as_callable() const { return std::get<std::shared_ptr<Callable>>(data_); }
    ArrayPtr as_array() const { return std::get<ArrayPtr>(data_); }
    MapPtr as_map() const { return std::get<MapPtr>(data_); }

    std::string type_name() const {
        if (is_nil()) return "nil";
        if (is_bool()) return "bool";
        if (is_number()) return "number";
        if (is_string()) return "string";
        if (is_callable()) return "function";
        if (is_array()) return "array";
        if (is_map()) return "map";
        return "unknown";
    }

    bool is_truthy() const {
        if (is_nil()) return false;
        if (is_bool()) return as_bool();
        if (is_number()) return as_number() != 0.0;
        if (is_string()) return !as_string().empty();
        if (is_array()) return !as_array()->empty();
        if (is_map()) return !as_map()->empty();
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
            s.erase(s.find_last_not_of('0') + 1, std::string::npos);
            if (s.back() == '.') s.pop_back();
            return s;
        }
        if (is_string()) return as_string();
        if (is_array()) {
            const auto& vec = *as_array();
            std::string res = "[";
            for (size_t i = 0; i < vec.size(); ++i) {
                if (vec[i].is_string()) {
                    res += "\"" + vec[i].as_string() + "\"";
                } else {
                    res += vec[i].to_string();
                }
                if (i + 1 < vec.size()) res += ", ";
            }
            res += "]";
            return res;
        }
        if (is_map()) {
            const auto& m = *as_map();
            std::string res = "{";
            size_t idx = 0;
            for (const auto& [k, v] : m) {
                res += "\"" + k + "\": ";
                if (v.is_string()) {
                    res += "\"" + v.as_string() + "\"";
                } else {
                    res += v.to_string();
                }
                if (++idx < m.size()) res += ", ";
            }
            res += "}";
            return res;
        }
        return "<callable>";
    }

    bool operator==(const Value& other) const {
        if (data_.index() != other.data_.index()) return false;
        if (is_nil()) return true;
        if (is_bool()) return as_bool() == other.as_bool();
        if (is_number()) return as_number() == other.as_number();
        if (is_string()) return as_string() == other.as_string();
        if (is_array()) {
            const auto& a = *as_array();
            const auto& b = *other.as_array();
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i) {
                if (a[i] != b[i]) return false;
            }
            return true;
        }
        if (is_map()) {
            const auto& a = *as_map();
            const auto& b = *other.as_map();
            if (a.size() != b.size()) return false;
            for (const auto& [k, v] : a) {
                auto it = b.find(k);
                if (it == b.end() || it->second != v) return false;
            }
            return true;
        }
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
    virtual int min_arity() const { return arity(); }
    virtual Value call(Interpreter& interpreter, const std::vector<Value>& arguments) = 0;
    virtual std::string to_string() const = 0;
};

} // namespace raven
