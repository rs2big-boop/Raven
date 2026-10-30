#include "raven/interpreter.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <random>
#include <regex>
#include <set>
#include <sstream>
#include <thread>

#include "raven/gc.hpp"

#include "raven/lexer.hpp"
#include "raven/parser.hpp"

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

struct NativeChannel {
    std::mutex mtx;
    std::condition_variable cv_recv;
    std::condition_variable cv_send;
    std::deque<Value> queue;
    size_t capacity = 0;
    bool closed = false;
};

static std::mutex g_channels_mutex;
static std::unordered_map<int64_t, std::shared_ptr<NativeChannel>> g_channels;
static int64_t g_next_channel_id = 1;

// Spawn tracking — for graceful runtime shutdown
struct SpawnRegistry {
    std::mutex mtx;
    std::vector<std::thread> threads;

    void add(std::thread t) {
        std::lock_guard<std::mutex> lk(mtx);
        threads.push_back(std::move(t));
    }

    // Join all tracked threads (called at shutdown).
    // Threads that do not finish within `timeout_ms` are detached.
    void join_all(int64_t timeout_ms = 2000) {
        std::lock_guard<std::mutex> lk(mtx);
        for (auto& t : threads) {
            if (t.joinable()) {
                // Use a detach approach: try join with a timed wait via a helper
                // future. If joinable threads are still running after timeout,
                // detach them to avoid blocking the main thread indefinitely.
                // For now we do a straightforward join (threads are expected to
                // be short-lived in typical Raven programs).
                (void)timeout_ms;
                t.join();
            }
        }
        threads.clear();
    }

    ~SpawnRegistry() {
        // Best-effort cleanup on static destruction — detach remaining threads.
        std::lock_guard<std::mutex> lk(mtx);
        for (auto& t : threads) {
            if (t.joinable()) t.detach();
        }
    }
};

static SpawnRegistry g_spawn_registry;

struct NativeWaitGroup {
    std::mutex mtx;
    std::condition_variable cv;
    int64_t counter = 0;
};

static std::mutex g_wait_groups_mutex;
static std::unordered_map<int64_t, std::shared_ptr<NativeWaitGroup>> g_wait_groups;
static int64_t g_next_wg_id = 1;

struct NativeMutex {
    std::recursive_mutex mtx;
    std::atomic<bool> locked{false};
};

static std::mutex g_native_mutexes_mutex;
static std::unordered_map<int64_t, std::shared_ptr<NativeMutex>> g_native_mutexes;
static int64_t g_next_mutex_id = 1;

struct CallStackGuard {
    Interpreter& interp;
    CallStackGuard(Interpreter& i, const std::string& name, size_t line, size_t col) : interp(i) {
        interp.push_frame(name, line, col);
    }
    ~CallStackGuard() {
        interp.pop_frame();
    }
};

class UserFunction : public Callable {
public:
    UserFunction(Token name, std::vector<Token> params, std::vector<std::shared_ptr<Stmt>> body,
                 std::shared_ptr<Environment> closure,
                 std::vector<std::shared_ptr<Expr>> param_defaults = {})
        : name_(std::move(name)), params_(std::move(params)), body_(std::move(body)),
          closure_(std::move(closure)), param_defaults_(std::move(param_defaults)) {}

    int arity() const override {
        return static_cast<int>(params_.size());
    }

    int min_arity() const override {
        int required = 0;
        for (size_t i = 0; i < params_.size(); ++i) {
            if (i >= param_defaults_.size() || param_defaults_[i] == nullptr) {
                required++;
            }
        }
        return required;
    }

