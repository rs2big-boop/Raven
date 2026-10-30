# Raven Architecture Specification v1.0.0

> **Document Version**: 1.0.0  
> **Status**: APPROVED & ACTIVE  
> **Audience**: Core Engine Developers, Toolchain Contributors, Systems Architects  
> **Repository**: [https://github.com/raven-lang/raven](file:///home/ran/Raven)

---

## Component Status Legend
Throughout this document, every architectural component is categorized according to its maturity:
- `[IMPLEMENTED]`: Fully implemented, verified by test suite (77/77 passing), and production ready.
- `[PARTIAL]`: Core functional implementation exists; advanced edge-case or performance optimizations ongoing.
- `[PLANNED]`: Architectural design specified; scheduled on the development roadmap.
- `[EXPERIMENTAL]`: Available for testing and evaluation; API subject to refinement before stabilization.

---

## Table of Contents
1. [System Architecture Overview](#1-system-architecture-overview)
2. [Design Principles & Language Philosophy](#2-design-principles--language-philosophy)
3. [Source Code Lifecycle](#3-source-code-lifecycle)
4. [Frontend Architecture](#4-frontend-architecture)
5. [AST & Intermediate Representation](#5-ast--intermediate-representation)
6. [Type System & Semantic Analysis](#6-type-system--semantic-analysis)
7. [Tree-Walk Interpreter](#7-tree-walk-interpreter)
8. [Compiler Architecture](#8-compiler-architecture)
9. [Optimization Pipeline](#9-optimization-pipeline)
10. [Corvus Virtual Machine](#10-corvus-virtual-machine)
11. [Memory Management & Object Model](#11-memory-management--object-model)
12. [Runtime & Object Model](#12-runtime--object-model)
13. [Concurrency & Threading Model](#13-concurrency--threading-model)
14. [Async / Await / Promise / Future](#14-async--await--promise--future)
15. [Error Handling & Diagnostics](#15-error-handling--diagnostics)
16. [Module & Package System](#16-module--package-system)
17. [Standard Library Architecture](#17-standard-library-architecture)
18. [Bytecode & Binary Format](#18-bytecode--binary-format)
19. [Foreign Function Interface (FFI)](#19-foreign-function-interface-ffi)
20. [Security & Sandboxing](#20-security--sandboxing)
21. [Developer Toolchain](#21-developer-toolchain)
22. [LSP & Editor Integration](#22-lsp--editor-integration)
23. [Testing & Quality Assurance](#23-testing--quality-assurance)
24. [Performance & Benchmarking](#24-performance--benchmarking)
25. [Build System & Distribution](#25-build-system--distribution)
26. [Cross-Platform Support](#26-cross-platform-support)
27. [Compatibility & Versioning](#27-compatibility--versioning)
28. [Debugging & Profiling](#28-debugging--profiling)
29. [Documentation Architecture](#29-documentation-architecture)
30. [Future Architecture / Roadmap](#30-future-architecture--roadmap)

---

## 1. System Architecture Overview `[IMPLEMENTED]`

Raven is engineered as a **Dual-Engine Language System** with a unified Frontend:

```
                            ┌────────────────────────┐
                            │   Raven Source (.rvn)   │
                            └───────────┬────────────┘
                                        │
                         ┌──────────────▼─────────────┐
                         │   Lexer & Tokenizer        │ [IMPLEMENTED]
                         └──────────────┬─────────────┘
                                        │ Token Stream
                         ┌──────────────▼─────────────┐
                         │  Recursive Descent Parser  │ [IMPLEMENTED]
                         └──────────────┬─────────────┘
                                        │ Abstract Syntax Tree (AST)
                 ┌──────────────────────┴──────────────────────┐
                 │                                             │
      ┌──────────▼──────────┐                       ┌──────────▼──────────┐
      │ Tree-Walk Engine    │ [IMPLEMENTED]         │ Bytecode Compiler   │ [IMPLEMENTED]
      │ (Expr/Stmt Visitor) │                       │ (Chunk Codegen)     │
      └──────────┬──────────┘                       └──────────┬──────────┘
                 │                                             │ .rvc binary
                 │ Immediate Eval                              │
      ┌──────────▼──────────┐                       ┌──────────▼──────────┐
      │ Interactive REPL,   │                       │ Corvus Bytecode VM  │ [IMPLEMENTED]
      │ Developer Tools     │                       │ (Stack Interpreter) │
      │ (fmt, vet, lsp)     │                       │ (rvm)               │
      └─────────────────────┘                       └─────────────────────┘
```

Both engines access the same **Standard Library**, runtime **Value system**, and native **CSP Concurrency** primitives.

---

## 2. Design Principles & Language Philosophy `[IMPLEMENTED]`

1. **Productivity Without Ceremony**: Dynamic typing, pattern matching, default parameters, optional chaining `?.`, and null coalescing `??` minimize boilerplate.
2. **First-Class Concurrency**: Native CSP threads and channels are built directly into the runtime rather than relegated to third-party packages.
3. **Batteries Included Toolchain**: The single `raven` executable provides formatting, linting, docs generation, package management, benchmarking, and Language Server Protocol.
4. **Determinism and Transparency**: Explicit error messages with Levenshtein-based suggestions, full multi-frame call stacks, and reproducible builds via lockfiles.

---

## 3. Source Code Lifecycle `[IMPLEMENTED]`

```
Source (.rvn) ──> Lexer ──> Parser ──> AST ──┬──> Interpreter ──> Result
                                              └──> Compiler ──> .rvc ──> RVM ──> Result
```

1. **Ingestion**: Source read into memory via UTF-8 buffer.
2. **Lexical Scanning**: Scanned into discrete tokens with line/column coordinates.
3. **Syntactic Parsing**: Formed into a type-safe AST via recursive descent.
4. **Execution Route**:
   - Route A (`raven run <file>`): AST evaluated immediately by `Interpreter`.
   - Route B (`ravenc <file> -o out.rvc`): AST compiled into serialized bytecode.
   - Route C (`rvm out.rvc`): Bytecode loaded, validated, and executed by Corvus VM.

---

## 4. Frontend Architecture `[IMPLEMENTED]`

### Lexer (`src/frontend/lexer.cpp`)
- **Token Model**: Tagged tokens (`TokenType`) with exact source location (`line`, `column`).
- **Literal Scanning**:
  - Numbers: Integer and IEEE-754 double precision floating point literals.
  - Single-line Strings: Double-quoted strings with escape processing (`\n`, `\t`, `\r`, `\"`, `\\`).
  - Multi-line Strings: Triple-quoted strings `""" ... """` preserving internal formatting and newlines.
  - Format Strings: Interpolated `f"Hello {name}"` strings desugared into string concatenation tokens.
- **Operators**: Multi-character operator resolution: `??` (`QuestionQuestion`), `?.` (`QuestionDot`), `...` (`DotDotDot`), `=>` (`Arrow`), `|>` (`PipeGreater`), compound assignments (`+=`, `-=`, `*=`, `/=`, `%=`).

### Parser (`src/frontend/parser.cpp`)
- **Technique**: Predictive Recursive Descent with Pratt-style precedence stratification.
- **Precedence Hierarchy (Lowest to Highest)**:
  1. Assignment (`=`, `+=`, `-=`, etc.)
  2. Pipeline (`|>`)
  3. Null Coalescing (`??`)
  4. Logical OR (`||`)
  5. Logical AND (`&&`)
  6. Equality (`==`, `!=`)
  7. Comparison (`<`, `<=`, `>`, `>=`, `is`)
  8. Term (`+`, `-`)
  9. Factor (`*`, `/`, `%`)
  10. Unary (`!`, `-`, `...`)
  11. Call / Index / Property (`()`, `[]`, `.`, `?.`)
  12. Primary (literals, variables, groupings, arrays, maps, lambdas, match)
- **Error Recovery**: Panic mode synchronization discarding tokens until statement boundaries (semicolon or statement-starting keywords).

---

## 5. AST & Intermediate Representation `[IMPLEMENTED / PLANNED]`

### Current AST Hierarchy (`include/raven/ast.hpp`) `[IMPLEMENTED]`
- **Expressions (`Expr`)**:
  - `BinaryExpr`: Arithmetic, logical, comparison, `??`, and `is`.
  - `UnaryExpr`: Negation, boolean not, and spread `...`.
  - `LiteralExpr`: Embedded tagged `Value`.
  - `VariableExpr`: Identifier binding lookup.
  - `AssignExpr`: Variable update.
  - `CallExpr`: Function invocation with argument vector.
  - `ArrayExpr`: List literals with inline spread.
  - `MapExpr`: Key-value dictionaries with inline spread.
  - `IndexExpr`: Array/map/string access with optional `?.` flag.
  - `PropertyExpr`: Member access with optional `?.` flag.
  - `LambdaExpr`: Anonymous closure definitions with default parameters.
  - `PipelineExpr`: Functional `value |> function` pipelining.
  - `MatchExpr`: Pattern matching with wildcard `_` fallback arms.
- **Statements (`Stmt`)**:
  - `ExpressionStmt`, `PrintStmt`, `VarDeclStmt`, `BlockStmt`, `IfStmt`, `WhileStmt` (and desugared `LoopStmt`), `ForInStmt`, `TryCatchStmt`, `ThrowStmt`, `ImportStmt`, `StructStmt`, `BreakStmt`, `ContinueStmt`, `FunctionStmt`, `ReturnStmt`.

### Planned Raven IR (RIR) `[PLANNED for v1.2.0]`
A linear three-address code (TAC) Intermediate Representation to enable shared optimization passes across both the Corvus VM and future LLVM/Native backends.

---

## 6. Type System & Semantic Analysis `[IMPLEMENTED / PARTIAL]`

### Dynamic Tagged Value System `[IMPLEMENTED]`
Raven is dynamically typed with first-class type introspection:
- Primitive types: `Number`, `String`, `Bool`, `Nil`.
- Composite types: `Array`, `Map`, `Struct Instance`.
- Executable types: `Function`, `NativeFunction`, `BytecodeFunction`.

### Introspection Operator (`is`) `[IMPLEMENTED]`
```raven
if (val is Number) { ... }
if (val is String) { ... }
if (val is Array)  { ... }
```

### Static Semantic Analyzer `[PARTIAL via linter.cpp]`
- Variable resolution checking.
- Unused variable detection.
- Variable shadowing detection.
- Unreachable code detection after `return` or `throw`.

---

## 7. Tree-Walk Interpreter `[IMPLEMENTED]`

### Execution Model (`src/runtime/interpreter.cpp`)
- Implements `ExprVisitor<Value>` and `StmtVisitor<void>`.
- **Environment Chain**: Lexical scoping managed through linked `Environment` instances.
- **Closures**: `UserFunction` captures a `shared_ptr<Environment>` creating a lexical closure.
- **Stack Trace Tracking**: `CallStackGuard` RAII structures record function entry and exit, producing detailed multi-frame stack traces on exceptions:
```
[line 12, col 5] Runtime Error: Division by zero.
    at calculate_ratio (line 12, col 5)
    at process_data (line 34, col 10)
    at main (line 50, col 1)
```

---

## 8. Compiler Architecture `[IMPLEMENTED]`

### Pipeline (`src/compiler/compiler.cpp`)
The `Compiler` translates AST statements into linear bytecode instructions:
1. Emits opcodes and operands into a `Chunk`.
2. Encodes constant values into the chunk's `constant_pool`.
3. Emits back-patched jump offsets for `if`, `while`, `break`, and `continue`.
4. Emits local variable stack slot bindings.

---

## 9. Optimization Pipeline `[IMPLEMENTED / PARTIAL]`

### AST Constant Folding (`src/compiler/optimizer.cpp`) `[IMPLEMENTED]`
Pre-computes deterministic literal expressions at compile time:
- Arithmetic: `2 + 3 * 4` ──> `14`
- String concatenation: `"hello " + "world"` ──> `"hello world"`
- Logic: `true && false` ──> `false`

### Bytecode Peephole Optimization `[PARTIAL]`
- Removes redundant stack push/pop sequences (`OP_Pop` immediately following push).
- Folds jumps targeting unconditional jumps.

---

## 10. Corvus Virtual Machine `[IMPLEMENTED]`

### VM Architecture (`src/runtime/vm.cpp`)
The Corvus VM is an efficient stack-based execution engine:

```
Corvus VM Engine
├── Value Stack: vector<Value> (Operand evaluation stack)
├── Call Frame Stack: vector<CallFrame> (Max depth: 1000)
│   └── CallFrame: { return_ip, slots_base_index }
├── Instruction Pointer (IP): const uint8_t*
└── Global Storage: unordered_map<string, Value>
```

### Supported Opcodes
- Literals: `Constant`, `Nil`, `True`, `False`
- Arithmetic: `Add`, `Subtract`, `Multiply`, `Divide`, `Modulo`, `Negate`
- Comparison: `Equal`, `Greater`, `Less`, `Not`
- Variables: `GetGlobal`, `SetGlobal`, `DefineGlobal`, `GetLocal`, `SetLocal`
- Control Flow: `Jump`, `JumpIfFalse`, `Loop`, `Break`
- Functions: `Call`, `Return`
- Data Structures: `BuildArray`, `BuildMap`, `GetIndex`, `SetIndex`
- Builtins: `Print`, `Halt`

---

## 11. Memory Management & Object Model `[IMPLEMENTED / PLANNED]`

### Memory Strategy (See [ADR 0005](file:///home/ran/Raven/docs/architecture/decisions/0005-memory-management.md))
- **Scalars**: `double`, `bool`, `Nil` stored directly inside `Value` on the execution stack.
- **Heap Objects**: Strings, arrays, maps, callables, environments managed via `std::shared_ptr`.
- **Deterministic Lifetime**: Resources (file handles, sockets, mutexes) close promptly on scope exit.
- **Cycle Collection `[PLANNED]`**: Tracing cycle collector pass for circular graph references.
- **Arena Allocation `[IMPLEMENTED]`**: `stdlib/arena` for high-throughput batch allocation and zero-cost mass reset.

---

## 12. Runtime & Object Model `[IMPLEMENTED]`

### Object Hierarchy
```
Value (std::variant)
├── Nil
├── Boolean (bool)
├── Number (double)
├── String (std::string)
├── Callable (std::shared_ptr<Callable>)
│   ├── BuiltinFunction (Native C++ lambda)
│   ├── UserFunction (Raven AST closure)
│   ├── BytecodeFunction (Corvus VM entry)
│   └── StructInstance (Dynamic struct constructor)
├── Array (std::shared_ptr<std::vector<Value>>)
└── Map (std::shared_ptr<std::unordered_map<std::string, Value>>)
```

---

## 13. Concurrency & Threading Model `[IMPLEMENTED]`

### CSP Model (See [ADR 0003](file:///home/ran/Raven/docs/architecture/decisions/0003-csp-concurrency.md))
- **OS Threads**: `spawn(fn)` executes tasks on native POSIX threads.
- **Channels**: Thread-safe synchronized message queues (`NativeChannel`) with condition variables for blocking `.send()` and `.recv()`.
- **Synchronization Primitives**:
  - `create_wait_group()`: Barrier synchronization.
  - `create_mutex()`: Mutex locking for shared resources.
  - `create_once()`: Guaranteed single execution.

---

## 14. Async / Await / Promise / Future `[IMPLEMENTED / PLANNED]`

- **Promise Abstraction (`stdlib/promise.rvn`) `[IMPLEMENTED]`**: Composable `.then()`, `.catch()`, `resolve()`, and `reject()` constructs.
- **Context Management (`stdlib/context.rvn`) `[IMPLEMENTED]`**: Deadline, timeout, and cancellation propagation.
- **Native `async`/`await` Desugaring `[PLANNED for v1.1.0]`**: Syntactic sugar transforming `async fn` into CSP thread tasks returning a promise future.

---

## 15. Error Handling & Diagnostics `[IMPLEMENTED]`

### Typo Suggestions ("Did You Mean?")
When an undefined variable is referenced, `Environment::get()` calculates Levenshtein distances against all declared identifiers in lexical scope and suggests the closest match (distance <= 3):
```
Undefined variable 'prnt'. Did you mean 'print'?
```

### Diagnostic Output
- Precise source line and column numbers.
- Uncaught exceptions propagate with complete multi-frame call stacks.

---

## 16. Module & Package System `[IMPLEMENTED]`

### Package Tooling
- `raven init [name]`: Scaffolds new projects with `raven.json`.
- `raven pkg install <git-url>[@tag]`: Clones packages into `raven_modules/` and locks versions in `raven.lock`.
- `raven pkg list`: Displays installed package versions.

---

## 17. Standard Library Architecture `[IMPLEMENTED]`

Structured in two layers:
1. **Native Builtins Core**: 50+ high-performance C++ primitives (`sin`, `cos`, `regex_match`, `base64_encode`, `time`, `sleep`, `chan_*`, `slice`).
2. **Raven Standard Library Modules**:
   - `stdlib/sync`, `stdlib/event`, `stdlib/cache`, `stdlib/validate`, `stdlib/cli`, `stdlib/promise`
   - `stdlib/math`, `stdlib/strings`, `stdlib/slices`, `stdlib/maps`, `stdlib/encoding/*`, `stdlib/net/http`, `stdlib/crypto`, `stdlib/log`, `stdlib/testing`

---

## 18. Bytecode & Binary Format `[IMPLEMENTED]`

### File Structure (`.rvc`)
| Field | Type | Description |
|---|---|---|
| Magic Bytes | `uint32_t` | `0x52564E43` ("RVNC") |
| Version | `uint32_t` | Format version (1) |
| Constant Pool Size | `uint32_t` | Count of constants |
| Constant Entries | Dynamic | Tagged literal values |
| Bytecode Size | `uint32_t` | Byte length of code |
| Bytecode Instructions | `uint8_t[]` | Stream of opcodes and operands |
| Line Table Size | `uint32_t` | Count of debug line entries |
| Line Entries | `uint32_t[]` | Source line map |

---

## 19. Foreign Function Interface (FFI) `[PLANNED for v1.3.0]`
- Direct C ABI dynamic library loading (`dlopen`/`dlsym`).
- Native type marshalling for integer, float, string, and raw pointer types.

---

## 20. Security & Sandboxing `[PLANNED]`
- Configurable resource limits: execution timeout, stack depth, and maximum memory allocation.
- Sandboxed filesystem and network access permissions for untrusted scripts.

---

## 21. Developer Toolchain `[IMPLEMENTED]`
- `raven run <file>`: Direct AST script execution.
- `raven fmt [-w] <file>`: Canonical source code formatting.
- `raven vet <file>`: Static analysis and linting.
- `raven doc <file>`: Automated Markdown documentation generation.
- `raven bench <file>`: Microsecond execution benchmark runner.
- `raven lsp`: Language Server Protocol server.

---

## 22. LSP & Editor Integration `[IMPLEMENTED]`
- **Protocol**: Standard JSON-RPC over stdin/stdout.
- **Features**: Diagnostic reporting, keyword and builtin autocompletion, hover documentation, and document symbol extraction.
- **VS Code Extension**: Full syntax grammar, snippets, formatting provider, and automated LSP client at `~/.vscode/extensions/raven-lang/`.

---

## 23. Testing & Quality Assurance `[IMPLEMENTED]`

### Test Suite
- **77 Automated Tests** covering all language features (100% pass rate).
- **Differential Testing**: Comparing results of Tree-Walk Interpreter against Corvus VM.
- Continuous verification script: `./all.bash`.

---

## 24. Performance & Benchmarking `[IMPLEMENTED]`
- Microsecond timing built into `raven bench`.
- Dual-engine benchmarking comparing AST evaluation vs. VM bytecode execution.

---

## 25. Build System & Distribution `[IMPLEMENTED]`
- **CMake Build**: Multi-target CMake build generating `raven`, `ravenc`, `rvm`, and test binaries.
- **Go-Style Root Scripts**: `make.bash`, `all.bash`, `run.bash`, `clean.bash`.
- **Distribution Script (`dist.bash`)**: Compiles, runs tests, packages self-contained `raven-v1.0.0-linux-x86_64.tar.gz` and calculates SHA-256 checksums.

---

## 26. Cross-Platform Support `[IMPLEMENTED / PARTIAL]`
- **Linux x86_64 / ARM64**: Full native support `[IMPLEMENTED]`.
- **macOS (Apple Silicon & Intel)**: POSIX-compliant build `[IMPLEMENTED]`.
- **Windows (MSVC / MinGW)**: Standard C++20 abstractions `[PARTIAL]`.

---

## 27. Compatibility & Versioning `[IMPLEMENTED]`
- **Semantic Versioning 2.0.0**: Strict SemVer guarantees for v1.0.0.
- **Stability Guarantee**: Syntax and standard library APIs documented in `docs/language-spec.md` remain backward-compatible across all v1.x releases.

---

## 28. Debugging & Profiling `[IMPLEMENTED]`
- Multi-frame stack traces on exceptions `[IMPLEMENTED]`.
- VM disassembly dump via `raven --disasm <file>` `[IMPLEMENTED]`.
- Interactive breakpoint debugger via `raven debug <file>` (`rvndb`) with stepping, stack inspection, breakpoints, and VS Code F5 integration `[IMPLEMENTED]`.

---

## 29. Documentation Architecture `[IMPLEMENTED]`
- `README.md`: Ecosystem overview and quickstart with official logo.
- `docs/language-spec.md`: Complete v1.0.0 syntax and standard library specification.
- `docs/architecture.md`: Comprehensive 30-section engineering specification.
- `docs/architecture/decisions/`: Architecture Decision Records (ADRs 0001–0006).
- `CHANGELOG.md`: Chronological release history.

---

## 30. Future Architecture / Roadmap `[PLANNED]`

```
v1.0.0 (Current)        v1.1.0                  v1.2.0                  v1.3.0
Dual-Engine Core    ──> Async/Await Syntax  ──> Raven IR (RIR)      ──> Native C FFI
Native CSP & 77 Tests   Cycle-Detecting GC      Register-Based VM       AOT LLVM Backend
Full Toolchain Suite    Extended LSP Actions    WASM Compilation        Package Registry
```
