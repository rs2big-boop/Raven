# Raven Development Roadmap

This document outlines the development trajectory of the Raven programming language. We prioritize pragmatic stability, runtime predictability, and developer experience over speculative complexity.

---

## Roadmap Milestones Overview

| Milestone | Focus | Target Status |
|---|---|---|
| **v1.0.0** | Stable Core, Dual-Engine, CSP Concurrency, Single-Binary Toolchain | **RELEASED `[COMPLETED]`** |
| **v1.1.0** | Concurrency Hardening, Debugger Integration, Diagnostics, Cycle Collector | **RELEASED `[COMPLETED]`** |
| **v1.2.0** | Raven IR (RIR), AST/Bytecode Optimizers, Package Ecosystem | **COMPLETED `[ACTIVE]`** |
| **v1.3.0** | C ABI FFI, WebAssembly Target, Native AOT Tooling | **PLANNED** |

---

## Milestone Details

### v1.0.0 — Production Core & Toolchain `[COMPLETED]`
- [x] **Dual-Engine Execution**: AST Tree-Walk Interpreter (`raven run`) and Corvus Bytecode VM (`ravenc` + `rvm`).
- [x] **Native CSP Concurrency**: `spawn()` executing on POSIX threads, thread-safe synchronized channels, condition-variable `WaitGroup`, and reentrant `Mutex`.
- [x] **Modern Ergonomics**: Default parameters, null coalescing (`??`), optional chaining (`?.`), runtime `is` type checking, spread operator (`...`), `loop {}`, multi-line strings (`"""..."""`), and pattern matching (`match`).
- [x] **Empathetic Error Reporting**: Levenshtein distance typo suggestions ("Did you mean?"), and multi-frame call stack traces on exceptions.
- [x] **Single-Binary Toolchain**: `fmt` (gofmt-style formatter), `vet` (linter), `doc` (markdown doc generator), `bench` (microsecond benchmark runner), and `lsp` (Language Server Protocol).
- [x] **Safety & Parity**:
  - `BytecodeVerifier` checking opcode validity, jump bounds, and constant pool indices before execution.
  - Differential parity tests verifying that the Interpreter and VM yield identical output.
  - 153/153 unit and integration tests passing at 100%.
- [x] **Reference Architecture**: Production-pattern reference projects (`examples/projects/`).

---

### v1.1.0 — Concurrency Hardening, Debugger & Cycle Collection `[COMPLETED]`
- [x] **Structured Concurrency Enhancements**:
  - Channel receive timeouts (`chan_recv_timeout(ch, ms)` and `ch.recv_timeout(ms)`).
- [x] **Debugger Support**:
  - Interactive CLI debugger (`rvndb`) in `raven debug <file>` with breakpoints, step over (`next`), step into (`step`), stack backtraces, and local variable inspection.
  - VS Code extension integration with `raven.debug` and `F5` keybinding.
- [x] **Differential VM Parity Expansion**:
  - Corvus VM bytecode compilation and execution for `??` (Null Coalescing via `OpCode::JumpIfNotNil`) and `is` (Runtime Type Checking via `OpCode::CheckType`).
  - Automated differential parity testing between Tree-Walk Interpreter and Corvus VM across arithmetic, arrays, loops, coalescing, and type inspection.
  - 153/153 tests passing at 100%.
- [x] **Cycle Collection (Bacon-Rajan Cycle Detector)**:
  - Tracing cycle detector pass (`include/raven/gc.hpp`) complementing the reference counter to break circular object references.
  - Built-in `gc_collect()` and `gc_stats()` runtime inspection functions.
  - Comprehensive cycle collection unit tests (`tests/memory/memory_tests.cpp`).
- [x] **Extended LSP Code Actions**:
  - `textDocument/codeAction` implementation with "Quick Fix" suggestions directly powered by the Levenshtein distance engine for unrecognized identifiers and syntax diagnostics.
  - Advertised `"codeActionProvider": true` in server initialize capabilities.

---

### v1.2.0 — Intermediate Representation & Ecosystem `[COMPLETED]`
- [x] **Raven IR (RIR)**:
  - Three-address SSA Intermediate Representation (`include/raven/ir/`, `src/ir/`).
  - Optimization passes: Constant Folding, Dead Code Elimination (DCE), and Common Subexpression Elimination (CSE).
- [x] **AST & Bytecode Optimizer**:
  - Constant expression folding, dead-branch pruning, and redundant grouping unwrap (`src/compiler/optimizer.cpp`).
- [x] **Package Registry & Ecosystem Engine**:
  - Integrated package manager (`raven pkg search`, `raven pkg add`, `raven pkg info`, `raven pkg publish`).
  - Deterministic lockfile resolution (`raven.lock`).
  - Interactive Packages Hub on the official website.

---

### v1.3.0 — Interoperability & Portability `[PLANNED]`
- [ ] **C ABI Foreign Function Interface (FFI)**:
  - Dynamic loading of native shared libraries (`.so`, `.dylib`, `.dll`) via `dlopen`/`dlsym`.
  - Type marshalling between Raven Values and native C structs.
- [ ] **WebAssembly (WASM)**:
  - Compiler target outputting `.wasm` for execution in browser and edge runtimes.
- [ ] **Native AOT Tooling**:
  - Direct native code emission extensions for platforms beyond x86_64 ELF.

---

## Contributing to the Roadmap
Proposals for new features or shifts in milestone priorities should be submitted as an Issue on GitHub: https://github.com/rs2big-boop/Raven