    Value call(Interpreter& interpreter, const std::vector<Value>& arguments) override {
        CallStackGuard guard(interpreter, name_.lexeme, name_.line, name_.column);
        auto env = std::make_shared<Environment>(closure_);
        for (std::size_t i = 0; i < params_.size(); ++i) {
            if (i < arguments.size()) {
                env->define(params_[i].lexeme, arguments[i]);
            } else if (i < param_defaults_.size() && param_defaults_[i] != nullptr) {
                Value def_val = interpreter.evaluate(*param_defaults_[i]);
                env->define(params_[i].lexeme, def_val);
            } else {
                env->define(params_[i].lexeme, Value());
            }
        }

        try {
            interpreter.execute_block(body_, env);
        } catch (const ReturnException& ret) {
            return ret.value;
        } catch (RuntimeError& err) {
            if (err.stack_trace().empty()) {
                err.set_stack_trace(interpreter.call_stack());
            }
            throw;
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
    std::vector<std::shared_ptr<Expr>> param_defaults_;
};

static std::string value_to_json(const Value& val) {
    if (val.is_nil()) return "null";
    if (val.is_bool()) return val.as_bool() ? "true" : "false";
    if (val.is_number()) {
        std::ostringstream ss;
        double n = val.as_number();
        if (std::floor(n) == n) {
            ss << static_cast<int64_t>(n);
        } else {
            ss << n;
        }
        return ss.str();
    }
    if (val.is_string()) {
        std::string res = "\"";
        for (char c : val.as_string()) {
            if (c == '"') res += "\\\"";
            else if (c == '\\') res += "\\\\";
            else if (c == '\n') res += "\\n";
            else if (c == '\t') res += "\\t";
            else if (c == '\r') res += "\\r";
            else res += c;
        }
        res += "\"";
        return res;
    }
    if (val.is_array()) {
        std::string res = "[";
        const auto& arr = *val.as_array();
        for (size_t i = 0; i < arr.size(); ++i) {
            res += value_to_json(arr[i]);
            if (i + 1 < arr.size()) res += ", ";
        }
        res += "]";
        return res;
    }
    if (val.is_map()) {
        std::string res = "{";
        const auto& m = *val.as_map();
        size_t count = 0;
        for (const auto& [k, v] : m) {
            res += "\"" + k + "\": " + value_to_json(v);
            if (++count < m.size()) res += ", ";
        }
        res += "}";
        return res;
    }
    return "\"<callable>\"";
}

class SimpleJsonParser {
public:
    explicit SimpleJsonParser(std::string src) : src_(std::move(src)), pos_(0) {}

    Value parse() {
        skip_ws();
        if (pos_ >= src_.size()) return Value();
        return parse_value();
    }

private:
    std::string src_;
    size_t pos_;

    void skip_ws() {
        while (pos_ < src_.size() && (src_[pos_] == ' ' || src_[pos_] == '\t' || src_[pos_] == '\n' || src_[pos_] == '\r')) {
            pos_++;
        }
    }

    Value parse_value() {
        skip_ws();
        if (pos_ >= src_.size()) return Value();
        char c = src_[pos_];
        if (c == '{') return parse_object();
        if (c == '[') return parse_array();
        if (c == '"') return parse_string();
        if (c == 't' || c == 'f') return parse_bool();
        if (c == 'n') return parse_null();
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parse_number();
        return Value();
    }

    Value parse_object() {
        pos_++; // '{'
        skip_ws();
        auto map = std::make_shared<std::unordered_map<std::string, Value>>();
        if (pos_ < src_.size() && src_[pos_] == '}') {
            pos_++;
            return Value(map);
        }
        while (pos_ < src_.size()) {
            skip_ws();
            if (pos_ >= src_.size() || src_[pos_] != '"') break;
            Value key = parse_string();
            skip_ws();
            if (pos_ < src_.size() && src_[pos_] == ':') pos_++;
            skip_ws();
            Value val = parse_value();
            (*map)[key.as_string()] = val;
            skip_ws();
            if (pos_ < src_.size() && src_[pos_] == ',') {
                pos_++;
            } else if (pos_ < src_.size() && src_[pos_] == '}') {
                pos_++;
                break;
            } else {
                break;
            }
        }
        return Value(map);
    }

    Value parse_array() {
        pos_++; // '['
        skip_ws();
        auto arr = std::make_shared<std::vector<Value>>();
        if (pos_ < src_.size() && src_[pos_] == ']') {
            pos_++;
            return Value(arr);
        }
        while (pos_ < src_.size()) {
            skip_ws();
            arr->push_back(parse_value());
            skip_ws();
            if (pos_ < src_.size() && src_[pos_] == ',') {
                pos_++;
            } else if (pos_ < src_.size() && src_[pos_] == ']') {
                pos_++;
                break;
            } else {
                break;
            }
        }
        return Value(arr);
    }

    Value parse_string() {
        pos_++; // '"'
        std::string s;
        while (pos_ < src_.size() && src_[pos_] != '"') {
            if (src_[pos_] == '\\' && pos_ + 1 < src_.size()) {
                pos_++;
                char esc = src_[pos_];
                if (esc == 'n') s += '\n';
                else if (esc == 't') s += '\t';
                else if (esc == 'r') s += '\r';
                else if (esc == '"') s += '"';
                else if (esc == '\\') s += '\\';
                else s += esc;
            } else {
                s += src_[pos_];
            }
            pos_++;
        }
        if (pos_ < src_.size() && src_[pos_] == '"') pos_++;
        return Value(s);
    }

    Value parse_number() {
        size_t start = pos_;
        if (src_[pos_] == '-') pos_++;
        while (pos_ < src_.size() && (std::isdigit(static_cast<unsigned char>(src_[pos_])) || src_[pos_] == '.' || src_[pos_] == 'e' || src_[pos_] == 'E' || src_[pos_] == '+' || src_[pos_] == '-')) {
            pos_++;
        }
        std::string num_str = src_.substr(start, pos_ - start);
        try {
            return Value(std::stod(num_str));
        } catch (...) {
            return Value(0.0);
        }
    }

    Value parse_bool() {
        if (src_.substr(pos_, 4) == "true") {
            pos_ += 4;
            return Value(true);
        }
        if (src_.substr(pos_, 5) == "false") {
            pos_ += 5;
            return Value(false);
        }
        return Value(false);
    }

    Value parse_null() {
        if (src_.substr(pos_, 4) == "null") {
            pos_ += 4;
        }
        return Value();
    }
};

Interpreter::Interpreter(std::ostream& out)
    : out_(out),
      globals_(std::make_shared<Environment>()),
      environment_(globals_) {
    register_builtins();
}

Interpreter::Interpreter(std::shared_ptr<Environment> globals, std::ostream& out)
    : out_(out),
      globals_(std::move(globals)),
      environment_(globals_) {
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

    // Math Native Functions
    def("abs", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_number()) throw RuntimeError(Token{TokenType::Identifier, "abs", 0, 0}, "abs() expects a number.");
        return Value(std::abs(args[0].as_number()));
    });
    def("min", 2, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_number() || !args[1].is_number()) {
            throw RuntimeError(Token{TokenType::Identifier, "min", 0, 0}, "min() expects two numbers.");
        }
        return Value(std::min(args[0].as_number(), args[1].as_number()));
    });
    def("max", 2, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_number() || !args[1].is_number()) {
            throw RuntimeError(Token{TokenType::Identifier, "max", 0, 0}, "max() expects two numbers.");
        }
        return Value(std::max(args[0].as_number(), args[1].as_number()));
    });
    def("sqrt", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_number()) throw RuntimeError(Token{TokenType::Identifier, "sqrt", 0, 0}, "sqrt() expects number.");
        return Value(std::sqrt(args[0].as_number()));
    });
    def("sin", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_number()) throw RuntimeError(Token{TokenType::Identifier, "sin", 0, 0}, "sin() expects number.");
        return Value(std::sin(args[0].as_number()));
    });
    def("cos", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_number()) throw RuntimeError(Token{TokenType::Identifier, "cos", 0, 0}, "cos() expects number.");
        return Value(std::cos(args[0].as_number()));
    });
    def("tan", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_number()) throw RuntimeError(Token{TokenType::Identifier, "tan", 0, 0}, "tan() expects number.");
        return Value(std::tan(args[0].as_number()));
    });
    def("floor", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_number()) throw RuntimeError(Token{TokenType::Identifier, "floor", 0, 0}, "floor() expects number.");
        return Value(std::floor(args[0].as_number()));
    });
    def("ceil", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_number()) throw RuntimeError(Token{TokenType::Identifier, "ceil", 0, 0}, "ceil() expects number.");
        return Value(std::ceil(args[0].as_number()));
    });
    def("round", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_number()) throw RuntimeError(Token{TokenType::Identifier, "round", 0, 0}, "round() expects number.");
        return Value(std::round(args[0].as_number()));
    });
    def("log", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_number()) throw RuntimeError(Token{TokenType::Identifier, "log", 0, 0}, "log() expects number.");
        return Value(std::log(args[0].as_number()));
    });
    def("exp", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_number()) throw RuntimeError(Token{TokenType::Identifier, "exp", 0, 0}, "exp() expects number.");
        return Value(std::exp(args[0].as_number()));
    });
    def("rand", 0, [](Interpreter&, const std::vector<Value>&) -> Value {
        static std::mt19937_64 rng(std::random_device{}());
        static std::uniform_real_distribution<double> dist(0.0, 1.0);
        return Value(dist(rng));
    });
    def("rand_int", 2, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_number() || !args[1].is_number()) {
            throw RuntimeError(Token{TokenType::Identifier, "rand_int", 0, 0}, "rand_int() expects two numbers.");
        }
        static std::mt19937_64 rng(std::random_device{}());
        int64_t low = static_cast<int64_t>(args[0].as_number());
        int64_t high = static_cast<int64_t>(args[1].as_number());
        if (low > high) std::swap(low, high);
        std::uniform_int_distribution<int64_t> dist(low, high);
        return Value(static_cast<double>(dist(rng)));
    });

    // String & Array Native Functions
    def("split", 2, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_string() || !args[1].is_string()) {
            throw RuntimeError(Token{TokenType::Identifier, "split", 0, 0}, "split() expects two strings.");
        }
        const std::string& s = args[0].as_string();
        const std::string& delim = args[1].as_string();
        auto res = std::make_shared<std::vector<Value>>();
        if (delim.empty()) {
            for (char c : s) res->push_back(Value(std::string(1, c)));
            return Value(res);
        }
        size_t start = 0;
        size_t end = s.find(delim);
        while (end != std::string::npos) {
            res->push_back(Value(s.substr(start, end - start)));
            start = end + delim.length();
            end = s.find(delim, start);
        }
        res->push_back(Value(s.substr(start)));
        return Value(res);
    });

    def("join", 2, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_array() || !args[1].is_string()) {
            throw RuntimeError(Token{TokenType::Identifier, "join", 0, 0}, "join() expects array and delimiter string.");
        }
        const auto& arr = *args[0].as_array();
        const std::string& delim = args[1].as_string();
        std::string res;
        for (size_t i = 0; i < arr.size(); ++i) {
            res += arr[i].to_string();
            if (i + 1 < arr.size()) res += delim;
        }
        return Value(res);
    });

    def("contains", 2, [](Interpreter&, const std::vector<Value>& args) -> Value {
        const auto& h = args[0];
        const auto& n = args[1];
        if (h.is_string() && n.is_string()) {
            return Value(h.as_string().find(n.as_string()) != std::string::npos);
        }
        if (h.is_array()) {
            for (const auto& item : *h.as_array()) {
                if (item == n) return Value(true);
            }
            return Value(false);
        }
        if (h.is_map() && n.is_string()) {
            return Value(h.as_map()->find(n.as_string()) != h.as_map()->end());
        }
        return Value(false);
    });

    def("to_upper", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_string()) throw RuntimeError(Token{TokenType::Identifier, "to_upper", 0, 0}, "to_upper() expects a string.");
        std::string s = args[0].as_string();
        for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return Value(s);
    });

    def("to_lower", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_string()) throw RuntimeError(Token{TokenType::Identifier, "to_lower", 0, 0}, "to_lower() expects a string.");
        std::string s = args[0].as_string();
        for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return Value(s);
    });

    def("trim", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_string()) throw RuntimeError(Token{TokenType::Identifier, "trim", 0, 0}, "trim() expects a string.");
        const std::string& s = args[0].as_string();
        auto start = s.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return Value("");
        auto end = s.find_last_not_of(" \t\n\r");
        return Value(s.substr(start, end - start + 1));
    });

    def("replace", 3, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_string() || !args[1].is_string() || !args[2].is_string()) {
            throw RuntimeError(Token{TokenType::Identifier, "replace", 0, 0}, "replace() expects three strings.");
        }
        std::string s = args[0].as_string();
        const std::string& from = args[1].as_string();
        const std::string& to = args[2].as_string();
        if (from.empty()) return Value(s);
        size_t pos = 0;
        while ((pos = s.find(from, pos)) != std::string::npos) {
            s.replace(pos, from.length(), to);
            pos += to.length();
        }
        return Value(s);
    });

    def("index_of", 2, [](Interpreter&, const std::vector<Value>& args) -> Value {
        const auto& h = args[0];
        const auto& n = args[1];
        if (h.is_string() && n.is_string()) {
            auto pos = h.as_string().find(n.as_string());
            return Value(pos == std::string::npos ? -1.0 : static_cast<double>(pos));
        }
        if (h.is_array()) {
            const auto& arr = *h.as_array();
            for (size_t i = 0; i < arr.size(); ++i) {
                if (arr[i] == n) return Value(static_cast<double>(i));
            }
            return Value(-1.0);
        }
        return Value(-1.0);
    });

    def("char_code", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_string()) throw RuntimeError(Token{TokenType::Identifier, "char_code", 0, 0}, "char_code() expects a string.");
        const std::string& s = args[0].as_string();
        if (s.empty()) return Value(0.0);
        return Value(static_cast<double>(static_cast<unsigned char>(s[0])));
    });

    def("char", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_number()) throw RuntimeError(Token{TokenType::Identifier, "char", 0, 0}, "char() expects a number.");
        int code = static_cast<int>(args[0].as_number());
        return Value(std::string(1, static_cast<char>(code)));
    });

    def("sort_arr", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_array()) throw RuntimeError(Token{TokenType::Identifier, "sort_arr", 0, 0}, "sort_arr() expects an array.");
        auto arr = args[0].as_array();
        std::sort(arr->begin(), arr->end(), [](const Value& a, const Value& b) {
            if (a.is_number() && b.is_number()) return a.as_number() < b.as_number();
            return a.to_string() < b.to_string();
        });
        return Value(arr);
    });

    // File I/O
    def("read_file", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_string()) throw RuntimeError(Token{TokenType::Identifier, "read_file", 0, 0}, "read_file() expects path string.");
        std::ifstream f(args[0].as_string());
        if (!f.is_open()) return Value();
        std::stringstream buf;
        buf << f.rdbuf();
        return Value(buf.str());
    });
    def("write_file", 2, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_string() || !args[1].is_string()) {
            throw RuntimeError(Token{TokenType::Identifier, "write_file", 0, 0}, "write_file() expects path and content strings.");
        }
        std::ofstream f(args[0].as_string());
        if (!f.is_open()) return Value(false);
        f << args[1].as_string();
        return Value(true);
    });
    def("file_exists", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_string()) return Value(false);
        return Value(std::filesystem::exists(args[0].as_string()));
    });

    // System & Execution
    def("sleep", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_number()) throw RuntimeError(Token{TokenType::Identifier, "sleep", 0, 0}, "sleep() expects milliseconds number.");
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int64_t>(args[0].as_number())));
        return Value();
    });
    def("env", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_string()) return Value();
        const char* val = std::getenv(args[0].as_string().c_str());
        if (!val) return Value();
        return Value(std::string(val));
    });
    def("exec", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_string()) throw RuntimeError(Token{TokenType::Identifier, "exec", 0, 0}, "exec() expects shell command string.");
        std::string cmd = args[0].as_string();
        std::array<char, 256> buffer;
        std::string result;
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return Value();
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result += buffer.data();
        }
        pclose(pipe);
        return Value(result);
    });

    def("input", 0, [](Interpreter&, const std::vector<Value>&) -> Value {
        std::string line;
        if (std::getline(std::cin, line)) {
            return Value(line);
        }
        return Value("");
    });

    def("print_err", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        std::cerr << args[0].to_string() << "\n";
        return Value();
    });

    // JSON Serialization
    def("to_json", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        return Value(value_to_json(args[0]));
    });
    def("parse_json", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_string()) throw RuntimeError(Token{TokenType::Identifier, "parse_json", 0, 0}, "parse_json() expects JSON string.");
        SimpleJsonParser parser(args[0].as_string());
        return parser.parse();
    });

    // Sequence Slicing
    def("slice", 3, [](Interpreter&, const std::vector<Value>& args) -> Value {
        int64_t start = static_cast<int64_t>(args[1].as_number());
        int64_t end = static_cast<int64_t>(args[2].as_number());
        if (args[0].is_string()) {
            const std::string& s = args[0].as_string();
            int64_t len = static_cast<int64_t>(s.size());
            if (start < 0) start += len;
            if (end < 0) end += len;
            start = std::max<int64_t>(0, std::min<int64_t>(start, len));
            end = std::max<int64_t>(0, std::min<int64_t>(end, len));
            if (start >= end) return Value("");
            return Value(s.substr(start, end - start));
        }
        if (args[0].is_array()) {
            const auto& arr = *args[0].as_array();
            int64_t len = static_cast<int64_t>(arr.size());
            if (start < 0) start += len;
            if (end < 0) end += len;
            start = std::max<int64_t>(0, std::min<int64_t>(start, len));
            end = std::max<int64_t>(0, std::min<int64_t>(end, len));
            auto res = std::make_shared<std::vector<Value>>();
            for (int64_t i = start; i < end; ++i) {
                res->push_back(arr[i]);
            }
            return Value(res);
        }
        return Value();
    });

    // Regular Expressions
    def("regex_match", 2, [](Interpreter&, const std::vector<Value>& args) -> Value {
        try {
            std::regex re(args[1].as_string());
            return Value(std::regex_search(args[0].as_string(), re));
        } catch (...) {
            return Value(false);
        }
    });

    def("regex_replace", 3, [](Interpreter&, const std::vector<Value>& args) -> Value {
        try {
            std::regex re(args[1].as_string());
            return Value(std::regex_replace(args[0].as_string(), re, args[2].as_string()));
        } catch (...) {
            return args[0];
        }
    });

    // Encoding & Hashing
    def("hash", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        const std::string& s = args[0].as_string();
        uint64_t h = 14695981039346656037ULL;
        for (unsigned char c : s) {
            h ^= c;
            h *= 1099511628211ULL;
        }
        std::stringstream ss;
        ss << std::hex << h;
        return Value(ss.str());
    });

    def("base64_encode", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        static const std::string b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        const std::string& in = args[0].as_string();
        std::string out;
        int val = 0, valb = -6;
        for (unsigned char c : in) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                out.push_back(b64[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6) out.push_back(b64[((val << 8) >> (valb + 8)) & 0x3F]);
        while (out.size() % 4) out.push_back('=');
        return Value(out);
    });

    def("base64_decode", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        static const std::string b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        const std::string& in = args[0].as_string();
        std::string out;
        std::vector<int> T(256, -1);
        for (int i = 0; i < 64; i++) T[static_cast<unsigned char>(b64[i])] = i;
        int val = 0, valb = -8;
        for (unsigned char c : in) {
            if (T[c] == -1) break;
            val = (val << 6) + T[c];
            valb += 6;
            if (valb >= 0) {
                out.push_back(static_cast<char>((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
        return Value(out);
    });

    // Networking HTTP
    def("http_get", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        std::string cmd = "curl -s -L \"" + args[0].as_string() + "\"";
        std::array<char, 256> buffer;
        std::string result;
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return Value();
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result += buffer.data();
        }
        pclose(pipe);
        return Value(result);
    });

    // String manipulation additions
    def("str_repeat", 2, [](Interpreter&, const std::vector<Value>& args) -> Value {
        std::string s = args[0].to_string();
        int n = static_cast<int>(args[1].as_number());
        std::string res;
        for (int i = 0; i < n; i++) res += s;
        return Value(res);
    });

    def("pad_left", 3, [](Interpreter&, const std::vector<Value>& args) -> Value {
        std::string s = args[0].to_string();
        int width = static_cast<int>(args[1].as_number());
        std::string ch = args[2].to_string();
        if (ch.empty()) ch = " ";
        while (static_cast<int>(s.size()) < width) s = ch[0] + s;
        return Value(s);
    });

    def("pad_right", 3, [](Interpreter&, const std::vector<Value>& args) -> Value {
        std::string s = args[0].to_string();
        int width = static_cast<int>(args[1].as_number());
        std::string ch = args[2].to_string();
        if (ch.empty()) ch = " ";
        while (static_cast<int>(s.size()) < width) s += ch[0];
        return Value(s);
    });

    def("str_reverse", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        std::string s = args[0].to_string();
        std::reverse(s.begin(), s.end());
        return Value(s);
    });

    def("is_digit", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        std::string s = args[0].to_string();
        if (s.empty()) return Value(false);
        return Value(static_cast<bool>(std::isdigit(static_cast<unsigned char>(s[0]))));
    });

    def("is_alpha", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        std::string s = args[0].to_string();
        if (s.empty()) return Value(false);
        return Value(static_cast<bool>(std::isalpha(static_cast<unsigned char>(s[0]))));
    });

    def("is_space", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        std::string s = args[0].to_string();
        if (s.empty()) return Value(false);
        return Value(static_cast<bool>(std::isspace(static_cast<unsigned char>(s[0]))));
    });

    def("is_upper", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        std::string s = args[0].to_string();
        if (s.empty()) return Value(false);
        return Value(static_cast<bool>(std::isupper(static_cast<unsigned char>(s[0]))));
    });

    def("is_lower", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        std::string s = args[0].to_string();
        if (s.empty()) return Value(false);
        return Value(static_cast<bool>(std::islower(static_cast<unsigned char>(s[0]))));
    });

    def("str_count", 2, [](Interpreter&, const std::vector<Value>& args) -> Value {
        std::string s = args[0].to_string();
        std::string sub = args[1].to_string();
        if (sub.empty()) return Value(0.0);
        int count = 0;
        size_t pos = 0;
        while ((pos = s.find(sub, pos)) != std::string::npos) {
            count++;
            pos += sub.size();
        }
        return Value(static_cast<double>(count));
    });

    def("has_prefix", 2, [](Interpreter&, const std::vector<Value>& args) -> Value {
        std::string s = args[0].to_string();
        std::string prefix = args[1].to_string();
        return Value(s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix);
    });

    def("has_suffix", 2, [](Interpreter&, const std::vector<Value>& args) -> Value {
        std::string s = args[0].to_string();
        std::string suffix = args[1].to_string();
        return Value(s.size() >= suffix.size() && s.substr(s.size() - suffix.size()) == suffix);
    });

    def("format", -1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (args.empty()) return Value(std::string(""));
        std::string tmpl = args[0].to_string();
        std::string result;
        size_t arg_idx = 1;
        size_t i = 0;
        while (i < tmpl.size()) {
            if (tmpl[i] == '{' && i + 1 < tmpl.size() && tmpl[i+1] == '}') {
                if (arg_idx < args.size()) {
                    result += args[arg_idx++].to_string();
                }
                i += 2;
            } else {
                result += tmpl[i++];
            }
        }
        return Value(result);
    });

    // Array and Collection additions
    def("arr_contains", 2, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_array()) return Value(false);
        auto arr = args[0].as_array();
        for (const auto& v : *arr) {
            if (v.to_string() == args[1].to_string()) return Value(true);
        }
        return Value(false);
    });

    def("unique", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_array()) return args[0];
        auto arr = args[0].as_array();
        auto result = std::make_shared<std::vector<Value>>();
        std::set<std::string> seen;
        for (const auto& v : *arr) {
            std::string k = v.to_string();
            if (seen.insert(k).second) {
                result->push_back(v);
            }
        }
        return Value(result);
    });

    def("flatten", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_array()) return args[0];
        auto arr = args[0].as_array();
        auto result = std::make_shared<std::vector<Value>>();
        for (const auto& v : *arr) {
            if (v.is_array()) {
                for (const auto& inner : *v.as_array()) {
                    result->push_back(inner);
                }
            } else {
                result->push_back(v);
            }
        }
        return Value(result);
    });

    def("arr_reverse", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_array()) return args[0];
        auto arr = args[0].as_array();
        auto result = std::make_shared<std::vector<Value>>(arr->rbegin(), arr->rend());
        return Value(result);
    });

    def("sum", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_array()) return Value(0.0);
        double total = 0.0;
        for (const auto& v : *args[0].as_array()) {
            if (v.is_number()) total += v.as_number();
        }
        return Value(total);
    });

    def("arr_min", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_array() || args[0].as_array()->empty()) return Value();
        auto arr = args[0].as_array();
        double m = arr->at(0).as_number();
        for (size_t i = 1; i < arr->size(); i++) {
            double v = arr->at(i).as_number();
            if (v < m) m = v;
        }
        return Value(m);
    });

    def("arr_max", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_array() || args[0].as_array()->empty()) return Value();
        auto arr = args[0].as_array();
        double m = arr->at(0).as_number();
        for (size_t i = 1; i < arr->size(); i++) {
            double v = arr->at(i).as_number();
            if (v > m) m = v;
        }
        return Value(m);
    });

    def("count_if", 2, [](Interpreter& interp, const std::vector<Value>& args) -> Value {
        if (!args[0].is_array() || !args[1].is_callable()) return Value(0.0);
        auto arr = args[0].as_array();
        auto fn = args[1].as_callable();
        int count = 0;
        for (const auto& v : *arr) {
            Value r = fn->call(interp, {v});
            if (r.is_truthy()) count++;
        }
        return Value(static_cast<double>(count));
    });

    def("map_merge", 2, [](Interpreter&, const std::vector<Value>& args) -> Value {
        auto result = std::make_shared<std::unordered_map<std::string, Value>>();
        if (args[0].is_map()) {
            for (const auto& [k, v] : *args[0].as_map()) (*result)[k] = v;
        }
        if (args[1].is_map()) {
            for (const auto& [k, v] : *args[1].as_map()) (*result)[k] = v;
        }
        return Value(result);
    });

    def("map_has_key", 2, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_map()) return Value(false);
        std::string key = args[1].to_string();
        return Value(args[0].as_map()->count(key) > 0);
    });

    def("map_delete", 2, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_map()) return args[0];
        std::string key = args[1].to_string();
        args[0].as_map()->erase(key);
        return args[0];
    });

    // Math additions
    def("int", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        return Value(static_cast<double>(static_cast<int64_t>(args[0].as_number())));
    });

    def("bool", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        return Value(args[0].is_truthy());
    });

    def("clamp", 3, [](Interpreter&, const std::vector<Value>& args) -> Value {
        double v = args[0].as_number();
        double lo = args[1].as_number();
        double hi = args[2].as_number();
        return Value(std::max(lo, std::min(hi, v)));
    });

    def("gcd", 2, [](Interpreter&, const std::vector<Value>& args) -> Value {
        int64_t a = static_cast<int64_t>(std::abs(args[0].as_number()));
        int64_t b = static_cast<int64_t>(std::abs(args[1].as_number()));
        while (b != 0) {
            int64_t t = b;
            b = a % b;
            a = t;
        }
        return Value(static_cast<double>(a));
    });

    def("lcm", 2, [](Interpreter&, const std::vector<Value>& args) -> Value {
        int64_t a = static_cast<int64_t>(std::abs(args[0].as_number()));
        int64_t b = static_cast<int64_t>(std::abs(args[1].as_number()));
        if (a == 0 || b == 0) return Value(0.0);
        int64_t orig_a = a, orig_b = b;
        while (b != 0) {
            int64_t t = b;
            b = a % b;
            a = t;
        }
        return Value(static_cast<double>((orig_a / a) * orig_b));
    });

    // IO and File System additions
    def("read_lines", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        std::string path = args[0].to_string();
        std::ifstream file(path);
        auto lines = std::make_shared<std::vector<Value>>();
        if (!file.is_open()) return Value(lines);
        std::string line;
        while (std::getline(file, line)) {
            lines->push_back(Value(line));
        }
        return Value(lines);
    });

    def("append_file", 2, [](Interpreter&, const std::vector<Value>& args) -> Value {
        std::string path = args[0].to_string();
        std::string content = args[1].to_string();
        std::ofstream file(path, std::ios::app);
        if (!file.is_open()) return Value(false);
        file << content;
        return Value(true);
    });

    def("is_dir", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        std::string path = args[0].to_string();
        std::error_code ec;
        return Value(std::filesystem::is_directory(path, ec));
    });

    def("list_dir", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        std::string path = args[0].to_string();
        auto entries = std::make_shared<std::vector<Value>>();
        std::error_code ec;
        if (std::filesystem::is_directory(path, ec)) {
            for (const auto& entry : std::filesystem::directory_iterator(path, ec)) {
                entries->push_back(Value(entry.path().filename().string()));
            }
        }
        return Value(entries);
    });

    def("mkdir", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        std::string path = args[0].to_string();
        std::error_code ec;
        return Value(std::filesystem::create_directories(path, ec));
    });

    def("remove_file", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        std::string path = args[0].to_string();
        std::error_code ec;
        return Value(std::filesystem::remove(path, ec));
    });

    def("get_cwd", 0, [](Interpreter&, const std::vector<Value>&) -> Value {
        std::error_code ec;
        return Value(std::filesystem::current_path(ec).string());
    });

    def("panic", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        throw RuntimeError(Token{TokenType::Identifier, "panic", 0, 0}, "panic: " + args[0].to_string());
    });

    // Native Concurrency: CSP Channels & Thread Spawning
    def("chan_create", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        auto ch = std::make_shared<NativeChannel>();
        ch->capacity = static_cast<size_t>(std::max(0.0, args[0].as_number()));
        std::lock_guard<std::mutex> lock(g_channels_mutex);
        int64_t id = g_next_channel_id++;
        g_channels[id] = ch;
        return Value(static_cast<double>(id));
    });

    def("chan_send", 2, [](Interpreter&, const std::vector<Value>& args) -> Value {
        int64_t id = static_cast<int64_t>(args[0].as_number());
        std::shared_ptr<NativeChannel> ch;
        {
            std::lock_guard<std::mutex> lock(g_channels_mutex);
            auto it = g_channels.find(id);
            if (it == g_channels.end()) {
                throw RuntimeError(Token{TokenType::Identifier, "chan_send", 0, 0}, "Invalid channel ID.");
            }
            ch = it->second;
        }
        std::unique_lock<std::mutex> lk(ch->mtx);
        if (ch->closed) {
            throw RuntimeError(Token{TokenType::Identifier, "chan_send", 0, 0}, "Cannot send on closed channel.");
        }
        if (ch->capacity > 0) {
            ch->cv_send.wait(lk, [&]() { return ch->closed || ch->queue.size() < ch->capacity; });
            if (ch->closed) {
                throw RuntimeError(Token{TokenType::Identifier, "chan_send", 0, 0}, "Cannot send on closed channel.");
            }
        }
        ch->queue.push_back(args[1]);
        ch->cv_recv.notify_one();
        return Value(true);
    });

    def("chan_recv", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        int64_t id = static_cast<int64_t>(args[0].as_number());
        std::shared_ptr<NativeChannel> ch;
        {
            std::lock_guard<std::mutex> lock(g_channels_mutex);
            auto it = g_channels.find(id);
            if (it == g_channels.end()) {
                throw RuntimeError(Token{TokenType::Identifier, "chan_recv", 0, 0}, "Invalid channel ID.");
            }
            ch = it->second;
        }
        std::unique_lock<std::mutex> lk(ch->mtx);
        ch->cv_recv.wait(lk, [&]() { return !ch->queue.empty() || ch->closed; });
        if (ch->queue.empty() && ch->closed) {
            return Value();
        }
        Value val = ch->queue.front();
        ch->queue.pop_front();
        if (ch->capacity > 0) {
            ch->cv_send.notify_one();
        }
        return val;
    });

    def("chan_recv_timeout", 2, [](Interpreter&, const std::vector<Value>& args) -> Value {
        int64_t id = static_cast<int64_t>(args[0].as_number());
        int64_t timeout_ms = static_cast<int64_t>(args[1].as_number());
        std::shared_ptr<NativeChannel> ch;
        {
            std::lock_guard<std::mutex> lock(g_channels_mutex);
            auto it = g_channels.find(id);
            if (it == g_channels.end()) {
                throw RuntimeError(Token{TokenType::Identifier, "chan_recv_timeout", 0, 0}, "Invalid channel ID.");
            }
            ch = it->second;
        }
        std::unique_lock<std::mutex> lk(ch->mtx);
        bool ok = ch->cv_recv.wait_for(lk, std::chrono::milliseconds(timeout_ms), [&]() {
            return !ch->queue.empty() || ch->closed;
        });
        if (!ok || (ch->queue.empty() && ch->closed)) {
            return Value();
        }
        Value val = ch->queue.front();
        ch->queue.pop_front();
        if (ch->capacity > 0) {
            ch->cv_send.notify_one();
        }
        return val;
    });

    def("chan_close", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        int64_t id = static_cast<int64_t>(args[0].as_number());
        std::shared_ptr<NativeChannel> ch;
        {
            std::lock_guard<std::mutex> lock(g_channels_mutex);
            auto it = g_channels.find(id);
            if (it == g_channels.end()) return Value(false);
            ch = it->second;
        }
        std::lock_guard<std::mutex> lk(ch->mtx);
        ch->closed = true;
        ch->cv_recv.notify_all();
        ch->cv_send.notify_all();
        return Value(true);
    });

    def("chan_len", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        int64_t id = static_cast<int64_t>(args[0].as_number());
        std::lock_guard<std::mutex> lock(g_channels_mutex);
        auto it = g_channels.find(id);
        if (it == g_channels.end()) return Value(0.0);
        std::lock_guard<std::mutex> lk(it->second->mtx);
        return Value(static_cast<double>(it->second->queue.size()));
    });

    def("spawn", 1, [this](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_callable()) {
            throw RuntimeError(Token{TokenType::Identifier, "spawn", 0, 0}, "spawn() expects a callable function.");
        }
        auto callable = args[0].as_callable();
        auto globals = this->globals_;
        // Use spawn registry instead of raw detach — enables graceful shutdown.
        std::thread t([callable, globals]() {
            try {
                Interpreter thread_interp(globals, std::cout);
                callable->call(thread_interp, {});
            } catch (...) {
                // Silently swallow unhandled exceptions in spawned tasks.
                // Future: propagate via a task result channel.
            }
        });
        g_spawn_registry.add(std::move(t));
        return Value(true);
    });

    // GC Builtins
    def("gc_collect", 0, [](Interpreter&, const std::vector<Value>&) -> Value {
        size_t broken = gc::GCRegistry::instance().collect();
        return Value(static_cast<double>(broken));
    });

    def("gc_stats", 0, [](Interpreter&, const std::vector<Value>&) -> Value {
        auto stats = gc::GCRegistry::instance().stats();
        auto m = std::make_shared<std::unordered_map<std::string, Value>>();
        (*m)["total_collections"] = Value(static_cast<double>(stats.total_collections));
        (*m)["cycles_broken"]     = Value(static_cast<double>(stats.cycles_broken));
        (*m)["objects_freed"]     = Value(static_cast<double>(stats.objects_freed));
        (*m)["roots_traced"]      = Value(static_cast<double>(stats.roots_traced));
        return Value(m);
    });

    // Native WaitGroup Builtins
    def("wg_create", 0, [](Interpreter&, const std::vector<Value>&) -> Value {
        auto wg = std::make_shared<NativeWaitGroup>();
        std::lock_guard<std::mutex> lock(g_wait_groups_mutex);
        int64_t id = g_next_wg_id++;
        g_wait_groups[id] = wg;
        return Value(static_cast<double>(id));
    });

    def("wg_add", 2, [](Interpreter&, const std::vector<Value>& args) -> Value {
        int64_t id = static_cast<int64_t>(args[0].as_number());
        int64_t delta = static_cast<int64_t>(args[1].as_number());
        std::shared_ptr<NativeWaitGroup> wg;
        {
            std::lock_guard<std::mutex> lock(g_wait_groups_mutex);
            auto it = g_wait_groups.find(id);
            if (it == g_wait_groups.end()) return Value(false);
            wg = it->second;
        }
        std::lock_guard<std::mutex> lk(wg->mtx);
        wg->counter += delta;
        return Value(true);
    });

    def("wg_done", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        int64_t id = static_cast<int64_t>(args[0].as_number());
        std::shared_ptr<NativeWaitGroup> wg;
        {
            std::lock_guard<std::mutex> lock(g_wait_groups_mutex);
            auto it = g_wait_groups.find(id);
            if (it == g_wait_groups.end()) return Value(false);
            wg = it->second;
        }
        std::lock_guard<std::mutex> lk(wg->mtx);
        wg->counter--;
        if (wg->counter <= 0) {
            wg->counter = 0;
            wg->cv.notify_all();
        }
        return Value(true);
    });

    def("wg_wait", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        int64_t id = static_cast<int64_t>(args[0].as_number());
        std::shared_ptr<NativeWaitGroup> wg;
        {
            std::lock_guard<std::mutex> lock(g_wait_groups_mutex);
            auto it = g_wait_groups.find(id);
            if (it == g_wait_groups.end()) return Value(false);
            wg = it->second;
        }
        std::unique_lock<std::mutex> lk(wg->mtx);
        wg->cv.wait(lk, [&]() { return wg->counter <= 0; });
        return Value(true);
    });

    // Native Mutex Builtins
    def("mutex_create", 0, [](Interpreter&, const std::vector<Value>&) -> Value {
        auto m = std::make_shared<NativeMutex>();
        std::lock_guard<std::mutex> lock(g_native_mutexes_mutex);
        int64_t id = g_next_mutex_id++;
        g_native_mutexes[id] = m;
        return Value(static_cast<double>(id));
    });

    def("mutex_lock", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        int64_t id = static_cast<int64_t>(args[0].as_number());
        std::shared_ptr<NativeMutex> m;
        {
            std::lock_guard<std::mutex> lock(g_native_mutexes_mutex);
            auto it = g_native_mutexes.find(id);
            if (it == g_native_mutexes.end()) return Value(false);
            m = it->second;
        }
        m->mtx.lock();
        m->locked = true;
        return Value(true);
    });

    def("mutex_unlock", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        int64_t id = static_cast<int64_t>(args[0].as_number());
        std::shared_ptr<NativeMutex> m;
        {
            std::lock_guard<std::mutex> lock(g_native_mutexes_mutex);
            auto it = g_native_mutexes.find(id);
            if (it == g_native_mutexes.end()) return Value(false);
            m = it->second;
        }
        m->locked = false;
        m->mtx.unlock();
        return Value(true);
    });
}

