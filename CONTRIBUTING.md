# Contributing to the Raven Programming Language

Welcome! We are thrilled that you are interested in contributing to Raven. Raven is built in the open, by developers, for developers.

---

## 1. Development Environment Setup

### Prerequisites
- **C++20 Compiler**: GCC 11+ or Clang 14+
- **Build System**: CMake 3.20+
- **Testing Framework**: GoogleTest (`libgtest-dev`)
- **Git**

### Ubuntu / Debian
```bash
sudo apt update
sudo apt install -y build-essential cmake libgtest-dev git
```

### macOS (Homebrew)
```bash
brew install cmake googletest
```

---

## 2. Building and Testing

We provide canonical Go-style shell scripts in the repository root:

```bash
# Fast incremental build
./make.bash

# Full rebuild, run all 74 automated tests, and run stdlib tour
./all.bash

# Run any script with the tree-walk interpreter
./run.bash examples/hello.rvn

# Run the test suite directly with ctest
ctest --test-dir build --output-on-failure

# Package distribution release tarball
./dist.bash
```

---

## 3. How to Contribute

### A. Adding a Native Builtin Function
1. Open `src/runtime/interpreter.cpp`.
2. Locate `setup_builtins()`.
3. Use the `def` helper lambda:
   ```cpp
   def("my_function", 1, [](Interpreter& interp, const std::vector<Value>& args) -> Value {
       // Validate arguments
       if (!args[0].is_string()) {
           throw RuntimeError(Token{TokenType::Identifier, "my_function", 0, 0}, "Expected string argument.");
       }
       // Perform operation
       return Value(args[0].as_string() + "_suffix");
   });
   ```
4. Add automated unit test in `tests/interpreter_tests.cpp`.

### B. Adding a Standard Library Module
1. Create a new `.rvn` file in `stdlib/` (e.g. `stdlib/my_module.rvn`).
2. Write idiomatic Raven code:
   ```raven
   // stdlib/my_module.rvn
   fn transform_data(item) {
       return item * 2;
   }
   ```
3. Document the module in `docs/language-spec.md`.
4. Add an example in `examples/` and a unit test in `tests/interpreter_tests.cpp`.

### C. Adding a VM Opcode
1. Add the enum value to `enum class OpCode` in `include/raven/bytecode.hpp`.
2. Update `disassemble_instruction()` in `include/raven/bytecode.hpp`.
3. Update `BytecodeVerifier` in `include/raven/bytecode_verifier.hpp`.
4. Implement compilation in `src/compiler/compiler.cpp`.
5. Implement execution handling in `src/runtime/vm.cpp`.
6. Add differential test in `tests/interpreter_tests.cpp`.

---

## 4. Pull Request Checklist

Before submitting a Pull Request, verify:
- [ ] Code compiles cleanly with zero warnings (`./make.bash`).
- [ ] All 74 unit tests pass (`./all.bash`).
- [ ] New features include automated tests in `tests/`.
- [ ] No regression in Differential Testing (`run_code` vs `run_vm_code`).
- [ ] Documentation updated in `docs/` and `CHANGELOG.md`.

Thank you for helping make Raven the most productive language for modern developers!
