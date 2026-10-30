# Raven Changelog

All notable changes to the Raven programming language are documented in this file.

## [1.1.0-dev] — 2026-10-30

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
