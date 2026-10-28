# Raven Programming Language

> **Raven** combines the expressiveness of **Python**, the functional pipelines of **Rust/Elixir**, and the dual-engine architecture of **Java/JVM**:
> featuring both an **AST Tree-Walk Interpreter** and the **RVM (Raven Virtual Machine)** with the **Corvus** execution engine.

---

## The Raven Toolchain

Similar to Java (`javac` + `java` JVM), Raven provides a dedicated, full-scale compiler and virtual machine toolchain:

| Tool | Analogous to | Description |
|---|---|---|
| **`ravenc`** | `javac` | **Raven Compiler**: Compiles `.rvn` source code into optimized `.rvc` binary bytecode with compile-time constant folding and dead-code elimination. |
| **`rvm`** | `java` (JVM) | **Raven Virtual Machine (Corvus Engine)**: High-speed stack-based virtual machine executing pre-compiled `.rvc` bytecode (or compiling on-the-fly). |
| **`raven`** | `python` / `node` | **Unified CLI**: Multi-purpose frontend for running scripts, launching the interactive REPL, or disassembling code. |

---

## File Formats

- **`.rvn`** — Human-readable Raven source code.
- **`.rvc`** — Raven Compiled Bytecode binary (Magic bytes `\x7fRVC`, Constant Pool, OpCode instructions).

---

## Quickstart

### 1. Compile to Bytecode with `ravenc`
```bash
./build/ravenc examples/hello.rvn -o hello.rvc -O
```
*Output:*
```text
Successfully compiled 'examples/hello.rvn' -> 'hello.rvc' (31 bytes bytecode, 9 constants)
```

### 2. Execute Bytecode with `rvm` (Corvus Engine)
```bash
./build/rvm -b hello.rvc
```
*Output:*
```text
Hello, Ran
Adult

[RVM Benchmark: execution finished in 21 us (0.021 ms)]
```

### 3. Disassemble Bytecode
```bash
./build/ravenc -d examples/hello.rvn
```
*Output:*
```text
== examples/hello.rvn (Bytecode Disassembly) ==
0000    1 OP_CONSTANT        0 ('Ran')
0002    | OP_DEFINE_GLOBAL   1 ('name')
0004    | OP_CONSTANT        2 ('20')
0006    2 OP_DEFINE_GLOBAL   3 ('age')
0008    4 OP_CONSTANT        4 ('Hello, ')
0010    | OP_GET_GLOBAL      5 ('name')
0012    | OP_ADD
0013    | OP_PRINT
0014    6 OP_GET_GLOBAL      6 ('age')
0016    4 OP_CONSTANT        7 ('18')
0018    6 OP_GREATER_EQUAL
0019    | OP_JUMP_IF_FALSE   -> 29
0022    | OP_POP
0023    | OP_CONSTANT        8 ('Adult')
0025    | OP_PRINT
0026    | OP_JUMP            -> 30
0029    | OP_POP
0030    | OP_RETURN
===============================================
```

### 4. Interactive REPL
```bash
./build/raven
```

---

## Language Highlights

### Rust-Style Pattern Matching (`match`)
```raven
let status_code = 404;
let message = match (status_code) {
    200 => "OK",
    404 => "Not Found",
    500 => "Internal Server Error",
    _   => "Unknown Status"
};
print(message); // Prints: Not Found
```

### Modular Standard Library (`import`)
```raven
import "stdlib/math";
import "stdlib/collections";
import "stdlib/iter";
import "stdlib/string";

// 1. Data Structures: Stack, Queue, Set
let stack = create_stack();
stack.push("hello");
stack.push("world");
print(stack.pop()); // world

// 2. Sequence Processing
let paired = zip(["A", "B"], [1, 2]); // [["A", 1], ["B", 2]]
let chunks = chunk([1, 2, 3, 4], 2);  // [[1, 2], [3, 4]]

// 3. Math & Statistics
print("pow(2, 8) = " + pow(2, 8));           // 256
print("hypot(3, 4) = " + hypot(3, 4));       // 5
print("mean([10, 20, 30]) = " + mean([10, 20, 30])); // 20

// 4. Native JSON & File I/O
let json_str = to_json({"language": "Raven", "score": 100});
let obj = parse_json(json_str);
print(obj.language); // Raven
```

### Functional Pipelines (`|>`) and First-Class Functions
```raven
let numbers = range(1, 10);
let square = fn(x) { return x * x; };
let is_even = fn(x) { return x % 2 == 0; };

let result = numbers 
    |> fn(arr) { return filter(arr, is_even); }
    |> fn(arr) { return map(arr, square); };
```

---

## Standard Library & Native Builtins

Raven ships with an extensive, batteries-included standard library:

### Modules
- **`stdlib/math`** — `PI`, `E`, `abs`, `min`, `max`, `pow`, `factorial`, `clamp`, `deg_to_rad`, `rad_to_deg`, `hypot`, `sum`, `mean`, `variance`, `std_dev`
- **`stdlib/collections`** — `create_stack()`, `create_queue()`, `create_set()`
- **`stdlib/iter`** — `zip`, `take`, `drop`, `chunk`, `flatten`, `any`, `all`, `find`, `reversed`
- **`stdlib/sort`** — `bubble_sort(arr)`, `insertion_sort(arr)`, `min_of_arr(arr)`, `max_of_arr(arr)`
- **`stdlib/string` & `stdlib/string_utils`** — `repeat`, `pad_left`, `pad_right`, `starts_with`, `ends_with`, `count_occurrences`
- **`stdlib/os`** — `get_env`, `path_exists`, `run_command`
- **`stdlib/testing`** — `describe`, `test`, `expect_eq`, `expect_true`, `expect_false`

### Built-in Functions Reference
| Function | Description |
|----------|-------------|
| `print(v)` / `print_err(v)` | Print to stdout / stderr |
| `input()` | Read line from stdin |
| `len(v)` | Length of string, array, or map |
| `type(v)` | Type name as string |
| `str(v)` / `num(v)` | Type conversion |
| `abs(x)` / `min(a,b)` / `max(a,b)` | Core arithmetic |
| `sqrt(x)` / `sin(x)` / `cos(x)` / `tan(x)` | Trigonometry & powers |
| `floor(x)` / `ceil(x)` / `round(x)` | Rounding |
| `log(x)` / `exp(x)` | Logarithm & exponentials |
| `rand()` / `rand_int(min, max)` | PRNG random generators |
| `range(start, end)` | Numeric sequences |
| `push(arr, v)` / `pop(arr)` | Dynamic array mutations |
| `keys(map)` / `values(map)` | Map inspections |
| `sort_arr(arr)` | In-place native array sort |
| `map(arr, fn)` / `filter(arr, fn)` / `reduce(arr, fn, init)` | Functional primitives |
| `split(s, sep)` / `join(arr, sep)` | String manipulation |
| `contains(container, elem)` | Universal membership check |
| `to_upper(s)` / `to_lower(s)` / `trim(s)` / `replace(s, from, to)` | String transforms |
| `char_code(s)` / `char(n)` | ASCII code & character conversions |
| `to_json(v)` / `parse_json(s)` | High-speed JSON serialization & parsing |
| `read_file(path)` / `write_file(path, data)` / `file_exists(path)` | Direct filesystem I/O |
| `time()` / `sleep(ms)` | Epoch clock & thread sleep |
| `env(var)` / `exec(cmd)` | Environment variables & shell execution |
| `assert(cond, msg)` | Runtime assertions |

---

## Building and Testing

Raven includes a comprehensive test suite (46 automated unit, regression & VM integration tests):

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```
