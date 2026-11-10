# Raven Changelog

All notable changes to the Raven programming language are documented in this file.

## [1.4.0] — 2026-11-10

### Added
- **Formal ADR Specifications**:
  - `ADR 0008.1`: Ownership, Root Tracking & Cleanup Semantics.
  - `ADR 0008.2`: Resource Leases, Arc/GC Synchronization & Safepoint Lifecycle.
- **Deterministic RAII `using` Statement & Cleanup Engine**:
  - Syntax: `using res = expr { body }`.
  - Guaranteed LIFO cleanup unwinding on normal exit, early `return`, `break`, `continue`, and exceptions.
  - Multi-cleanup error chaining with Primary and Suppressed exception hierarchy.
  - Ownership slot transfer: `move` inside `using` transfers ownership and suppresses RAII close.
- **Move-Only Ownership & Compiler Enforcement**:
  - Prefix operator `move x` transfers ownership and invalidates the variable handle.
  - Static compile-time and semantic use-after-move detection via `Resolver`.
  - Double-move rejection and reassignment revival.
  - Bytecode compilation of `move` transferring local/global slots to nil while preserving the returned value.
- **Thread-Safe Resource Handles & Active Leases**:
  - `ResourceControlBlock`: atomic state machine (`Open`, `Closing`, `Closed`) draining in-flight operations before close.
  - `ResourceLease`: RAII operation lease preventing concurrent close while read/write operations execute.
  - `ResourceOwnerHandle`: tracks handle-level invalidation on move.
  - `ResourceReaperQueue`: background daemon draining GC-detected leaks without stalling worker threads.
- **Arc / GC Root Synchronization**:
  - `ArcRootRegistry`: thread-safe atomic registry tracking `Arc<T>` pointers as live roots for GC.
  - Exclusive lock snapshot during STW Initial Mark phase.
  - Decoupled memory reclamation (GC sole reclaimer).
  - Root write barrier shading newly registered roots during `IncrementalMarking`.
- **Corvus VM Precise Stack Maps & Safepoint Lifecycle**:
  - `StackMapTable`: exact operand stack and local slot pointer bitmaps generated per chunk.
  - Safepoint polling at loop back-edges (`OpCode::Loop`) and function call sites (`OpCode::Call`).
  - `SafepointManager`: native thread parking (`enter_native` / `exit_native`) allowing GC to proceed without blocking when threads sleep or perform blocking I/O.
- **Generational GC Card Table & Incremental Marking Barrier**:
  - `CardTable`: 512-byte page granularity tracking Old -> Young cross-generational references.
  - Generational write barrier dirtying cards on Old-to-Young field stores.
  - Incremental update barrier preserving strong tri-color invariant (shades white targets to gray when pointed to by black objects).
- **Test Suite Expanded to 210 Tests**: 210 automated unit, memory, concurrency, semantic, differential, and benchmark tests passing at 100%.
- **Reliability Verified under Sanitizers**:
  - Full AddressSanitizer (ASan) & UndefinedBehaviorSanitizer (UBSan) verification: zero leaks, buffer overflows, or use-after-free.
  - Full ThreadSanitizer (TSan) verification: zero data races across multi-threaded Arc mutations and GC coordinator cycles.
- **Formal Benchmarks**:
  - GC Stop-The-World (STW) Pause: 29 µs (1K roots), 301 µs (10K roots), 1.55 ms (50K roots).
  - Arc Throughput: up to 1.3M ops/sec.
  - ResourceLease Latency: 196 ns p50.
  - Card Table Throughput: 84.9M marks/sec, 3.9 ms scan for 524,288 cards (256 MB Heap).

## [1.3.0] — 2026-11-04

### Added
- **Native `async` and `await` syntax**:
  - `async fn name(args) { ... }` natively defines an asynchronous function executing concurrently in background worker threads.
  - `await expr` prefix operator resolves the background channel promise and yields the result seamlessly, or returns existing synchronous values as-is.