void Interpreter::interpret(const std::vector<std::shared_ptr<Stmt>>& statements) {
    try {
        for (const auto& stmt : statements) {
            if (stmt) {
                execute(*stmt);
            }
        }
    } catch (RuntimeError& error) {
        if (error.stack_trace().empty()) {
            error.set_stack_trace(call_stack_);
        }
        throw;
    }
}

void Interpreter::execute(const Stmt& stmt) {
    if (step_callback_) {
        step_callback_(*this, stmt);
    }
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
    if (expr.op.type == TokenType::QuestionQuestion) {
        Value left = evaluate(*expr.left);
        if (!left.is_nil()) {
            return left;
        }
        return evaluate(*expr.right);
    }

    if (expr.op.type == TokenType::Is) {
        Value left = evaluate(*expr.left);
        std::string type_name;
        if (auto* var_expr = dynamic_cast<VariableExpr*>(expr.right.get())) {
            type_name = var_expr->name.lexeme;
        } else {
            type_name = evaluate(*expr.right).to_string();
        }
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
            if (it != m->end() && it->second.to_string() == type_name) {
                return Value(true);
            }
        }
        return Value(false);
    }

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
    int min_a = function->min_arity();
    int max_a = function->arity();
    if (max_a >= 0 && (static_cast<int>(arguments.size()) < min_a || static_cast<int>(arguments.size()) > max_a)) {
        if (min_a == max_a) {
            throw RuntimeError(expr.paren, "Expected " +
                std::to_string(max_a) + " arguments but got " +
                std::to_string(arguments.size()) + ".");
        } else {
            throw RuntimeError(expr.paren, "Expected between " +
                std::to_string(min_a) + " and " + std::to_string(max_a) + " arguments but got " +
                std::to_string(arguments.size()) + ".");
        }
    }

    return function->call(*this, arguments);
}

