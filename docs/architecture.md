# Raven Architecture & Advanced Runtime Design

Raven combines the expressiveness of Python, the functional elegance of Rust/Elixir, and the dual-engine architecture of the JVM: a high-level AST interpreter alongside a Stack-based Bytecode Virtual Machine (VM).

```
+-----------------------------------------------------------+
|                       Source Code                         |
|                   (e.g., showcase.rvn)                    |
+-----------------------------+-----------------------------+
                              |
                              v
+-----------------------------------------------------------+
|                          Lexer                            |
|             Converts raw text into Token stream           |
+-----------------------------+-----------------------------+
                              |
                              v
+-----------------------------------------------------------+
|                         Parser                            |
|        Validates syntax and builds the Abstract           |
|                  Syntax Tree (AST)                        |
+-----------------------------+-----------------------------+
                              |
              +---------------+---------------+
              |                               |
              v                               v
+-----------------------------+ +---------------------------+
|      Tree-Walk Engine       | |     Bytecode Compiler     |
|   - AST Visitor Execution   | |   - Emits OpCode Chunks   |
|   - Dynamic Lexical Scopes  | |   - Constant Pool Table   |
|   - Standard Library Engine | |   - Disassembler Engine   |
+-----------------------------+ +-------------+-------------+
                                              |
                                              v
                                +---------------------------+
                                |    Raven Virtual Machine  |
                                |   - Stack-Based VM        |
                                |   - High-Speed Execution  |
                                +---------------------------+
```

---

## 1. Dual Execution Engines

### Engine A: Tree-Walk Interpreter (`Interpreter`)
- Direct AST traversal using modern C++20 `Visitor` pattern.
- Immediate execution without compilation pauses.
- Dynamic environment frames (`Environment`) with full support for closures, lexical shadowing, and user exceptions (`try` / `catch` / `throw`).

### Engine B: Raven Virtual Machine (`VM`)
- Emits byte-sized opcodes into contiguous linear memory `Chunk`.
- Stack-based architecture mimicking JVM / CPython:
  - `OP_CONSTANT`, `OP_ADD`, `OP_SUBTRACT`, `OP_MULTIPLY`, `OP_DIVIDE`, `OP_MODULO`
  - `OP_EQUAL`, `OP_GREATER_EQUAL`, `OP_LESS_EQUAL`, etc.
  - `OP_JUMP_IF_FALSE`, `OP_JUMP`, `OP_LOOP`
  - `OP_BUILD_ARRAY`, `OP_INDEX_GET`, `OP_INDEX_SET`
  - `OP_DEFINE_GLOBAL`, `OP_GET_GLOBAL`, `OP_SET_GLOBAL`
- Built-in Disassembler accessible via `raven --disasm <file.rvn>`.

---

## 2. Revolutionary Features

1. **Native Collections**:
   - Dynamic Arrays `[1, "two", 3.0]` with negative indexing (`arr[-1]`), slicing, and push/pop mutations.
   - Hash Maps / Objects `{"name": "Ran"}` with both bracket access `user["name"]` and dot syntax `user.name`.
2. **Functional Pipelines (`|>`)**:
   - Left-to-right composition operator:
     ```rvn
     data |> filter_fn |> map_fn |> print;
     ```
3. **First-Class Lambdas & Closures**:
   - Anonymous functions `fn(x) { return x * 2; }` capturing outer lexical scope.
4. **Collection Iteration (`for-in`)**:
   - `for (let item in collection) { ... }` natively supporting arrays and maps.
5. **Modern Exception Handling**:
   - `try { ... } catch (err) { ... }` combined with `throw value;`.
6. **Standard Library**:
   - `len()`, `push()`, `pop()`, `keys()`, `values()`, `range()`, `map()`, `filter()`, `reduce()`, `type()`, `time()`, `assert()`, `str()`, `num()`.
