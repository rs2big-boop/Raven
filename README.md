# MiniLang

A modern, educational programming language built in C++20 from scratch.

MiniLang demonstrates the core concepts of language implementation:
- **Lexical Analysis (Lexer)**: Streaming characters into structured Tokens with line and column tracking.
- **Syntactic Analysis (Parser)**: Recursive descent parsing generating an Abstract Syntax Tree (AST). *(Coming next)*
- **Tree-Walk Interpreter**: Evaluating expressions, handling variables, scopes, and control flow. *(Coming next)*

---

## Project Structure

```
MiniLang/
├── CMakeLists.txt
├── README.md
├── docs/
│   ├── architecture.md
│   └── language-spec.md
├── examples/
│   └── hello.mini
├── include/
│   └── minilang/
│       ├── token.hpp
│       ├── lexer.hpp
│       ├── ast.hpp          (planned)
│       ├── parser.hpp       (planned)
│       └── interpreter.hpp  (planned)
├── src/
│   ├── main.cpp
│   ├── lexer.cpp
│   ├── parser.cpp           (planned)
│   └── interpreter.cpp      (planned)
└── tests/
    ├── lexer_tests.cpp
    ├── parser_tests.cpp      (planned)
    └── interpreter_tests.cpp (planned)
```

---

## Building and Testing

### Prerequisites
- C++20 compatible compiler (e.g., GCC 12+, Clang 15+)
- CMake 3.20+
- GoogleTest

### Build
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### Run Tests
```bash
ctest --test-dir build --output-on-failure
```

### Run the CLI
```bash
./build/minilang examples/hello.mini
```