Value Interpreter::visit(const ArrayExpr& expr) {
    auto list = std::make_shared<std::vector<Value>>();
    for (const auto& elem : expr.elements) {
        if (auto* unary = dynamic_cast<UnaryExpr*>(elem.get())) {
            if (unary->op.type == TokenType::DotDotDot) {
                Value val = evaluate(*unary->right);
                if (val.is_array()) {
                    for (const auto& item : *val.as_array()) {
                        list->push_back(item);
                    }
                } else if (val.is_string()) {
                    for (char ch : val.as_string()) {
                        list->push_back(Value(std::string(1, ch)));
                    }
                } else {
                    list->push_back(val);
                }
                continue;
            }
        }
        list->push_back(evaluate(*elem));
    }
    return Value(list);
}

Value Interpreter::visit(const MapExpr& expr) {
    auto map = std::make_shared<std::unordered_map<std::string, Value>>();
    for (const auto& [k, v] : expr.entries) {
        if (auto* unary = dynamic_cast<UnaryExpr*>(k.get())) {
            if (unary->op.type == TokenType::DotDotDot) {
                Value val = evaluate(*unary->right);
                if (val.is_map()) {
                    for (const auto& [sub_k, sub_v] : *val.as_map()) {
                        (*map)[sub_k] = sub_v;
                    }
                }
                continue;
            }
        }
        Value key_val = evaluate(*k);
        Value val_val = v ? evaluate(*v) : Value();
        (*map)[key_val.to_string()] = val_val;
    }
    return Value(map);
}

