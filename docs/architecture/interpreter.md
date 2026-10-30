# Tree-Walk Interpreter Architecture

The Raven Tree-Walk Interpreter is the reference execution engine for Raven programs. It directly traverses the Abstract Syntax Tree (AST) produced by the parser without an intermediate bytecode compilation step.

---

## 1. Overview & Role

- **Execution Model**: Visitor pattern traversing AST nodes (`ExprVisitor<Value>` and `StmtVisitor<void>`).
- **Use Cases**: Rapid prototyping, REPL interactive sessions, debugging with breakpoints and single-stepping, and behavioral ground-truth for VM differential testing.
- **Speed**: Optimized for fast startup and interactive latency rather than peak loop throughput.

---

## 2. Key Components

### 2.1 AST Visitor
The `Interpreter` class implements the visitor pattern defined in `include/raven/ast.hpp`:
```cpp
class Interpreter : public ExprVisitor<Value>, public StmtVisitor<void> { ... }
```
- Each statement executes via `execute(const Stmt& stmt)`.
- Each expression evaluates to a tagged `Value` via `evaluate(const Expr& expr)`.

### 2.2 Environment & Scoping Chain
Variable scopes are managed by the `Environment` class (`include/raven/environment.hpp`):
- Each lexical block (`{ ... }`) pushes a new child `Environment` holding a pointer to its enclosing scope.
- Lookups traverse upward to the global environment.
- Lexical closures hold a `std::shared_ptr<Environment>` to preserve variables across invocations.

### 2.3 Interactive Debugging Hooks
The interpreter exposes a step callback hook:
```cpp
using StepCallback = std::function<void(Interpreter&, const Stmt&)>;
void set_step_callback(StepCallback cb);
```
Before executing each statement, `execute()` invokes this callback, enabling:
- Breakpoint evaluation based on file and line numbers.
- Single-stepping (`step`, `next`).
- Live inspection of local variables via `environment()->values()`.

---

## 3. Call Stack & Diagnostics

When an unhandled exception or runtime error occurs, the interpreter constructs a rich stack trace via `CallStackGuard`:
```cpp
struct StackFrame {
    std::string function_name;
    size_t line;
    size_t column;
};
```
Whenever an undefined variable error occurs, the interpreter runs a Levenshtein distance check against all visible identifiers in scope to print helpful "Did you mean 'x'?" suggestions.
