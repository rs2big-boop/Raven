#include "raven/interpreter.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <random>
#include <sstream>
#include <thread>

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

    // JSON Serialization
    def("to_json", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        return Value(value_to_json(args[0]));
    });
    def("parse_json", 1, [](Interpreter&, const std::vector<Value>& args) -> Value {
        if (!args[0].is_string()) throw RuntimeError(Token{TokenType::Identifier, "parse_json", 0, 0}, "parse_json() expects JSON string.");
        SimpleJsonParser parser(args[0].as_string());
        return parser.parse();
    });
}

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
    if (function->arity() >= 0 && arguments.size() != static_cast<std::size_t>(function->arity())) {
        throw RuntimeError(expr.paren, "Expected " +
            std::to_string(function->arity()) + " arguments but got " +
            std::to_string(arguments.size()) + ".");
    }

    return function->call(*this, arguments);
}

Value Interpreter::visit(const ArrayExpr& expr) {
    auto list = std::make_shared<std::vector<Value>>();
    for (const auto& elem : expr.elements) {
        list->push_back(evaluate(*elem));
    }
    return Value(list);
}

Value Interpreter::visit(const MapExpr& expr) {
    auto map = std::make_shared<std::unordered_map<std::string, Value>>();
    for (const auto& [k, v] : expr.entries) {
        Value key_val = evaluate(*k);
        Value val_val = evaluate(*v);
        (*map)[key_val.to_string()] = val_val;
    }
    return Value(map);
}

Value Interpreter::visit(const IndexExpr& expr) {
    Value obj = evaluate(*expr.object);
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
    if (obj.is_map()) {
        auto& m = *obj.as_map();
        auto it = m.find(expr.name.lexeme);
        if (it == m.end()) return Value();
        return it->second;
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
        environment_
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
        execute(*stmt.body);
    }
}

void Interpreter::visit(const ForInStmt& stmt) {
    Value iterable = evaluate(*stmt.iterable);

    if (iterable.is_array()) {
        const auto& arr = *iterable.as_array();
        for (const auto& item : arr) {
            auto loop_env = std::make_shared<Environment>(environment_);
            loop_env->define(stmt.variable.lexeme, item);
            execute_block({stmt.body}, loop_env);
        }
    } else if (iterable.is_map()) {
        const auto& m = *iterable.as_map();
        for (const auto& [k, v] : m) {
            auto loop_env = std::make_shared<Environment>(environment_);
            loop_env->define(stmt.variable.lexeme, Value(k));
            execute_block({stmt.body}, loop_env);
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
    std::vector<std::string> candidates = {
        path,
        path + ".rvn",
        "../" + path,
        "../" + path + ".rvn",
        "stdlib/" + path + ".rvn",
        "stdlib/" + path,
        "../stdlib/" + path + ".rvn",
        "../stdlib/" + path
    };

    std::string resolved;
    for (const auto& c : candidates) {
        std::ifstream test(c);
        if (test.good()) {
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