Value Interpreter::visit(const IndexExpr& expr) {
    Value obj = evaluate(*expr.object);
    if (expr.is_optional && obj.is_nil()) {
        return Value();
    }
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

    if (expr.is_optional) {
        return Value();
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
    if (expr.is_optional && obj.is_nil()) {
        return Value();
    }
    if (obj.is_map()) {
        auto& m = *obj.as_map();
        auto it = m.find(expr.name.lexeme);
        if (it == m.end()) return Value();
        return it->second;
    }
    if (expr.is_optional) {
        return Value();
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
        environment_,
        expr.param_defaults
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

Value Interpreter::visit(const MatchExpr& expr) {
    Value target = evaluate(*expr.value);
    for (const auto& arm : expr.arms) {
        if (arm.is_wildcard) {
            return evaluate(*arm.body);
        }
        Value pattern_val = evaluate(*arm.pattern);
        if (target == pattern_val) {
            return evaluate(*arm.body);
        }
    }
    return Value();
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
        try {
            execute(*stmt.body);
        } catch (const BreakException&) {
            break;
        } catch (const ContinueException&) {
            continue;
        }
    }
}

void Interpreter::visit(const ForInStmt& stmt) {
    Value iterable = evaluate(*stmt.iterable);

    if (iterable.is_array()) {
        const auto& arr = *iterable.as_array();
        for (const auto& item : arr) {
            auto loop_env = std::make_shared<Environment>(environment_);
            loop_env->define(stmt.variable.lexeme, item);
            try {
                execute_block({stmt.body}, loop_env);
            } catch (const BreakException&) {
                break;
            } catch (const ContinueException&) {
                continue;
            }
        }
    } else if (iterable.is_map()) {
        const auto& m = *iterable.as_map();
        for (const auto& [k, v] : m) {
            auto loop_env = std::make_shared<Environment>(environment_);
            loop_env->define(stmt.variable.lexeme, Value(k));
            try {
                execute_block({stmt.body}, loop_env);
            } catch (const BreakException&) {
                break;
            } catch (const ContinueException&) {
                continue;
            }
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

void Interpreter::visit(const ImportStmt& stmt) {
    std::string path = stmt.module_path;
    std::string base = path;
    size_t last_slash = path.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        base = path.substr(last_slash + 1);
    }

    std::string rel_path = path;
    if (rel_path.rfind("stdlib/", 0) == 0) {
        rel_path = rel_path.substr(7);
    }

    std::vector<std::string> candidates = {
        path + ".rvn",
        path + "/" + base + ".rvn",
        "stdlib/" + rel_path + ".rvn",
        "stdlib/" + rel_path + "/" + base + ".rvn",
        "../stdlib/" + rel_path + ".rvn",
        "../stdlib/" + rel_path + "/" + base + ".rvn",
        "/home/ran/Raven/stdlib/" + rel_path + ".rvn",
        "/home/ran/Raven/stdlib/" + rel_path + "/" + base + ".rvn",
        "/usr/local/share/raven/stdlib/" + rel_path + ".rvn",
        "/usr/local/share/raven/stdlib/" + rel_path + "/" + base + ".rvn",
        "raven_modules/" + path + ".rvn",
        path,
        "stdlib/" + rel_path,
        "/home/ran/Raven/stdlib/" + rel_path,
        "/usr/local/share/raven/stdlib/" + rel_path
    };

    // Check executable path relative stdlib
    std::error_code ec_exe;
    auto exe_path = std::filesystem::read_symlink("/proc/self/exe", ec_exe);
    if (!ec_exe && !exe_path.empty()) {
        auto exe_dir = exe_path.parent_path();
        candidates.push_back((exe_dir / "stdlib" / (rel_path + ".rvn")).string());
        candidates.push_back((exe_dir.parent_path() / "stdlib" / (rel_path + ".rvn")).string());
        candidates.push_back((exe_dir.parent_path() / "share" / "raven" / "stdlib" / (rel_path + ".rvn")).string());
    }

    std::string resolved;
    for (const auto& c : candidates) {
        std::error_code ec;
        if (std::filesystem::is_regular_file(c, ec)) {
            resolved = c;
            break;
        }
    }

    if (resolved.empty()) {
        throw RuntimeError(stmt.keyword, "Could not resolve module: " + stmt.module_path);
    }

    if (imported_modules_.find(resolved) != imported_modules_.end()) {
        return;
    }
    imported_modules_.insert(resolved);

    std::ifstream file(resolved);
    if (!file.is_open()) {
        throw RuntimeError(stmt.keyword, "Could not open module file: " + resolved);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto statements = parser.parse();
    if (parser.has_error()) {
        std::string err_msg = "Parse error in module '" + resolved + "':";
        for (const auto& e : parser.errors()) {
            err_msg += "\n  " + e;
        }
        throw RuntimeError(stmt.keyword, err_msg);
    }

    auto previous = environment_;
    environment_ = globals_;
    try {
        for (const auto& s : statements) {
            if (s) execute(*s);
        }
        environment_ = previous;
    } catch (...) {
        environment_ = previous;
        throw;
    }
}

class StructConstructor : public Callable {
public:
    StructConstructor(std::string name, std::vector<std::string> fields)
        : name_(std::move(name)), fields_(std::move(fields)) {}

    int arity() const override { return static_cast<int>(fields_.size()); }

    Value call(Interpreter&, const std::vector<Value>& args) override {
        auto map = std::make_shared<std::unordered_map<std::string, Value>>();
        (*map)["__type__"] = Value(name_);
        for (size_t i = 0; i < fields_.size() && i < args.size(); ++i) {
            (*map)[fields_[i]] = args[i];
        }
        return Value(map);
    }

    std::string to_string() const override {
        return "<struct " + name_ + ">";
    }

private:
    std::string name_;
    std::vector<std::string> fields_;
};

void Interpreter::visit(const StructStmt& stmt) {
    std::vector<std::string> field_names;
    for (const auto& f : stmt.fields) {
        field_names.push_back(f.lexeme);
    }
    auto constructor = std::make_shared<StructConstructor>(stmt.name.lexeme, std::move(field_names));
    environment_->define(stmt.name.lexeme, Value(constructor));
}

void Interpreter::visit(const BreakStmt& /*stmt*/) {
    throw BreakException{};
}

void Interpreter::visit(const ContinueStmt& /*stmt*/) {
    throw ContinueException{};
}

void Interpreter::visit(const FunctionStmt& stmt) {
    auto function = std::make_shared<UserFunction>(
        stmt.name,
        stmt.params,
        stmt.body,
        environment_,
        stmt.param_defaults
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
