#pragma once

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <functional>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "raven/value.hpp"
#include "raven/interpreter.hpp"
#include "raven/platform/env.hpp"
#include "raven/platform/filesystem.hpp"

namespace raven::native::rt {

inline Interpreter& get_interpreter() {
    static std::ostringstream* null_out = new std::ostringstream();
    static Interpreter* interp = []() {
        auto* it = new Interpreter(*null_out);
        auto env_path = platform::find_env_file(".");
        if (env_path) {
            platform::load_env_file(*env_path);
        }
        return it;
    }();
    return *interp;
}

inline void init(int argc, char* argv[]) {
    auto& interp = get_interpreter();
    auto args_vec = std::make_shared<std::vector<Value>>();
    for (int i = 0; i < argc; ++i) {
        args_vec->push_back(Value(std::string(argv[i])));
    }
    interp.globals()->define("args", Value(args_vec));
    auto env_path = platform::find_env_file(".");
    if (env_path) {
        platform::load_env_file(*env_path);
    }
}

class NativeFunction : public Callable {
public:
    using NativeFn = std::function<Value(const std::vector<Value>&)>;

    NativeFunction(std::string name, int arity, NativeFn fn)
        : name_(std::move(name)), arity_(arity), fn_(std::move(fn)) {}

    int arity() const override { return arity_; }
    int min_arity() const override { return arity_; }

    Value call(Interpreter&, const std::vector<Value>& args) override {
        return fn_(args);
    }

    std::string to_string() const override {
        return "<native fn " + (name_.empty() ? "anonymous" : name_) + ">";
    }

private:
    std::string name_;
    int arity_;
    NativeFn fn_;
};

inline Value make_callable(std::string name, int arity, NativeFunction::NativeFn fn) {
    return Value(std::make_shared<NativeFunction>(std::move(name), arity, std::move(fn)));
}

// Built-in print implementation
inline void print(const std::vector<Value>& values, bool newline = true) {
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) std::cout << " ";
        std::cout << values[i].to_string();
    }
    if (newline) {
        std::cout << "\n";
    }
    std::cout.flush();
}

inline void print(const Value& val, bool newline = true) {
    std::cout << val.to_string();
    if (newline) {
        std::cout << "\n";
    }
    std::cout.flush();
}

// Arithmetic Operations
inline Value add(const Value& left, const Value& right) {
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
    throw std::runtime_error("Operands must be two numbers, strings, or arrays.");
}

inline Value sub(const Value& left, const Value& right) {
    if (left.is_number() && right.is_number()) {
        return Value(left.as_number() - right.as_number());
    }
    throw std::runtime_error("Operands must be two numbers.");
}

inline Value mul(const Value& left, const Value& right) {
    if (left.is_number() && right.is_number()) {
        return Value(left.as_number() * right.as_number());
    }
    throw std::runtime_error("Operands must be two numbers.");
}

inline Value div(const Value& left, const Value& right) {
    if (left.is_number() && right.is_number()) {
        if (right.as_number() == 0.0) {
            throw std::runtime_error("Division by zero.");
        }
        return Value(left.as_number() / right.as_number());
    }
    throw std::runtime_error("Operands must be two numbers.");
}

inline Value mod(const Value& left, const Value& right) {
    if (left.is_number() && right.is_number()) {
        if (right.as_number() == 0.0) {
            throw std::runtime_error("Modulo by zero.");
        }
        return Value(std::fmod(left.as_number(), right.as_number()));
    }
    throw std::runtime_error("Operands must be two numbers.");
}

inline Value neg(const Value& val) {
    if (val.is_number()) {
        return Value(-val.as_number());
    }
    throw std::runtime_error("Operand must be a number.");
}

inline Value lnot(const Value& val) {
    return Value(!val.is_truthy());
}

inline Value coalesce(const Value& left, const Value& right) {
    return !left.is_nil() ? left : right;
}

// Comparison Operations
inline Value lt(const Value& left, const Value& right) {
    if (left.is_number() && right.is_number()) return Value(left.as_number() < right.as_number());
    if (left.is_string() && right.is_string()) return Value(left.as_string() < right.as_string());
    throw std::runtime_error("Operands must be two numbers or two strings.");
}

inline Value lte(const Value& left, const Value& right) {
    if (left.is_number() && right.is_number()) return Value(left.as_number() <= right.as_number());
    if (left.is_string() && right.is_string()) return Value(left.as_string() <= right.as_string());
    throw std::runtime_error("Operands must be two numbers or two strings.");
}

inline Value gt(const Value& left, const Value& right) {
    if (left.is_number() && right.is_number()) return Value(left.as_number() > right.as_number());
    if (left.is_string() && right.is_string()) return Value(left.as_string() > right.as_string());
    throw std::runtime_error("Operands must be two numbers or two strings.");
}

