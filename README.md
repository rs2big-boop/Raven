# Raven Programming Language

> **Raven** combines the expressiveness of **Python**, the functional pipelines of **Rust/Elixir**, and the dual-engine architecture of **Java/JVM**:
> featuring both an **AST Tree-Walk Interpreter** and the **RVM (Raven Virtual Machine)** with the **Corvus** execution engine.

---

## The Raven Toolchain & Developer Tools

Raven provides a complete Go-like developer ecosystem and dual-engine runtime:

| Tool / Command | Analogous to | Description |
|---|---|---|
| **`raven run <file>`** | `go run` / `python` | Execute Raven script with automatic module resolution and instant startup. |
| **`raven init [name]`** | `go mod init` / `cargo init` | Initialize a new structured Raven project with `raven.json` manifest and `src/main.rvn`. |
| **`raven fmt [-w] <file>`** | `gofmt` | Canonical code formatter (asthetically formats indentation, spacing, and braces). |
| **`raven vet <file>`** | `go vet` | Static analyzer / linter detecting unused variables, shadowing, and unreachable code. |
| **`raven doc <file>`** | `godoc` | Automatic API documentation generator producing GitHub-flavored Markdown. |
| **`raven bench <file>`** | `go test -bench` | Microsecond-accurate benchmark runner for scripts and modules. |
| **`raven lsp`** | `gopls` | Official Language Server Protocol (LSP) server providing diagnostics, completions, formatting, and hover. |
| **`ravenc`** | `javac` | Dedicated compiler: Compiles `.rvn` source code into optimized `.rvc` binary bytecode. |
| **`rvm`** | `java` (JVM) | High-speed Corvus Virtual Machine executing pre-compiled `.rvc` bytecode. |

---

## Go-Style Build Scripts

Standard Go-style root scripts are provided for fast development:
- `./make.bash` — Fast compile and link
- `./all.bash` — Full rebuild, test execution (57/57 tests), and stdlib tour run
- `./run.bash <file>` — Quick-run any Raven script
- `./clean.bash` — Clean build artifacts

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

### Structs and Object Construction (`struct`)
```raven
struct User { name, role, karma };

let user = User("Ran", "Lead Architect", 9000);
print(user.name);    // Ran
print(user.__type__); // User
user.karma = 9001;
```

