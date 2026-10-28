# Raven

> A modern, clean programming language built from scratch in C++20.
> File extension: `.rvn` · CLI: `raven`

Raven is an educational yet production-grade tree-walk interpreter implementing lexical analysis, recursive descent parsing with operator precedence, lexical environments, functions with closures, and control flow.

---

## Example (`hello.rvn`)

```rvn
let name = "Ran";
let age = 20;

print("Hello, " + name);

if (age >= 18) {
    print("Adult");
}
```

**Output:**
```text
Hello, Ran
Adult
```

---

## Features

- **Lexer**: Tokenization with 1-based line and column tracking, string escape handling, comments (`//` and `/* ... */`).
- **Parser**: Recursive descent parser supporting operator precedence, grouping, and error recovery (`synchronize()`).
- **Data Types**: Numbers (doubles/integers), Strings (with `+` concatenation), Booleans (`true`/`false`), Nil, and First-class Functions.
- **Variables & Scopes**: Lexical scoping with `let`, variable mutation, and nested block shadowing `{ ... }`.
- **Control Flow**: `if` / `else` branching and `while` loops.
- **Functions**: User-defined functions `fn name(params) { ... }`, recursion, and `return` statements.
- **Interactive REPL & CLI**: Run `.rvn` scripts or launch an interactive shell.
- **Automated Tests**: Comprehensive GoogleTest test suite covering Lexer, Parser, and Interpreter.
- **CI / GitHub Actions**: Automated builds and test runs on every commit.

---

## Project Structure

```
Raven/
├── CMakeLists.txt
├── README.md
├── docs/
│   ├── architecture.md
│   └── language-spec.md
├── examples/
│   ├── hello.rvn
│   ├── fibonacci.rvn
│   └── while.rvn
├── include/
│   └── raven/
│       ├── ast.hpp
│       ├── environment.hpp
│       ├── interpreter.hpp
│       ├── lexer.hpp
│       ├── token.hpp
│       └── value.hpp
├── src/
│   ├── interpreter.cpp
│   ├── lexer.cpp
│   ├── main.cpp
│   └── parser.cpp
└── tests/
    ├── interpreter_tests.cpp
    ├── lexer_tests.cpp
    └── parser_tests.cpp
```

---

## Building & Testing

### Prerequisites
- C++20 compiler (GCC 12+, Clang 15+, or MSVC)
- CMake 3.20+
- GoogleTest

### Build
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Run Tests
```bash
ctest --test-dir build --output-on-failure
```

### Run Scripts
```bash
./build/raven examples/hello.rvn
./build/raven examples/fibonacci.rvn
./build/raven examples/while.rvn
```

### Start Interactive REPL
```bash
./build/raven
```
```text
Raven 0.1.0 Interactive Shell
Type 'exit' or press Ctrl+D to quit.
raven> let a = 10;
raven> let b = 20;
raven> print(a + b);
30
raven> exit
```