inline Value gte(const Value& left, const Value& right) {
    if (left.is_number() && right.is_number()) return Value(left.as_number() >= right.as_number());
    if (left.is_string() && right.is_string()) return Value(left.as_string() >= right.as_string());
    throw std::runtime_error("Operands must be two numbers or two strings.");
}

inline Value eq(const Value& left, const Value& right) {
    return Value(left == right);
}

inline Value neq(const Value& left, const Value& right) {
    return Value(left != right);
}

inline Value is_type(const Value& left, const std::string& type_name) {
    std::string lower_t = type_name;
    for (char& c : lower_t) c = std::tolower(static_cast<unsigned char>(c));
    if (lower_t == "number" || lower_t == "num") return Value(left.is_number());
    if (lower_t == "string" || lower_t == "str") return Value(left.is_string());
    if (lower_t == "bool" || lower_t == "boolean") return Value(left.is_bool());
    if (lower_t == "array" || lower_t == "list") return Value(left.is_array());
    if (lower_t == "map" || lower_t == "dict") return Value(left.is_map());
    if (lower_t == "nil" || lower_t == "null") return Value(left.is_nil());
    if (lower_t == "function" || lower_t == "fn" || lower_t == "callable") return Value(left.is_callable());
    if (left.is_map()) {
        auto m = left.as_map();
        auto it = m->find("__type__");
        if (it != m->end() && it->second.to_string() == type_name) return Value(true);
        auto it_sup = m->find("__supers__");
        if (it_sup != m->end() && it_sup->second.is_array()) {
            for (const auto& sup : *it_sup->second.as_array()) {
                if (sup.to_string() == type_name) return Value(true);
            }
        }
    }
    return Value(false);
}

// Container Construction
inline Value make_array(std::initializer_list<Value> elements) {
    return Value(std::make_shared<std::vector<Value>>(elements));
}

inline Value make_array(const std::vector<Value>& elements) {
    return Value(std::make_shared<std::vector<Value>>(elements));
}

inline Value make_map(std::initializer_list<std::pair<std::string, Value>> entries) {
    auto m = std::make_shared<std::unordered_map<std::string, Value>>();
    for (const auto& [k, v] : entries) {
        (*m)[k] = v;
    }
    return Value(m);
}

inline Value make_map(const std::vector<std::pair<std::string, Value>>& entries) {
    auto m = std::make_shared<std::unordered_map<std::string, Value>>();
    for (const auto& [k, v] : entries) {
        (*m)[k] = v;
    }
    return Value(m);
}

// Indexing and Slicing
inline Value index_get(const Value& obj, const Value& index, bool is_optional = false) {
    if (is_optional && obj.is_nil()) return Value();
    if (obj.is_array()) {
        if (!index.is_number()) throw std::runtime_error("Array index must be a number.");
        int64_t i = static_cast<int64_t>(index.as_number());
        const auto& arr = *obj.as_array();
        if (i < 0) i += arr.size();
        if (i < 0 || static_cast<size_t>(i) >= arr.size()) {
            throw std::runtime_error("Index out of bounds: " + std::to_string(i));
        }
        return arr[i];
    }
    if (obj.is_map()) {
        std::string key = index.is_string() ? index.as_string() : index.to_string();
        const auto& m = *obj.as_map();
        auto it = m.find(key);
        if (it == m.end()) return Value();
        return bind_if_method(obj, it->second);
    }
    if (obj.is_string()) {
        if (!index.is_number()) throw std::runtime_error("String index must be a number.");
        int64_t i = static_cast<int64_t>(index.as_number());
        const auto& s = obj.as_string();
        if (i < 0) i += s.size();
        if (i < 0 || static_cast<size_t>(i) >= s.size()) {
            throw std::runtime_error("Index out of bounds: " + std::to_string(i));
        }
        return Value(std::string(1, s[i]));
    }
    if (is_optional) return Value();
    throw std::runtime_error("Only arrays, maps, and strings can be indexed.");
}

inline Value index_set(Value& obj, const Value& index, const Value& val) {
    if (obj.is_array()) {
        if (!index.is_number()) throw std::runtime_error("Array index must be a number.");
        int64_t i = static_cast<int64_t>(index.as_number());
        auto& arr = *obj.as_array();
        if (i < 0) i += arr.size();
        if (i < 0 || static_cast<size_t>(i) >= arr.size()) {
            throw std::runtime_error("Index out of bounds: " + std::to_string(i));
        }
        arr[i] = val;
        return val;
    }
    if (obj.is_map()) {
        std::string key = index.is_string() ? index.as_string() : index.to_string();
        (*obj.as_map())[key] = val;
        return val;
    }
    throw std::runtime_error("Only arrays and maps support index assignment.");
}

