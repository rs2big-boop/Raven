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
* **The Raven Solution**: **Native CSP Concurrency with Explicit Communication Primitives**. Raven provides native `spawn()`, thread-safe `channels`, `WaitGroups`, and `Mutexes` backed directly by operating system synchronization primitives. Communication between tasks is explicit, structured, and predictable.

### Problem 2: Tooling Fragmentation
In many ecosystems, setting up a project requires separate installations for a formatter, linter, test runner, documentation generator, language server, and package manager.
* **The Raven Solution**: **A Single Unified Toolchain**. The single `raven` executable provides:
  - `raven run`: Direct script execution
  - `raven fmt`: Canonical code formatter
  - `raven vet`: Static analysis & linting
  - `raven doc`: Automated API documentation generation
  - `raven bench`: Built-in benchmark runner
  - `raven debug`: Interactive CLI step-through debugger (`rvndb`)
  - `raven lsp`: Native Language Server Protocol
  - `raven pkg`: Package manager with deterministic lockfiles

### Problem 3: The Gap Between Prototyping and Compiled Deployment
* **The Raven Solution**: **Dual-Engine Architecture**.
  - **Tree-Walk Interpreter (`raven run`)**: No separate compilation step, full AST retention, and immediate execution for scripting, REPL, and IDE analysis.
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
3. **Predictable Runtime**: The Tree-Walk Interpreter and the Corvus VM target strict semantic compatibility in evaluation, variable scoping, and runtime behavior.
4. **Batteries Included**: Developers shouldn't need a dozen configuration files and third-party tools just to format, test, or lint their code.
5. **Pragmatic Engineering**: We build small, consistent, and rock-solid components before adding complexity. Stability and usability always precede speculative features.

---

## 6. Development Roadmap

### Phase 1: Foundation (v1.0.0 — CURRENT RELEASE) `[COMPLETED]`
- [x] Dual-engine architecture (Tree-Walk Interpreter + Corvus VM).
- [x] Native CSP concurrency (OS native threads, channels, WaitGroups, Mutexes).
- [x] Unified developer toolchain (`run`, `fmt`, `vet`, `doc`, `bench`, `debug`, `lsp`, `pkg`).
- [x] Comprehensive test suite (77/77 tests passing).
- [x] Differential Parity testing between Interpreter and VM.
- [x] Bytecode integrity verifier (`BytecodeVerifier`).
- [x] Four production-pattern reference projects (REST API, CLI tool, DB client, Worker pool).
- [x] Open-source foundation: CI workflow, issue templates, Contributing guide, Code of Conduct, and Security policy.

### Phase 2: Runtime & Concurrency Hardening (v1.1.0) `[IN PROGRESS]`
- [x] Structured Task Cancellation and Timeouts across channels (`chan_recv_timeout`, `ch.recv_timeout(ms)`).
- [x] Interactive step-through debugger (`rvndb` via `raven debug <file>` + VS Code F5 integration).
- [x] Corvus VM parity expansion for `??` (Null Coalescing) and `is` (Runtime Type Checking).
- [ ] Runtime shutdown and lifecycle management (clean task unwinding on termination).
- [ ] Thread-safe shared value ownership rules and mutation boundary enforcement.
- [ ] Exception propagation and reporting across spawned background tasks.
- [ ] Channel closure and cancellation semantics hardening.
- [ ] Deadlock and race-condition stress-testing test suite.
- [ ] Research & experimental prototype: Tracing cycle-collector pass to complement reference counting.

### Phase 3: Compiler Intermediate Representation (v1.2.0) `[PLANNED]`
- [ ] **Raven IR (RIR)**: Three-address code SSA representation.
- [ ] Compiler optimization passes: Dead Code Elimination (DCE), Common Subexpression Elimination (CSE).
- [ ] Register-based Corvus VM engine for accelerated bytecode execution.

### Phase 4: Native Interoperability & Portability (v1.3.0) `[PLANNED]`
- [ ] C ABI Foreign Function Interface (FFI) for dynamic library binding.
- [ ] WebAssembly (WASM) compiler target for edge and browser runtimes.
- [ ] Centralized package registry client.

---

## 7. Memory Management Strategy

Raven employs a **Hybrid Memory Management Model**:
- **Reference Counting**: Automatic deterministic lifetime tracking for common heap objects (`Array`, `Map`, `Callable`).
- **Tracing Cycle Collector (Research & Prototype)**: Designed to detect and reclaim unreachable reference cycles without stopping the world.
- **RAII Resource Cleanup**: Native C++ destruction guarantees for sockets, file descriptors, and system mutexes.
- **Explicit Cross-Thread Boundaries**: Separate concerns between reference-count synchronization and object mutation safety.

---

## 8. Runtime Guarantees

Raven establishes clear guarantees for code running on its platform today:

- **Dual-Engine Semantic Parity**: Core arithmetic, control flow, functions, closures, collections, coalescing, and type inspection produce equivalent results across the Tree-Walk Interpreter and Corvus VM.
- **Explicit Channel Semantics**: Sending on closed channels throws a documented runtime exception; channel receive with timeout cleanly returns `nil` on expiration without blocking indefinitely.
- **Empathetic Diagnostic Guarantees**: Runtime exceptions provide call stack traces with source line coordinates; misspelled identifiers produce Levenshtein-distance "Did you mean?" suggestions.
- **Bytecode Integrity**: The `BytecodeVerifier` validates opcode ranges, jump bounds, and constant pool indices before any chunk is executed by the Corvus VM.
- **Deterministic Resource Cleanup**: File handles, network sockets, and synchronization locks are released deterministically upon scope exit via native RAII.

---

## 9. Non-Goals

To maintain sharp focus and long-term engineering velocity, Raven explicitly defines what it is **not** trying to be:

- **Not a Drop-in Replacement for C or Rust**: Raven optimizes for developer velocity, expressive ergonomics, and clean multi-threaded design, rather than manual pointer manipulation or zero-cost hardware abstraction.
- **Not a Language that Hides Concurrency**: Concurrency in Raven is explicit. We do not hide threading behind implicit magic or unbounded single-threaded event queues that obscure bottleneck analysis.
- **Not a Kitchen-Sink Language**: Features are added only when they solve concrete engineering problems. We avoid speculative syntax additions solely for feature-list parity with other languages.
- **Not an Overnight Re-write of the Ecosystem**: Raven is engineered pragmatically, preferring small, robust, battle-tested components over premature massive rewrites.

---

## 10. Built in the Open

Raven is developed in the open. Its source code, architecture specifications, design decisions, roadmap, and development process are publicly accessible. Contributions from developers around the world are warmly welcome.

Join the project on GitHub: [https://github.com/raven-lang/raven](https://github.com/raven-lang/raven)  
*Let's build the future of pragmatic programming together.*
