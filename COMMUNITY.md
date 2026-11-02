# Welcome to the Raven Community 🦅

> *"Simple enough to script in minutes. Powerful enough to run distributed systems."*

Raven is an open-source programming language built in the open, by developers, for developers. Whether you are interested in compiler design, virtual machine internals, standard library design, IDE tooling, or web applications, you are warmly invited to become a part of the Raven community!

---

## 🌟 The Raven Story & Vision

Raven was created to solve a recurring tension in modern software development:
- **Scripting languages** (like Python and JavaScript) are fast to write, but often make true multi-core concurrency and standalone single-binary deployments complex.
- **Systems languages** (like Rust, C++, and Go) provide raw speed and explicit concurrency, but often carry high compile times and ceremony when you just want to write a quick script or CLI tool.

**Raven bridges this gap**:
1. **Dual-Engine Architecture**: Start instantaneously with the AST tree-walk interpreter, or compile to high-speed bytecode executed by the **Corvus Virtual Machine**.
2. **Native Standalone Executables**: Compile directly into a standalone ELF 64-bit binary (`raven build app.rvn -o app`) with **zero external dependencies** (no GCC, Clang, or C build tools required).
3. **Native CSP Concurrency**: Multi-threaded `spawn()`, thread-safe channels (`chan_create`, `chan_send`, `chan_recv`), wait groups, and mutexes.
4. **Batteries-Included Standard Library**: More than 25 Go-inspired modules (`net/http`, `encoding/json`, `crypto`, `bufio`, `slices`, `sync`, `web`).
5. **Modern IDE Tooling**: Full VS Code and Cursor extension with an interactive step-through debugger (`rvndb`), gutter breakpoints, and F5 launch support.

Read our full manifesto: [**RAVEN_VISION.md**](RAVEN_VISION.md).

---

## 🗺️ Codebase Tour for Contributors

If you want to dive into the codebase, here is where each subsystem lives:

```text
Raven/
├── src/
│   ├── cmd/                 # Executable entry points (raven, ravenc, rvm, rvndb)
│   ├── frontend/            # Lexer, Parser, AST nodes, and Diagnostic reports
│   ├── semantic/            # Scope resolver, symbol table, and type checker
│   ├── compiler/            # AST-to-Bytecode compiler, optimizer, and serializer
│   ├── ir/                  # Intermediate Representation and optimization passes
│   ├── vm/                  # Corvus Virtual Machine (stack, frames, loader, opcodes)
│   ├── interpreter/         # Tree-walk interpreter and environment scoping
│   ├── runtime/             # Core values, built-in functions, and memory subsystem
│   ├── concurrency/         # Native OS threads, CSP channels, mutexes, wait groups
│   ├── packages/            # Package manager and lockfile (raven.lock) resolver
│   └── tools/               # Language Server (LSP), linter, formatter, debugger
├── include/raven/           # Modular C++20 header architecture
├── stdlib/                  # Standard Library packages written in Raven
│   ├── web/                 # Native HTML DSL, string templates, and HTTP routing
│   ├── encoding/            # JSON, Base64, CSV, and Hex encoders
│   ├── net/                 # HTTP client, URL parser
│   ├── math/                # Math, statistics, PRNG random
│   └── sync.rvn, ...        # Concurrency primitives
├── tests/                   # 147 automated tests (GoogleTest & Differential tests)
├── editors/vscode/          # VS Code & Cursor extension with debugger & LSP
├── site/                    # Official website in React 19 + TypeScript + Tailwind CSS
└── examples/                # Real-world reference scripts, servers, and pipelines
```

---

## 🤝 How to Get Involved

There are many ways to contribute to Raven, whether you have 30 minutes or 30 days:

### 1. Great "First Issues"
- **Standard Library Modules**: Add helper functions to `stdlib/strings.rvn`, `stdlib/slices.rvn`, `stdlib/math.rvn`, or `stdlib/path.rvn`.
- **Sample Programs & Tutorials**: Build an interesting CLI tool, algorithm, or web service in `examples/`.
- **Documentation & Guides**: Improve function docstrings, write guides, or translate documentation.
- **Benchmarking**: Write comparative benchmarks comparing Raven tree-walk, Corvus VM, and other languages.

### 2. Intermediate & Core Contributions
- **New Compiler Optimizations**: Add an IR pass in `src/ir/passes/` (e.g. dead branch elimination, loop invariant code motion).
- **Virtual Machine Instructions**: Implement new opcodes in `src/vm/` and `src/compiler/` to speed up common patterns.
- **LSP Features**: Add semantic highlighting, rename symbol, or find references in `src/tools/lsp.cpp`.
- **VS Code Extension Improvements**: Expand code snippets, add problem matchers, or enrich debug variable inspection.

### 3. Advanced Roadmap Tracks
- **JIT Compiler**: Help prototype an x86-64 Just-In-Time compiler for the Corvus VM.
- **WebAssembly Target**: Help compile Corvus to Wasm so Raven can run client-side in the browser.
- **Cross-Platform Binary Packaging**: Extend standalone binary generation to Windows PE and macOS Mach-O.

---

## 🛠️ Local Development Quickstart

```bash
# 1. Clone the repository
git clone https://github.com/your-username/Raven.git
cd Raven

# 2. Build the toolchain (incremental build in ~2 seconds)
./make.bash

# 3. Run the full test suite (147/147 tests + stdlib tour)
./all.bash

# 4. Test standalone native binary compilation
./build/raven build examples/fibonacci.rvn -o fib_test && ./fib_test

# 5. Install the VS Code extension
./scripts/install_vscode_ext.sh
```

---

## 💬 Community Channels & Discussions

- **GitHub Discussions**: For asking questions, proposing ideas, and sharing your Raven projects.
- **GitHub Issues**: For bug reports and structured feature proposals (using our issue templates).
- **Pull Requests**: Every PR receives thoughtful code review. Read our [**CONTRIBUTING.md**](CONTRIBUTING.md) for guidelines.
- **Code of Conduct**: We are committed to providing a welcoming, inclusive, and harassment-free experience for everyone. Please review our [**CODE_OF_CONDUCT.md**](CODE_OF_CONDUCT.md).

---

## 🏆 Hall of Contributors

Raven is proud to be built by developers worldwide. Every contribution—whether a typo fix, a test case, or a major compiler pass—is celebrated and credited.

Thank you for being part of the Raven journey! 🚀
