# Raven Language Specification v1.0.0

This document defines the formal grammar, syntax, lexical structure, type system, and semantics of the Raven programming language.

---

## 1. Lexical Structure

### 1.1 Source Encoding
Raven source files are encoded in UTF-8.

### 1.2 Comments
- Single-line comments start with `//` and extend to the end of the line:
  ```raven
  // This is a comment
  ```
- Block comments are enclosed between `/*` and `*/` and can span multiple lines:
  ```raven
  /* Multi-line
     block comment */
  ```

### 1.3 Whitespace
Whitespace (spaces, tabs, newlines) serves as token separators and is otherwise ignored outside string literals.

### 1.4 Identifiers
Identifiers name variables, functions, and parameters. They must start with an ASCII letter or underscore (`[a-zA-Z_]`), followed by zero or more alphanumeric characters or underscores (`[a-zA-Z0-9_]*`).

### 1.5 Keywords
The following reserved keywords cannot be used as identifiers:
```
let        fn         return     if         else
while      for        in         break      continue
loop       struct     try        catch      finally
throw      import     spawn      is         nil
true       false
```

### 1.6 Literals
- **Numeric**: Decimal integers and floating-point values: `42`, `3.14159`, `-10`.
- **String**: Enclosed in double quotes `"..."` or triple quotes `"""..."""` for multiline strings. Supports escape sequences (`\n`, `\t`, `\r`, `\"`, `\\`) and string interpolation with `${expression}`.
- **Boolean**: `true` and `false`.
- **Nil**: `nil` represents the absence of a value.

---

## 2. Type System

Raven features dynamic typing with strict runtime safety and an explicit `is` type-checking operator.

| Type | Syntax / Literal | Description |
|---|---|---|
| `Nil` | `nil` | Unit / null value |
| `Bool` | `true`, `false` | Boolean values |
| `Number` | `10`, `-3.5` | 64-bit IEEE 754 floating point numbers |
| `String` | `"hello"`, `"""..."""` | UTF-8 string sequences |
| `Array` | `[1, 2, "three"]` | Dynamically-sized heterogeneous lists |
| `Map` | `{"key": "value"}` | String-keyed hash dictionaries |
| `Function`| `fn(x) { ... }` | First-class closures |
| `Channel` | via `chan_create()` | Thread-safe message passing channels |

### 2.1 Truthiness
The following values evaluate to `false` in conditionals:
- `nil`
- `false`
- `0` (and `0.0`)
- `""` (empty string)
- `[]` (empty array)
- `{}` (empty map)

All other values evaluate to `true`.

### 2.2 Type Checking (`is` Operator)
Types can be inspected at runtime using the `is` operator or `type(val)` builtin:
```raven
let x = 42;
if (x is Number) { ... }
if (x is String) { ... }
if (x is Bool)   { ... }
if (x is Array)  { ... }
if (x is Map)    { ... }
if (x is Nil)    { ... }
```

---

## 3. Expressions & Operators

### 3.1 Operator Precedence (Lowest to Highest)

| Precedence | Operator | Description | Associativity |
|---|---|---|---|
| 1 | `=` | Assignment | Right |
| 2 | `\|>` | Pipeline | Left |
| 3 | `??` | Null Coalescing | Right |
| 4 | `\|\|` | Logical OR | Left |
| 5 | `&&` | Logical AND | Left |
| 6 | `==`, `!=` | Equality | Left |
| 7 | `<`, `<=`, `>`, `>=`, `is` | Comparison & Type Check | Left |
| 8 | `+`, `-` | Addition, Subtraction, String Concatenation | Left |
| 9 | `*`, `/`, `%` | Multiplication, Division, Modulo | Left |
| 10 | `!`, `-` (unary) | Logical NOT, Negation | Right |
| 11 | `()`, `[]`, `.`, `?.` | Function call, Indexing, Member access, Optional chain | Left |

### 3.2 Null Coalescing (`??`)
Evaluates to the right-hand operand if the left-hand operand is `nil`:
```raven
let port = env("PORT") ?? "8080";
```

### 3.3 Optional Chaining (`?.`)
Safely navigates properties without raising runtime errors if the target is `nil`:
```raven
let city = user?.address?.city; // Returns nil if user or address is nil
```

### 3.4 Spread Operator (`...`)
Expands arrays or maps inside collection literals:
```raven
let a = [1, 2];
let b = [...a, 3, 4]; // [1, 2, 3, 4]

let base = {"theme": "dark"};
let user = {...base, "name": "Ran"};
```

---

## 4. Statements & Control Flow

### 4.1 Variable Declaration
Variables are declared using `let`. Reassignment does not use `let`:
```raven
let count = 10;
count = count + 1;
```

### 4.2 Conditionals
```raven
if (condition) {
    // ...
} else if (another_condition) {
    // ...
} else {
    // ...
}
```

### 4.3 Loops
- **`while`**:
  ```raven
  let i = 0;
  while (i < 5) {
      i += 1;
  }
  ```
- **`for-in`**:
  ```raven
  for (let item in ["a", "b", "c"]) {
      print(item);
  }
  ```
- **`loop`** (Infinite loop equivalent to `while (true)`):
  ```raven
  let count = 0;
  loop {
      count += 1;
      if (count >= 10) { break; }
  }
  ```
- **Control**: `break` terminates loop execution, `continue` jumps to the next iteration.

---

## 5. Functions & Closures

Functions are first-class citizens:
```raven
fn calculate(a, b = 10) {
    return a * b;
}
```
Anonymous lambdas:
```raven
let double = fn(x) { return x * 2; };
```
Closures capture lexical state:
```raven
fn make_adder(x) {
    return fn(y) { return x + y; };
}
let add5 = make_adder(5);
print(add5(3)); // 8
```

---

## 6. Concurrency & CSP

Raven provides native concurrency using Communicating Sequential Processes (CSP):
- **`spawn(fn)`**: Executes a function concurrently in a managed OS thread.
- **Channels**: Synchronous (unbuffered) or asynchronous (buffered) communication channels:
  ```raven
  let ch = chan_create(2);
  spawn(fn() {
      chan_send(ch, "message");
  });
  let msg = chan_recv(ch);
  ```
- **Timeouts**: Non-blocking channel read with timeout:
  ```raven
  let res = chan_recv_timeout(ch, 500); // Wait up to 500ms
  ```

---

## 7. Error Handling

- **Exceptions**: `try`, `catch`, `finally`, and `throw`:
  ```raven
  try {
      throw "database connection failed";
  } catch (err) {
      print("Caught: " + str(err));
  } finally {
      print("cleanup complete");
  }
  ```
- **Fatal Terminations**: `panic(msg)` causes an unrecoverable failure and prints stack trace.
