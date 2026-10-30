# The Raven Vision: Building in the Open

> **A programming language built by developers, for developers.**  
> *"Simple enough to script in minutes. Powerful enough to run distributed systems."*

---

## 1. Why Does Raven Exist?

Over the past three decades, software development has made extraordinary progress, but developers still frequently encounter a compromise between developer velocity and systems architecture:

- **Scripting & Dynamic Languages**: Languages like Python, Ruby, and JavaScript provide rapid prototyping, dynamic ergonomics, and expressive syntax. However, their concurrency models often complicate full multi-core CPU utilization, thread-level task orchestration, and straightforward concurrent system design.
- **Systems & Compiled Languages**: Languages like Rust, C++, and Go offer high performance and predictable concurrency, but often demand additional ceremony: extensive boilerplate, strict compile-time constraints, or complex build graphs that can slow down iterative scripting and rapid automation.

**Raven exists to bridge this divide.**

Raven delivers the **ergonomics and expressive velocity of modern scripting**, combined with the **explicit concurrency, single-binary tooling, and runtime predictability of modern systems engineering**.

---

## 2. What Problems Does Raven Solve?

### Problem 1: Concurrency Should Be Explicit and Direct
Many dynamic languages rely on complex asynchronous event loops (`async`/`await`), which can lead to function coloring and subtle scheduling edge-cases.
* **The Raven Solution**: **Native CSP Concurrency with Explicit Communication Primitives**. Raven provides native `spawn()`, thread-safe `channels`, `WaitGroups`, and `Mutexes` backed directly by operating system threads and futex primitives. Communication between tasks is explicit, structured, and predictable.

### Problem 2: Tooling Fragmentation
In many ecosystems, setting up a project requires separate installations for a formatter, linter, test runner, documentation generator, language server, and package manager.
* **The Raven Solution**: **A Single Unified Toolchain**. The single `raven` executable provides:
  - `raven run`: Instant script execution
  - `raven fmt`: Canonical code formatter
  - `raven vet`: Static analysis & linting
  - `raven doc`: Automated API documentation generation
  - `raven bench`: Microsecond benchmark runner
  - `raven lsp`: Native Language Server Protocol
  - `raven pkg`: Package manager with deterministic lockfiles

### Problem 3: The Gap Between Prototyping and Compiled Deployment
* **The Raven Solution**: **Dual-Engine Architecture**.
  - **Tree-Walk Interpreter (`raven run`)**: Zero compilation overhead, full AST retention, and immediate execution for scripting, REPL, and IDE analysis.
  - **Corvus Virtual Machine (`ravenc` & `rvm`)**: Stack-based bytecode compiler and high-speed execution VM with binary serialization (`.rvc`) for deployment.

---

## 3. What Makes Raven Unique?

Every feature in Raven is engineered with a focus on developer experience and explicit semantics:

### Language Features
- **Default Function Parameters** `[IMPLEMENTED]`: `fn greet(name = "World") { ... }` in functions and closures.
- **Null Coalescing Operator (`??`)** `[IMPLEMENTED]`: Safe fallback evaluation (`let port = env("PORT") ?? 8080`).
- **Optional Chaining (`?.`)** `[IMPLEMENTED]`: Safe navigation on nil values (`user?.address?.city`, `items?.[0]`).
- **`is` Type Introspection Operator** `[IMPLEMENTED]`: Clean runtime type checking (`x is Number`, `s is String`).
- **Functional Pipeline Operator (`|>`)** `[IMPLEMENTED]`: Left-to-right functional composition (`data |> filter_active |> transform`).
- **Pattern Matching (`match`)** `[IMPLEMENTED]`: Rust-inspired pattern matching with wildcard `_` fallback.
- **Spread Operator (`...`)** `[IMPLEMENTED]`: Inline array and map merging (`[...prefix, 4, 5]`, `{...base, "k": v}`).
- **Multi-line Triple-Quoted Strings** `[IMPLEMENTED]`: Preserved newlines and indentation (`""" ... """`).
- **`loop {}` Infinite Loop** `[IMPLEMENTED]`: Clean loop construct with `break` and `continue`.

