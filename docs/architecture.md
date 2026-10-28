# MiniLang Architecture & Design

MiniLang is an educational, production-style tree-walk interpreter written from scratch in modern C++20.

```
+-----------------------------------------------------------+
|                       Source Code                         |
|                   (e.g., hello.mini)                      |
+-----------------------------+-----------------------------+
                              |
                              v
+-----------------------------------------------------------+
|                          Lexer                            |
|             Converts raw text into Token stream           |
+-----------------------------+-----------------------------+
                              |
                              v
+-----------------------------------------------------------+
|                         Parser                            |
|        Validates syntax and builds the Abstract           |
|                  Syntax Tree (AST)                        |
+-----------------------------+-----------------------------+
                              |
                              v
+-----------------------------------------------------------+
|                      Interpreter                          |
|         Evaluates expressions, manages scopes             |
|                 and executes statements                   |
+-----------------------------------------------------------+
```

---

## 1. Pipeline Stages

### 01. Lexical Analysis (`Lexer`)
- **Input**: Raw text (`std::string`).
- **Output**: Linear stream of `Token` structures (`std::vector<Token>`).
- **Role**:
  - Classifies character sequences into `TokenType` (Keywords, Identifiers, Numbers, Strings, Operators).
  - Skips non-semantic whitespace and comments (both single-line `//` and block `/* ... */`).
  - Records precise 1-based `line` and `column` for actionable error diagnostics.
  - Generates `TokenType::Invalid` instead of crashing on unrecognized characters.

### 02. Syntactic Analysis (`Parser`)
- **Input**: `std::vector<Token>`.
- **Output**: `std::vector<std::shared_ptr<Stmt>>` (AST root nodes).
- **Technique**: Recursive Descent with Precedence Climbing.
  - Implements unambiguous operator precedence:
    `Assignment < Logical OR < Logical AND < Equality < Comparison < Term (+,-) < Factor (*,/,%) < Unary (!,-) < Call < Primary`.
  - Panic-mode error recovery (`synchronize()`): recovers at statement boundaries (`let`, `if`, `while`, `;`) to report multiple diagnostics in a single run.

### 03. Abstract Syntax Tree (`AST`)
- **Design**: Strongly-typed node hierarchies with modern C++ Visitor pattern:
  - `Expr`: `BinaryExpr`, `UnaryExpr`, `LiteralExpr`, `VariableExpr`, `AssignExpr`, `LogicalExpr`, `CallExpr`, `GroupingExpr`.
  - `Stmt`: `VarDeclStmt`, `ExpressionStmt`, `PrintStmt`, `BlockStmt`, `IfStmt`, `WhileStmt`, `FunctionStmt`, `ReturnStmt`.
- Node lifecycles are managed cleanly using `std::unique_ptr` and `std::shared_ptr`.

### 04. Environment & Scopes (`Environment`)
- Implements **lexical scoping** via linked environment frames (`enclosing_`).
- Variable declaration (`define`): binds variable in current scope.
- Variable assignment (`assign`): mutates the variable in the innermost scope where it was defined, or throws `RuntimeError`.
- Nested blocks `{ ... }` create child environments; when the block terminates, the outer scope is restored.

### 05. Execution Engine (`Interpreter`)
- Implements `ExprVisitor<Value>` and `StmtVisitor<void>`.
- **Dynamic Typing (`Value`)**:
  - Encapsulated via `std::variant<Nil, bool, double, std::string, std::shared_ptr<Callable>>`.
  - Natural operator semantics:
    - `+` performs arithmetic addition for numbers, and string concatenation if an operand is a string.
    - Short-circuit evaluation for `&&` and `||`.
    - Truthiness semantics: `nil`, `false`, `0`, and empty strings are falsy.
- **Functions & Recursion**:
  - First-class user functions (`UserFunction`) capturing their lexical environment (closures).
  - Return statements unwind the call stack via a lightweight `ReturnException`.
