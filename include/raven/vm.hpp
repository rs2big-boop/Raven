#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "raven/bytecode.hpp"
#include "raven/value.hpp"

namespace raven {

enum class InterpretResult {
    Ok,
    CompileError,
    RuntimeError
};

class VM {
public:
    explicit VM(std::ostream& out = std::cout);

    InterpretResult run(const Chunk& chunk);

    void push(Value value);
    Value pop();
    Value peek(int distance = 0) const;

    std::unordered_map<std::string, Value>& globals() { return globals_; }

private:
    std::ostream& out_;
    std::vector<Value> stack_;
    std::unordered_map<std::string, Value> globals_;
};

} // namespace raven