inline Value slice(const Value& obj, const Value& start_val, const Value& end_val, bool has_start, bool has_end) {
    if (obj.is_array()) {
        const auto& arr = *obj.as_array();
        int64_t s = has_start ? static_cast<int64_t>(start_val.as_number()) : 0;
        int64_t e = has_end ? static_cast<int64_t>(end_val.as_number()) : static_cast<int64_t>(arr.size());
        if (s < 0) s += arr.size();
        if (e < 0) e += arr.size();
        s = std::max<int64_t>(0, std::min<int64_t>(s, arr.size()));
        e = std::max<int64_t>(s, std::min<int64_t>(e, arr.size()));
        auto res = std::make_shared<std::vector<Value>>();
        for (int64_t i = s; i < e; ++i) {
            res->push_back(arr[i]);
        }
        return Value(res);
    }
    if (obj.is_string()) {
        const auto& str = obj.as_string();
        int64_t s = has_start ? static_cast<int64_t>(start_val.as_number()) : 0;
        int64_t e = has_end ? static_cast<int64_t>(end_val.as_number()) : static_cast<int64_t>(str.size());
        if (s < 0) s += str.size();
        if (e < 0) e += str.size();
        s = std::max<int64_t>(0, std::min<int64_t>(s, str.size()));
        e = std::max<int64_t>(s, std::min<int64_t>(e, str.size()));
        return Value(str.substr(s, e - s));
    }
    throw std::runtime_error("Only arrays and strings can be sliced.");
}

inline Value property_get(const Value& obj, const std::string& prop, bool is_optional = false) {
    if (is_optional && obj.is_nil()) return Value();
    if (obj.is_map()) {
        const auto& m = *obj.as_map();
        auto it = m.find(prop);
        if (it == m.end()) return is_optional ? Value() : Value();
        return bind_if_method(obj, it->second);
    }
    if (is_optional) return Value();
    throw std::runtime_error("Only maps/objects have properties.");
}

inline Value property_set(Value& obj, const std::string& prop, const Value& val) {
    if (obj.is_map()) {
        (*obj.as_map())[prop] = val;
        return val;
    }
    throw std::runtime_error("Only maps/objects have properties.");
}

inline std::vector<Value> for_in_iterable(const Value& obj) {
    std::vector<Value> items;
    if (obj.is_array()) {
        const auto& arr = *obj.as_array();
        items.insert(items.end(), arr.begin(), arr.end());
    } else if (obj.is_map()) {
        for (const auto& [k, _] : *obj.as_map()) {
            items.push_back(Value(k));
        }
    } else if (obj.is_string()) {
        for (char c : obj.as_string()) {
            items.push_back(Value(std::string(1, c)));
        }
    }
    return items;
}

inline Value destructure_array_elem(const Value& obj, size_t index) {
    if (obj.is_array()) {
        const auto& arr = *obj.as_array();
        if (index < arr.size()) return arr[index];
    }
    return Value();
}

inline Value destructure_map_elem(const Value& obj, const std::string& key) {
    if (obj.is_map()) {
        const auto& m = *obj.as_map();
        auto it = m.find(key);
        if (it != m.end()) return it->second;
    }
    return Value();
}

inline void close_resource(const Value& res) {
    if (res.is_map()) {
        const auto& m = *res.as_map();
        auto it = m.find("close");
        if (it != m.end() && it->second.is_callable()) {
            Value bound = bind_if_method(res, it->second);
            if (bound.is_callable()) bound.as_callable()->call(get_interpreter(), {});
            return;
        }
        it = m.find("dispose");
        if (it != m.end() && it->second.is_callable()) {
            Value bound = bind_if_method(res, it->second);
            if (bound.is_callable()) bound.as_callable()->call(get_interpreter(), {});
        }
    }
}

// Function Calls
inline Value call(const Value& callee, const std::vector<Value>& args) {
    if (!callee.is_callable()) {
        throw std::runtime_error("Can only call functions.");
    }
    return callee.as_callable()->call(get_interpreter(), args);
}

// Built-ins direct native implementations
inline Value builtin_len(const Value& v) {
    if (v.is_string()) return Value(static_cast<double>(v.as_string().size()));
    if (v.is_array()) return Value(static_cast<double>(v.as_array()->size()));
    if (v.is_map()) return Value(static_cast<double>(v.as_map()->size()));
    throw std::runtime_error("len() expects string, array, or map.");
}

inline Value builtin_push(Value arr, const Value& val) {
    if (!arr.is_array()) throw std::runtime_error("push() expects array as first argument.");
    arr.as_array()->push_back(val);
    return val;
}

inline Value builtin_pop(Value arr) {
    if (!arr.is_array()) throw std::runtime_error("pop() expects array as argument.");
    auto& vec = *arr.as_array();
    if (vec.empty()) return Value();
    Value last = vec.back();
    vec.pop_back();
    return last;
}