### Python-Style String Interpolation (`f"..."`)
```raven
let name = "Raven";
let version = 2.0;
print(f"Welcome to {name} v{version}! 2 + 2 = {2 + 2}");
// Prints: Welcome to Raven v2! 2 + 2 = 4
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
- **`stdlib/fmt`** — Go-style formatted I/O (`println`, `eprintln`, `sprint`, `sprintf`, `printf`, `errorf`)
- **`stdlib/strings`** — Complete string utilities (`str_repeat`, `str_contains`, `str_has_prefix`, `str_has_suffix`, `str_count`, `str_index`, `str_title`, `str_words`, `str_lines`, `str_reverse`)
- **`stdlib/bytes`** — Byte slices, byte-to-string conversion, and dynamic `create_buffer()` (Buffer Writer)
- **`stdlib/slices`** — Modern array manipulation (`slices_unique`, `slices_contains`, `slices_reverse`, `slices_sum`, `slices_min`, `slices_max`, `slices_take`, `slices_drop`, `slices_chunk`, `slices_flatten`, `slices_zip`)
- **`stdlib/maps`** — Map utilities (`map_keys_list`, `map_values_list`, `map_has`, `map_del`, `map_copy`, `map_invert`, `map_equal`)
- **`stdlib/strconv`** — String to number & boolean conversions (`atoi`, `itoa`, `parse_int`, `parse_float`, `parse_bool`, `format_bool`)
- **`stdlib/encoding/json`** — Go-style `marshal()` and `unmarshal()`
- **`stdlib/encoding/base64`** — `encode_to_string()` and `decode_string()`
- **`stdlib/encoding/csv`** — `csv_read_all()` and `csv_write_all()`
- **`stdlib/encoding/hex`** — `encode_to_hex()`
- **`stdlib/crypto`** & **`stdlib/hash/fnv`** — `fnv_hash()`, `sha256_sum()`, `md5_sum()`
- **`stdlib/net/http`** & **`stdlib/net/url`** — `http_get_url()`, `http_post()`, `http_post_json()`, `url_encode()`, `parse_query()`
- **`stdlib/time`** — `now_ms()`, `now_unix()`, `sleep_duration_ms()`, `since_ms()`, `format_duration()`
- **`stdlib/path`** & **`stdlib/path/filepath`** — `path_join()`, `path_base()`, `path_dir()`, `path_ext()`, `path_is_abs()`
- **`stdlib/io`** & **`stdlib/bufio`** — `read_all()`, `write_all()`, `append_all()`, `read_all_lines()`, `create_scanner()`, `create_word_scanner()`
- **`stdlib/os`** — `get_env()`, `path_exists()`, `run_command()`, `current_dir()`, `make_dir()`, `remove()`, `read_dir()`, `is_directory()`
- **`stdlib/log`** — Leveled structured logger (`log_debug`, `log_info`, `log_warn`, `log_error`, `log_fatal`)
- **`stdlib/context`** — Cancellation context (`create_context()`, `cancel()`, `is_done()`, `set_deadline_ms()`)
- **`stdlib/sync`** — Concurrency primitives (`create_mutex()`, `create_once()`, `create_wait_group()`)
- **`stdlib/container/heap`**, **`container/list`**, **`container/ring`** — Min-heap priority queues, linked lists, circular rings
- **`stdlib/archive/zip`** & **`stdlib/archive/tar`** — `zip_extract()`, `zip_create()`, `tar_extract()`, `tar_create()`
- **`stdlib/compress/gzip`** — `gzip_compress_string()`, `gzip_decompress_string()`
- **`stdlib/regexp`** — Regular expression matching & replacement (`compile()`, `match_string()`, `replace_all_string()`)
- **`stdlib/reflect`** — Type reflection (`type_of()`, `is_nil()`, `is_string()`, `is_number()`, `fields()`)
- **`stdlib/flag`** — Command-line argument parsing (`flag_parse()`, `flag_string()`, `flag_bool()`, `flag_int()`)
- **`stdlib/html`** — `html_escape()`, `html_unescape()`, `html_tag()`
- **`stdlib/unicode`** — `unicode_is_digit()`, `unicode_is_alpha()`, `unicode_is_upper()`, `unicode_code_point()`
- **`stdlib/cmp`** — `compare()`, `less()`, `or_default()`
- **`stdlib/testing`** — Unit testing framework (`describe()`, `test()`, `expect_eq()`, `expect_true()`)

### Built-in Functions Reference
| Function | Description |
|----------|-------------|
| `print(v)` / `print_err(v)` | Print to stdout / stderr |
| `input()` | Read line from stdin |
| `len(v)` | Length of string, array, or map |
| `type(v)` | Type name as string |
| `str(v)` / `num(v)` / `int(v)` / `bool(v)` | Type conversions |
| `format(tmpl, ...)` | Positional string formatting |
| `abs(x)` / `min(a,b)` / `max(a,b)` / `clamp(x,lo,hi)` | Arithmetic & clamping |
| `gcd(a,b)` / `lcm(a,b)` | Greatest common divisor & least common multiple |
| `sqrt(x)` / `pow(b,e)` / `sin(x)` / `cos(x)` / `tan(x)` | Trigonometry & powers |
| `floor(x)` / `ceil(x)` / `round(x)` | Rounding |
| `log(x)` / `exp(x)` | Logarithm & exponentials |
| `rand()` / `rand_int(min, max)` | PRNG random generators |
| `range(start, end)` | Numeric sequences |
| `push(arr, v)` / `pop(arr)` | Dynamic array mutations |
| `keys(map)` / `values(map)` | Map inspections |
| `map_merge(m1, m2)` / `map_has_key(m, k)` / `map_delete(m, k)` | Map primitives |
| `sort_arr(arr)` | In-place native array sort |
| `sum(arr)` / `arr_min(arr)` / `arr_max(arr)` | Native array aggregations |
| `unique(arr)` / `flatten(arr)` / `arr_reverse(arr)` / `count_if(arr, fn)` | High-speed array operations |
| `map(arr, fn)` / `filter(arr, fn)` / `reduce(arr, fn, init)` | Functional primitives |
| `split(s, sep)` / `join(arr, sep)` | String manipulation |
| `str_repeat(s, n)` / `pad_left(s, w, ch)` / `pad_right(s, w, ch)` | String formatting |
| `str_reverse(s)` / `str_count(s, sub)` / `has_prefix(s, p)` / `has_suffix(s, s)` | String queries |
| `is_digit(s)` / `is_alpha(s)` / `is_space(s)` / `is_upper(s)` / `is_lower(s)` | Char predicates |
| `slice(seq, start, end)` | Slicing strings and arrays |
| `regex_match(str, pat)` / `regex_replace(str, pat, rep)` | Regex engine |
| `hash(str)` / `base64_encode(s)` / `base64_decode(s)` | Hashing & Base64 |
| `http_get(url)` | Native HTTP client |
| `contains(container, elem)` | Universal membership check |
| `to_upper(s)` / `to_lower(s)` / `trim(s)` / `replace(s, from, to)` | String transforms |
| `char_code(s)` / `char(n)` | ASCII code & character conversions |
| `to_json(v)` / `parse_json(s)` | High-speed JSON serialization & parsing |
| `read_file(path)` / `write_file(path, data)` / `append_file(path, data)` | Direct filesystem I/O |
| `read_lines(path)` / `file_exists(path)` / `is_dir(path)` / `list_dir(path)` | Filesystem inspection |
| `mkdir(path)` / `remove_file(path)` / `get_cwd()` | Directory & process utilities |
| `time()` / `sleep(ms)` | Epoch clock & thread sleep |
| `env(var)` / `exec(cmd)` | Environment variables & shell execution |
| `assert(cond, msg)` / `panic(msg)` | Runtime assertions & panics |

---

## Building, Installing and Testing

Raven includes a comprehensive test suite (57 automated unit, regression, toolchain & VM integration tests):

```bash
# Fast build and test
./all.bash

# Or standard CMake:
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure

# System-wide Installation (optional)
sudo ./install.sh
```

## VS Code Extension

Raven comes with a full-featured Visual Studio Code extension located in `editors/vscode`:
- **Syntax Highlighting**: Complete grammar for keywords, builtins, operators, strings, numbers, and comments.
- **Language Server (LSP)**: Automatic diagnostics on type, live linting, code completions, hover tooltips, and document formatting.
- **Snippets**: Pre-built templates for `fn`, `struct`, `while`, `for-in`, `if-else`, `match`, `try-catch`, and `import`.
- **Keybindings & Actions**:
  - `Shift+F5`: Run active Raven file in terminal
  - Format on Save: Configurable automatic `raven fmt` execution
  - Context Menu Actions: Run, Format, Vet, Doc generation
  - Status Bar: One-click "Raven Run" button

To install:
```bash
cp -r editors/vscode ~/.vscode/extensions/raven-lang
```

---

## License

Raven is licensed under the [MIT License](LICENSE).