### Empathetic Diagnostics & Developer Experience
- **Levenshtein "Did you mean?" suggestions** `[IMPLEMENTED]`: Typo suggestions for undefined variables.
- **Full Call Stack Traces** `[IMPLEMENTED]`: Function names, line numbers, and column coordinates on exceptions.
- **Interactive REPL** `[IMPLEMENTED]`: Balanced multi-line block detection and expression result previews.

### Standard Library
- Comprehensive modules for Networking, HTTP, JSON, Crypto, Testing, Logging, Events, Caches, and Collections `[IMPLEMENTED]`.

---

## 4. Who is Raven For?

- **Backend & Cloud Engineers**: Building microservices, HTTP REST APIs, worker pools, and asynchronous data pipelines.
- **DevOps & CLI Authors**: Writing fast, dependable command-line utilities with self-contained dependencies and fast startup.
- **Systems & Infrastructure Developers**: Requiring explicit multi-threaded task coordination without complex runtime layers.
- **Learners & Language Explorers**: Looking to inspect and contribute to a modern, cleanly structured C++20 compiler and VM.

---

## 5. Design Philosophy

Raven's technical decisions are guided by five core principles:

1. **Developer Experience First**: Syntax, error messages, and developer tooling should make writing and maintaining code a delightful experience.
2. **Explicit Concurrency**: Task communication should be explicit, transparent, and structured through channels and synchronization barriers, avoiding hidden magic.
3. **Predictable Runtime**: The Tree-Walk Interpreter and the Corvus VM must exhibit identical semantic behavior, results, and error handling.
4. **Batteries Included**: Developers shouldn't need a dozen configuration files and third-party tools just to format, test, or lint their code.
5. **Pragmatic Engineering**: We build small, consistent, and rock-solid components before adding complexity. Stability and usability always precede speculative features.

---

## 6. Development Roadmap

### Phase 1: Foundation (v1.0.0 — CURRENT RELEASE) `[COMPLETED]`
- [x] Dual-engine architecture (Tree-Walk Interpreter + Corvus VM).
- [x] Native CSP concurrency (POSIX threads, channels, WaitGroups, Mutexes).
- [x] Unified developer toolchain (`run`, `fmt`, `vet`, `doc`, `bench`, `lsp`, `pkg`).
- [x] Comprehensive test suite (74/74 tests, 100% pass rate).
- [x] Differential Parity testing between Interpreter and VM.
- [x] Bytecode integrity verifier (`BytecodeVerifier`).
- [x] Four production-pattern reference projects (REST API, CLI tool, DB client, Worker pool).
- [x] Open-source foundation: CI workflow, issue templates, Contributing guide, Code of Conduct, and Security policy.

### Phase 2: Runtime & Concurrency Hardening (v1.1.0) `[IN PROGRESS]`
- [ ] Structured Task Cancellation and Timeouts across channels.
- [ ] Interactive step-through debugger (DAP - Debug Adapter Protocol integration for VS Code).
- [ ] Tracing cycle-collector pass to complement reference counting.
- [ ] Expanded differential conformance test suite.

### Phase 3: Compiler Intermediate Representation (v1.2.0) `[PLANNED]`
- [ ] **Raven IR (RIR)**: Three-address code SSA representation.
- [ ] Compiler optimization passes: Dead Code Elimination (DCE), Common Subexpression Elimination (CSE).
- [ ] Register-based Corvus VM engine for accelerated bytecode execution.

### Phase 4: Native Interoperability & Portability (v1.3.0) `[PLANNED]`
- [ ] C ABI Foreign Function Interface (FFI) for dynamic library binding.
- [ ] WebAssembly (WASM) compiler target for edge and browser runtimes.
- [ ] Centralized package registry client.

---

## 7. Built in the Open

Raven is developed in the open. Its source code, architecture specifications, design decisions, roadmap, and development process are publicly accessible. Contributions from developers around the world are warmly welcome.

Join the project on GitHub: [https://github.com/raven-lang/raven](https://github.com/raven-lang/raven)  
*Let's build the future of pragmatic programming together.*
