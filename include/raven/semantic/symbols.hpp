#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <memory>

namespace raven::semantic {

enum class SymbolKind {
    Variable,   // let x = ...
    Function,   // fn foo(...) { ... }
    Parameter,  // fn parameter
    Builtin,    // native function
};

struct Symbol {
    std::string name;
    SymbolKind  kind;
    int         depth;       // Scope depth where defined
    int         slot;        // Local variable slot (for compiler)
    bool        is_defined;  // Fully initialized?
    bool        is_const;    // Immutable?

    Symbol(std::string n, SymbolKind k, int d, int s = 0)
        : name(std::move(n)), kind(k), depth(d), slot(s),
          is_defined(true), is_const(false) {}
};

} // namespace raven::semantic