- **Foreign Function Interface (FFI) stdlib wrapper (`stdlib/ffi.rvn`)**:
  - `open_lib()`, `ffi_library()`, `ffi_load()`, with type constants (`FFI_VOID`, `FFI_INT`, `FFI_DOUBLE`, `FFI_STRING`, `FFI_PTR`).
  - Safe error handling and dynamic shared library access.
- **Top-level Web Framework import (`stdlib/web.rvn`)**:
  - Exposes `web_app()` and full web framework suite (`stdlib/web/web`, `stdlib/web/app`, `stdlib/web/html`, `stdlib/web/template`).
- **Python-compatible Socket Library (`stdlib/socket.rvn`)**:
  - TCP client and server abstractions modeled directly after Python's `socket` library.
- **Extended type conversion builtins**:
  - `float()`, `bool(str)`, `array()`/`list()`, `dict()`, and prompt support in `input(prompt)`.
- **Standalone Native Binary Flag (`raven build --standalone`)**:
  - Convenient alias for `raven build --native` to produce single-file standalone native executables.
- **Test suite expansion**: 174 automated unit, semantic, and differential tests passing (100%).

## [1.0.1] — 2026-11-04

### Fixed
- **Windows portability fix**: removed Linux-only executable and stdlib path assumptions that caused broken behavior on Windows installs and runtime resolution.
- **Release packaging cleanup**: hardened the release workflow and install scripts so they resolve binaries and standard libraries from the actual execution environment rather than hardcoded machine paths.
- **Cross-platform CLI stability**: replaced shell-only directory creation and Linux-specific path probing with portable `std::filesystem` logic across the CLI toolchain.

## [1.0.0] — 2026-11-02

### Added
- **Standalone Native ELF Binary Generator (`raven build`)**:
  - `raven build script.rvn -o binary` and `ravenc --native` compile Raven scripts into self-contained ELF 64-bit executables using Raven's own Corvus engine with **zero external compiler dependencies** (no GCC, Clang, or C build tools required).
  - Executable embeds serialized bytecode and auto-executes upon invocation via direct ELF loading.
- **VS Code & Cursor IDE Extension v1.0.0**:
  - Integrated step-through debugger (`rvndb`) with editor gutter breakpoints and `F5` launch support.
  - Added Run (`play`) and Debug (`debug-alt`) buttons directly in editor title bar.
  - Packaged standalone `.vsix` with installer script (`scripts/install_vscode_ext.sh`).
- **Native Web Framework (`stdlib/web/`)**:
  - Express/Sinatra-style router (`stdlib/web/app.rvn`), safe HTML element DSL (`stdlib/web/html.rvn`), and template engine (`stdlib/web/template.rvn`).
- **Official Web Portal in React 19 + TypeScript (`site/`)**:
  - Built with React 19, TypeScript 5.8, Vite 6, and Tailwind CSS v4.
  - Full interactive Playground, standard library explorer, interactive documentation, and architecture diagrams.
  - Production server in Raven ([examples/serve_website.rvn](examples/serve_website.rvn)) serves the compiled frontend.
- **Full Test Suite (147/147 tests passing at 100%)**:
  - Lexer, Parser, AST, Tree-Walk Interpreter, Corvus Bytecode VM, Differential Testing, Native Concurrency, Memory Subsystem, and IR Passes.
- **Corvus VM & Compiler Full Parity**:
  - `MapExpr` & `OpCode::BuildMap` supporting full map literals in Bytecode.
  - `PropertyExpr` & `PropertyAssignExpr` (`obj.prop`, `obj.prop = val`) in Corvus VM.
  - `MatchExpr` pattern matching compiled directly to Bytecode branches.
  - `ForInStmt` and `OpCode::Length` compiling `for (let x in arr)` loops to linear VM instructions.
  - `BreakStmt` and `ContinueStmt` loop jump backpatching with lexical slot cleanup.
  - `LambdaExpr` anonymous first-class functions in Bytecode chunks.
