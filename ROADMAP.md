# Raven Development Roadmap

This document outlines the development trajectory of the Raven programming language. We prioritize pragmatic stability, runtime predictability, and developer experience over speculative complexity.

---

## Roadmap Milestones Overview

| Milestone | Focus | Target Status |
|---|---|---|
| **v1.0.0** | Stable Core, Dual-Engine, CSP Concurrency, Single-Binary Toolchain | **RELEASED `[COMPLETED]`** |
| **v1.1.0** | Concurrency Hardening, Debugger Integration, Diagnostics | **IN PROGRESS `[ACTIVE]`** |
| **v1.2.0** | Raven IR, Register-Based VM, Advanced Optimizations | **PLANNED** |
| **v1.3.0** | C ABI FFI, WebAssembly Target, Package Registry Client | **PLANNED** |

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
  - 74/74 unit and integration tests passing at 100%.
- [x] **Reference Architecture**: Four production-pattern reference projects (`examples/projects/`).

---

### v1.1.0 — Concurrency Hardening & Debugger `[IN PROGRESS]`
- [x] **Structured Concurrency Enhancements**:
  - Channel receive timeouts (`chan_recv_timeout(ch, ms)` and `ch.recv_timeout(ms)`).
- [x] **Debugger Support**:
  - Interactive CLI debugger (`rvndb`) in `raven debug <file>` with breakpoints, step over (`next`), step into (`step`), stack backtraces, and local variable inspection.
  - VS Code extension integration with `raven.debug` and `F5` keybinding.
- [x] **Differential VM Parity Expansion**:
  - Corvus VM bytecode compilation and execution for `??` (Null Coalescing via `OpCode::JumpIfNotNil`) and `is` (Runtime Type Checking via `OpCode::CheckType`).
  - Automated differential parity testing between Tree-Walk Interpreter and Corvus VM across arithmetic, arrays, loops, coalescing, and type inspection.
  - 78/78 tests passing at 100%.
- [ ] **Cycle Collection**:
  - Tracing cycle detector pass for cyclic object references complementing the reference counter.
- [ ] **Extended LSP Code Actions**:
  - "Quick Fix" suggestions directly using the Levenshtein suggestion engine.

---

### v1.2.0 — Intermediate Representation & VM Optimization `[PLANNED]`
- [ ] **Raven IR (RIR)**:
  - Three-address code SSA Intermediate Representation.
  - Common optimization passes: Constant Propagation, Dead Code Elimination (DCE).
- [ ] **Register-Based Corvus VM**:
  - Evaluation of 3-operand register VM architecture to reduce stack push/pop overhead.
- [ ] **Peephole Bytecode Optimizer**:
  - Redundant instruction elimination and jump threading.

---

### v1.3.0 — Interoperability & Portability `[PLANNED]`
- [ ] **C ABI Foreign Function Interface (FFI)**:
  - Dynamic loading of native shared libraries (`.so`, `.dylib`, `.dll`) via `dlopen`/`dlsym`.
  - Type marshalling between Raven Values and native C structs.
- [ ] **WebAssembly (WASM)**:
  - Compiler target outputting `.wasm` for execution in browser and edge runtimes.
- [ ] **Package Registry**:
  - Centralized package manifest publishing and dependency resolution.

---

## Contributing to the Roadmap
Proposals for new features or shifts in milestone priorities should be submitted as an Issue using the [Feature Request template](.github/ISSUE_TEMPLATE/feature_request.md).
