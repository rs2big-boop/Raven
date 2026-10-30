// builtin_registry.cpp — Registry for native builtin functions
#include <string>
#include <unordered_map>
#include <vector>

namespace raven::runtime {

struct BuiltinInfo {
    std::string name;
    int arity;
    std::string signature;
    std::string doc;
};

class BuiltinRegistry {
public:
    static BuiltinRegistry& instance() {
        static BuiltinRegistry reg;
        return reg;
    }

    void register_builtin(BuiltinInfo info) {
        builtins_[info.name] = std::move(info);
    }

    bool has(const std::string& name) const {
        return builtins_.count(name) > 0;
    }

    std::vector<std::string> all_names() const {
        std::vector<std::string> names;
        names.reserve(builtins_.size());
        for (const auto& [k, _] : builtins_) names.push_back(k);
        return names;
    }

    size_t count() const { return builtins_.size(); }

private:
    BuiltinRegistry() {
        std::vector<BuiltinInfo> known = {
            {"print", 1, "print(v)", "Print value to stdout"},
            {"len", 1, "len(v)", "Length of container or string"},
            {"type", 1, "type(v)", "Type name as string"},
            {"str", 1, "str(v)", "Convert to string"},
            {"num", 1, "num(v)", "Convert to number"},
            {"abs", 1, "abs(x)", "Absolute value"},
            {"sqrt", 1, "sqrt(x)", "Square root"},
            {"range", 1, "range(n)", "Generate range array"},
            {"push", 2, "push(arr, v)", "Append to array"},
            {"pop", 1, "pop(arr)", "Remove last element"},
            {"keys", 1, "keys(m)", "Map keys"},
            {"values", 1, "values(m)", "Map values"},
            {"time", 0, "time()", "Current timestamp"},
            {"sleep", 1, "sleep(ms)", "Sleep milliseconds"},
        };
        for (auto& b : known) register_builtin(std::move(b));
    }

    std::unordered_map<std::string, BuiltinInfo> builtins_;
};

} // namespace raven::runtime