- **Modular Subsystem Refactoring**:
  - Reorganized project architecture with dedicated `src/interpreter/`, `src/vm/`, and `src/runtime/memory/`.
  - Implemented `MemoryManager` (`raven/runtime/memory/memory_manager.hpp`) unifying reference counting and cycle collection.
  - Mirrored headers in modular `include/raven/{frontend, compiler, interpreter, vm, runtime, tools}/` directories.
- **Modular Concurrency Subsystem**:
  - Extracted native CSP concurrency primitives to dedicated `include/raven/concurrency/` and `src/concurrency/`.
  - Created singleton managers: `ChannelManager`, `WaitGroupManager`, `MutexManager`, and `SpawnRegistry`.
- **Test suite expanded to 85/85 tests passing at 100%**.
- **Interactive Step-Through Debugger (`raven debug <file>`)**:
  - Full CLI debugger `rvndb` featuring breakpoints (`break <line>`, `delete <line>`, `info break`), stepping (`step` into, `next` over), continue (`continue`), call stack trace (`bt`), local variable inspection (`locals`), and live expression evaluation (`print <expr>`).
  - Visual source context rendering around current execution line with `->` arrow and breakpoint markers `●`.
  - VS Code extension integration: Added `raven.debug` ("Raven: Debug Current File") bound to `F5`.
- **Channel Receive Timeouts**:
  - Native `chan_recv_timeout(ch_id, timeout_ms)` in C++ runtime using `std::condition_variable::wait_for`.
  - High-level `ch.recv_timeout(timeout_ms)` in `stdlib/sync.rvn` returning `nil` safely on timeout without deadlocking.
- **Corvus VM Dual-Engine Parity Expansion**:
  - Added native bytecode compilation and VM execution for `??` (Null Coalescing via `OpCode::JumpIfNotNil`).
  - Added native bytecode compilation and VM execution for `is` (Runtime Type Checking via `OpCode::CheckType`).
  - Automated differential parity testing between Tree-Walk Interpreter and Corvus VM across arithmetic, arrays, loops, coalescing, and type inspection.
  - Cycle Collector prototype (`gc_collect`, `gc_stats`, `GCRegistry`) under ADR-0007.
  - Comprehensive documentation tree (`docs/getting-started/`, `docs/language/`, `docs/architecture/`, `docs/stdlib/`).
  - Test suite expanded to 78/78 tests passing at 100%.

## [1.0.0] — 2026-10-30

### Added
- **Foreign Function Interface (FFI)**: Native `ffi_open`, `ffi_close`, `ffi_call` (`<dlfcn.h>`) with `stdlib/ffi.rvn` module for zero-overhead calling into C/shared libraries.
- **Corvus VM & Compiler Full Parity**:
  - `MapExpr` & `OpCode::BuildMap` supporting full map literals in Bytecode.
  - `PropertyExpr` & `PropertyAssignExpr` (`obj.prop`, `obj.prop = val`) in Corvus VM.
  - `MatchExpr` pattern matching compiled directly to Bytecode branches.
  - `ForInStmt` and `OpCode::Length` compiling `for (let x in arr)` loops to linear VM instructions.
  - `BreakStmt` and `ContinueStmt` loop jump backpatching with lexical slot cleanup.
  - `LambdaExpr` anonymous first-class functions in Bytecode chunks.
- **Modular Subsystem Refactoring**:
  - Reorganized project architecture with dedicated `src/interpreter/`, `src/vm/`, and `src/runtime/memory/`.
  - Implemented `MemoryManager` (`raven/runtime/memory/memory_manager.hpp`) unifying reference counting and cycle collection.
  - Mirrored headers in modular `include/raven/{frontend, compiler, interpreter, vm, runtime, tools}/` directories.
- **Modular Concurrency Subsystem**:
  - Extracted native CSP concurrency primitives to dedicated `include/raven/concurrency/` and `src/concurrency/`.
  - Created singleton managers: `ChannelManager`, `WaitGroupManager`, `MutexManager`, and `SpawnRegistry`.
