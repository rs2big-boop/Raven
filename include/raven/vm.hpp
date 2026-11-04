#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "raven/bytecode.hpp"
#include "raven/value.hpp"

namespace raven {

// Forward declaration to avoid circular header dependency.
// The full Interpreter is included in vm.cpp.
class Interpreter;

enum class InterpretResult {
    Ok,
    CompileError,
    RuntimeError
};

struct CallFrame {
    size_t return_ip;
    size_t slots;
};

class VM {
public:
    explicit VM(std::ostream& out = std::cout);
    ~VM();  // Defined in vm.cpp where Interpreter is fully known

    InterpretResult run(const Chunk& chunk);
    Value execute_function(const BytecodeFunction& fn, const std::vector<Value>& args);

    void push(Value value);
    Value pop();
    Value peek(int distance = 0) const;

    std::unordered_map<std::string, Value>& globals() { return globals_; }

private:
    InterpretResult run_chunk(const Chunk& chunk, size_t initial_ip, size_t target_frame_depth);

    std::ostream& out_;
    std::vector<Value> stack_;
    std::unordered_map<std::string, Value> globals_;
    std::vector<CallFrame> frames_;
    // Lazily-created interpreter bridge used to execute native/builtin callables.
    // Reused across native calls within a single run() invocation.
    std::unique_ptr<Interpreter> native_interp_;
};

} // namespace raven
