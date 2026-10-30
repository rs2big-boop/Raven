# ADR 0001: Dual-Engine Runtime Architecture (Tree-Walk + Corvus VM)

- **Status**: ACCEPTED [IMPLEMENTED]
- **Date**: 2026-10-28
- **Author**: Raven Language Core Architecture Team

## Context
A modern scripting and application language faces two competing requirements:
1. **Developer Experience & Tooling**: Rapid iteration, immediate REPL startup, lossless AST access for formatting (`raven fmt`), linting (`raven vet`), language server diagnostics (`raven lsp`), and exact source line/column runtime stack traces.
2. **Execution Performance & Portability**: High-throughput execution without the overhead of tree traversal, compact bytecode serialization (`.rvc`) for distribution, and pre-compiled bytecode loading.

Relying solely on an AST interpreter sacrifices execution throughput and memory density. Relying solely on a complex JIT/VM upfront slows development of developer tooling and increases debugging complexity.

## Decision
Raven implements a **Dual-Engine Runtime Architecture**:
- **Tree-Walk Interpreter (`Interpreter`)**: Operates directly on the AST via visitor pattern (`ExprVisitor<Value>`, `StmtVisitor<void>`). Used for instant scripts (`raven run`), the interactive REPL, and developer tooling where AST fidelity and precise line-level debugging are essential.
- **Corvus Virtual Machine (`VM` + `Compiler`)**: A stack-based bytecode virtual machine. The `Compiler` transforms AST statements into linear chunks of bytecode (`Chunk`), serialized to `.rvc` files by `ravenc` and executed by `rvm`.

Both engines share identical Frontend components (`Lexer`, `Parser`, `AST`), runtime value representation (`Value`), and standard library native bindings.

## Consequences

### Positive
- Instantaneous startup for scripts and interactive REPL without bytecode compilation overhead.
- High-speed VM execution for compute-intensive workloads and compiled deployment.
- Independent optimization paths: AST transforms (constant folding) and bytecode peephole optimization.
- Enables **Differential Testing**: any program executed by both engines must yield identical output and side effects.

### Negative / Trade-offs
- Must maintain semantic parity across both execution engines as new language features are added.
- Features must be implemented in both the AST visitor and the bytecode compiler/VM dispatcher.