- **Test suite expanded to 85/85 tests passing at 100%**.
- **Native CSP Concurrency**: `spawn()` running on native POSIX threads, `chan_create()`, `chan_send()`, `chan_recv()`, `chan_close()`, `chan_len()`, and high-level `stdlib/sync` (`go()`, `create_channel()`, `create_wait_group()`, `create_mutex()`, `create_once()`).
- **Call Stack Traces**: Full multi-frame stack traces on runtime errors with function names, line numbers, and column numbers.
- **Package Manager**: Built-in `raven pkg init`, `raven pkg install <repo>[@tag]`, `raven pkg list`, with cryptographic-ready `raven.lock` lockfile tracking dependencies and versions.
- **Default Function Parameters**: `fn greet(name = "World") { ... }` with default argument resolution for both functions and anonymous closures (`fn(a, b = 10) {}`).
- **Null Coalescing Operator (`??`)**: `x ?? fallback` short-circuiting operator returning the left side if non-nil, otherwise the right side.
- **Optional Chaining (`?.`)**: Safe navigation for maps and properties (`obj?.field`) and indices (`arr?.[0]`) without throwing runtime errors when targeting `nil`.
- **`is` Type Checking Operator**: Clean expressive type introspection: `42 is Number`, `"hello" is String`, `true is Bool`, `[1, 2] is Array`, `{"a": 1} is Map`, `nil is Nil`, custom struct checking.
- **Spread Operator (`...`)**: Modern inline expansion: `[...prefix, 4, 5]` for arrays and `{...base, "override": 1}` for maps.
- **`loop {}` Infinite Loop**: Clean syntactic keyword for infinite loops with `break` and `continue`.
- **Multi-line Triple-Quoted Strings**: `""" ... """` strings with preserved newlines and escape sequences.
- **Intelligent Typo Suggestions ("Did You Mean?")**: Levenshtein distance-based typo detector that automatically suggests closest matching variables/builtins when an undefined identifier is accessed.
- **Enhanced REPL Shell**:
  - Interactive commands: `.help`, `.quit`, `.exit`, `.load <file>`, `.clear`, `.version`.
  - Multi-line block detection for balanced braces `{`, `(`, `[`.
  - Expression evaluation result display (`=> value`).
- **Standard Library Modules**:
  - `stdlib/event`: Full event emitter and pub-sub system (`on`, `emit`, `off`, `listener_count`).
  - `stdlib/cache`: LRU cache with eviction (`create_lru_cache`) and TTL time-to-live cache (`create_ttl_cache`).
  - `stdlib/validate`: Schema and field validator (`validate_schema`, `validate_string`, `validate_number`).
  - `stdlib/promise`: Async Promise / Future pattern with `.then()`, `.catch()`, `resolve()`, `reject()`.
  - `stdlib/cli`: Declarative CLI command and flag builder (`create_cli`).
  - Full Go-inspired stdlib: `archive`, `compress`, `container`, `crypto`, `database`, `encoding`, `hash`, `net`, `path`, `sync`, `syscall`, `uuid`, `xid`, `reflect`.
- **Developer Tooling Suite**:
  - `raven fmt`: Code formatting engine.
  - `raven vet`: Static analysis and linter.
  - `raven doc`: Automated Markdown doc generator.
  - `raven bench`: Benchmark execution harness.
  - `raven lsp`: Language Server Protocol server with diagnostics, completion, hover, and document symbols.
- **VS Code Extension v1.0.0**: Syntax highlighting, snippets, formatting provider, and full LSP client.
- **Release Tarball**: Self-contained `dist/raven-v1.0.0-linux-x86_64.tar.gz` with SHA-256 verification via `./dist.bash`.
- **Official Branding**: Raven Programming Language official logo integrated in README and docs.

### Changed
- Production version bumped from `0.3.0` to `1.0.0`.
- All tests updated and 100% passing (68/68 test suite).
- Concurrency engine now uses native OS threads instead of cooperative simulation.

### Fixed
- Fixed VM call stack tracking and frame offsets.
- Bytecode serializer handles `BytecodeFunction` objects safely.
- Import resolver supports both flat and sub-directory imports (`stdlib/net/http`, `stdlib/encoding/json`, etc.).