inline Value builtin_keys(const Value& m) {
    if (!m.is_map()) throw std::runtime_error("keys() expects map as argument.");
    auto res = std::make_shared<std::vector<Value>>();
    for (const auto& [k, _] : *m.as_map()) {
        res->push_back(Value(k));
    }
    return Value(res);
}

inline Value builtin_values(const Value& m) {
    if (!m.is_map()) throw std::runtime_error("values() expects map as argument.");
    auto res = std::make_shared<std::vector<Value>>();
    for (const auto& [_, v] : *m.as_map()) {
        res->push_back(v);
    }
    return Value(res);
}

inline Value builtin_map_has_key(const Value& m, const Value& k) {
    if (!m.is_map()) return Value(false);
    return Value(m.as_map()->count(k.is_string() ? k.as_string() : k.to_string()) > 0);
}

inline Value builtin_type(const Value& v) {
    return Value(v.type_name());
}

inline Value builtin_str(const Value& v) {
    return Value(v.to_string());
}

inline Value builtin_num(const Value& v) {
    if (v.is_number()) return v;
    if (v.is_string()) {
        try {
            return Value(std::stod(v.as_string()));
        } catch (...) {
            return Value(0.0);
        }
    }
    return Value(0.0);
}

inline Value builtin_range(const Value& start_val, const Value& end_val) {
    if (!start_val.is_number() || !end_val.is_number()) {
        throw std::runtime_error("range() expects two numbers.");
    }
    double start = start_val.as_number();
    double end = end_val.as_number();
    auto res = std::make_shared<std::vector<Value>>();
    for (double i = start; i < end; i += 1.0) {
        res->push_back(Value(i));
    }
    return Value(res);
}

inline Value builtin_filter(const Value& arr, const Value& fn) {
    if (!arr.is_array() || !fn.is_callable()) {
        throw std::runtime_error("filter() expects array and function.");
    }
    auto res = std::make_shared<std::vector<Value>>();
    for (const auto& item : *arr.as_array()) {
        if (fn.as_callable()->call(get_interpreter(), {item}).is_truthy()) {
            res->push_back(item);
        }
    }
    return Value(res);
}

inline Value builtin_map(const Value& arr, const Value& fn) {
    if (!arr.is_array() || !fn.is_callable()) {
        throw std::runtime_error("map() expects array and function.");
    }
    auto res = std::make_shared<std::vector<Value>>();
    for (const auto& item : *arr.as_array()) {
        res->push_back(fn.as_callable()->call(get_interpreter(), {item}));
    }
    return Value(res);
}

inline Value builtin_reduce(const Value& arr, const Value& fn, const Value& initial) {
    if (!arr.is_array() || !fn.is_callable()) {
        throw std::runtime_error("reduce() expects array, function, and initial value.");
    }
    Value acc = initial;
    for (const auto& item : *arr.as_array()) {
        acc = fn.as_callable()->call(get_interpreter(), {acc, item});
    }
    return acc;
}

inline Value builtin_time() {
    auto now = std::chrono::system_clock::now();
    double s = std::chrono::duration<double>(now.time_since_epoch()).count();
    return Value(s);
}

inline Value builtin_clock() {
    auto now = std::chrono::steady_clock::now();
    double s = std::chrono::duration<double>(now.time_since_epoch()).count();
    return Value(s);
}

inline Value builtin_sleep(const Value& ms_val) {
    if (!ms_val.is_number()) throw std::runtime_error("sleep() expects milliseconds as number.");
    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int64_t>(ms_val.as_number())));
    return Value();
}

inline Value builtin_env(const std::vector<Value>& args) {
    if (args.empty()) return Value();
    std::string name = args[0].to_string();
    std::string def = args.size() > 1 ? args[1].to_string() : "";
    auto opt = platform::get_env_var(name);
    return opt ? Value(*opt) : (def.empty() ? Value() : Value(def));
}

inline Value builtin_get_env(const Value& name) {
    auto opt = platform::get_env_var(name.to_string());
    return opt ? Value(*opt) : Value("");
}

inline Value builtin_set_env(const Value& name, const Value& val) {
    platform::set_env_var(name.to_string(), val.to_string());
    return val;
}

inline Value call_global_or_builtin(const std::string& name, const std::vector<Value>& args) {
    auto& interp = get_interpreter();
    try {
        Value fn_val = interp.globals()->get(Token{TokenType::Identifier, name, 0, 0});
        if (fn_val.is_callable()) {
            return fn_val.as_callable()->call(interp, args);
        }
    } catch (...) {}
    throw std::runtime_error("Undefined function or builtin '" + name + "'.");
}

} // namespace raven::native::rt
