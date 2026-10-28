# Raven Programming Language

> **Raven** combines the expressiveness of **Python**, the functional pipelines of **Rust/Elixir**, and the dual-engine architecture of **Java/JVM**:
> featuring both an **AST Tree-Walk Interpreter** and the **RVM (Raven Virtual Machine)** with the **Corvus** execution engine.

---

## The Raven Toolchain

Similar to Java (`javac` + `java` JVM), Raven provides a dedicated, full-scale compiler and virtual machine toolchain:

| Tool | Analogous to | Description |
|---|---|---|
| **`ravenc`** | `javac` | **Raven Compiler**: Compiles `.rvn` source code into optimized `.rvc` binary bytecode with compile-time constant folding and dead-code elimination. |
| **`rvm`** | `java` (JVM) | **Raven Virtual Machine (Corvus Engine)**: High-speed stack-based virtual machine executing pre-compiled `.rvc` bytecode (or compiling on-the-fly). |
| **`raven`** | `python` / `node` | **Unified CLI**: Multi-purpose frontend for running scripts, launching the interactive REPL, or disassembling code. |

---

## File Formats

- **`.rvn`** — Human-readable Raven source code.
- **`.rvc`** — Raven Compiled Bytecode binary (Magic bytes `\x7fRVC`, Constant Pool, OpCode instructions).

---

## Quickstart

### 1. Compile to Bytecode with `ravenc`
```bash
./build/ravenc examples/hello.rvn -o hello.rvc -O
```
*Output:*
```text
Successfully compiled 'examples/hello.rvn' -> 'hello.rvc' (31 bytes bytecode, 9 constants)
```

### 2. Execute Bytecode with `rvm` (Corvus Engine)
```bash
./build/rvm -b hello.rvc
```
*Output:*
```text
Hello, Ran
Adult

[RVM Benchmark: execution finished in 21 us (0.021 ms)]
```

### 3. Disassemble Bytecode
```bash
./build/ravenc -d examples/hello.rvn
```
*Output:*
```text
== examples/hello.rvn (Bytecode Disassembly) ==
0000    1 OP_CONSTANT        0 ('Ran')
0002    | OP_DEFINE_GLOBAL   1 ('name')
0004    | OP_CONSTANT        2 ('20')
0006    2 OP_DEFINE_GLOBAL   3 ('age')
0008    4 OP_CONSTANT        4 ('Hello, ')
0010    | OP_GET_GLOBAL      5 ('name')
0012    | OP_ADD
0013    | OP_PRINT
0014    6 OP_GET_GLOBAL      6 ('age')
0016    4 OP_CONSTANT        7 ('18')
0018    6 OP_GREATER_EQUAL
0019    | OP_JUMP_IF_FALSE   -> 29
0022    | OP_POP
0023    | OP_CONSTANT        8 ('Adult')
0025    | OP_PRINT
0026    | OP_JUMP            -> 30
0029    | OP_POP
0030    | OP_RETURN
===============================================
```

### 4. Interactive REPL
```bash
./build/raven
```

---

## Language Highlights

### Rust-Style Pattern Matching (`match`)
```raven
let status_code = 404;
let message = match (status_code) {
    200 => "OK",
    404 => "Not Found",
    500 => "Internal Server Error",
    _   => "Unknown Status"
};
print(message); // Prints: Not Found
```

### Modular Standard Library (`import`)
```raven
import "stdlib/math";
import "stdlib/string";

print("pow(2, 8) = " + pow(2, 8));           // 256
print("factorial(5) = " + factorial(5));     // 120
print(pad_left("42", 6, "0"));               // 000042
```

### Functional Pipelines (`|>`) and First-Class Functions
```raven
let numbers = range(1, 10);
let square = fn(x) { return x * x; };
let is_even = fn(x) { return x % 2 == 0; };

let result = numbers 
    |> fn(arr) { return filter(arr, is_even); }
    |> fn(arr) { return map(arr, square); };
```

---

## Building and Testing

Raven includes a comprehensive test suite (39 automated unit & regression tests):

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```
