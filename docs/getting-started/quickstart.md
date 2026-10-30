# Raven Quickstart Guide

Get up and running with Raven in under 5 minutes.

---

## 1. Interactive REPL

Start the Raven interactive shell:
```bash
raven
```

```raven
🐦 Raven 1.0.0 Interactive Shell
   Type .help for commands, .quit to exit

raven> let name = "Developer";
raven> "Hello, " + name + "!"
=> Hello, Developer!

raven> fn square(n) { return n * n; }
raven> square(9)
=> 81

raven> .quit
Goodbye! 🐦
```

---

## 2. Your First Script

Create `hello.rvn`:
```raven
// hello.rvn
let greeting = "Welcome to Raven!";
print(greeting);

// Modern ergonomics
let user = nil;
let default_user = user ?? "Anonymous";
print("Logged in as: " + default_user);

// Functional pipelines
let numbers = [1, 2, 3, 4, 5];
let doubled = map(numbers, fn(x) { return x * 2; });
print("Doubled: " + str(doubled));
```

Run it immediately:
```bash
raven run hello.rvn
```

---

## 3. Formatting and Linting

Raven comes with built-in tools inspired by Go:

```bash
# Format code canonically (gofmt style)
raven fmt hello.rvn -w

# Static analysis and unused variable detection (go vet style)
raven vet hello.rvn
```

---

## 4. Compiling to Bytecode

Raven features a dual-engine architecture. You can compile your source to bytecode:

```bash
# Compile to .rvc binary chunk
ravenc hello.rvn -o hello.rvc

# Execute bytecode on Corvus VM
rvm hello.rvc
```

---

## 5. Interactive Debugging

Debug your script with breakpoints and variable inspection:
```bash
raven debug hello.rvn
```

Commands:
- `b 5` — Set breakpoint at line 5
- `c` — Continue execution
- `locals` — Inspect active variables
- `p expression` — Evaluate expression in context
- `q` — Quit session
