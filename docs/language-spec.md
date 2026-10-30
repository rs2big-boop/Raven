# Raven Language Specification v1.0.0

> **Raven** is a modern, dynamically-typed, multi-paradigm programming language designed for expressiveness, performance, and developer ergonomics. It compiles to efficient bytecode and runs on the Corvus virtual machine, while also supporting direct tree-walk interpretation for rapid development.

---

## Table of Contents

1. [File Format](#file-format)
2. [Formal Grammar (EBNF)](#formal-grammar-ebnf)
3. [Lexical Structure](#lexical-structure)
4. [Data Types](#data-types)
5. [Variables & Scope](#variables--scope)
6. [Operators](#operators)
7. [Control Flow](#control-flow)
8. [Functions](#functions)
9. [Closures & Lambdas](#closures--lambdas)
10. [Pattern Matching](#pattern-matching)
11. [Structs](#structs)
12. [Error Handling](#error-handling)
13. [Modules & Imports](#modules--imports)
14. [Concurrency](#concurrency)
15. [String Interpolation](#string-interpolation)
16. [Built-in Functions](#built-in-functions)
17. [Standard Library](#standard-library)

---

## File Format

Raven source files use the `.rvn` extension (e.g., `main.rvn`).
Compiled bytecode uses the `.rvc` extension (Raven Compiled Bytecode, magic bytes `\x7fRVC`).

---

## Formal Grammar (EBNF)

```ebnf
program        -> declaration* EOF ;

declaration    -> funDecl
                | structDecl
                | varDecl
                | importStmt
                | statement ;

funDecl        -> "fn" IDENTIFIER "(" parameters? ")" block ;
structDecl     -> "struct" IDENTIFIER "{" IDENTIFIER ("," IDENTIFIER)* "}" ";" ;
parameters     -> IDENTIFIER ( "," IDENTIFIER )* ;

varDecl        -> "let" IDENTIFIER ( "=" expression )? ";" ;
importStmt     -> "import" STRING ";" ;

statement      -> exprStmt
                | ifStmt
                | whileStmt
                | forInStmt
                | breakStmt
                | continueStmt
                | tryCatchStmt
                | throwStmt
                | returnStmt
                | block ;

exprStmt       -> expression ";" ;
ifStmt         -> "if" "(" expression ")" statement ( "else" statement )? ;
whileStmt      -> "while" "(" expression ")" statement ;
forInStmt      -> "for" "(" "let"? IDENTIFIER "in" expression ")" statement ;
breakStmt      -> "break" ";" ;
continueStmt   -> "continue" ";" ;
tryCatchStmt   -> "try" statement "catch" "(" IDENTIFIER ")" statement ;
throwStmt      -> "throw" expression ";" ;
returnStmt     -> "return" expression? ";" ;
block          -> "{" declaration* "}" ;

expression     -> assignment ;
assignment     -> ( IDENTIFIER | indexExpr | propertyExpr ) ( "=" | "+=" | "-=" | "*=" | "/=" | "%=" ) assignment
                | pipeline ;
pipeline       -> logic_or ( "|>" logic_or )* ;
logic_or       -> logic_and ( "||" logic_and )* ;
logic_and      -> equality ( "&&" equality )* ;
equality       -> comparison ( ( "!=" | "==" ) comparison )* ;
comparison     -> term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
term           -> factor ( ( "-" | "+" ) factor )* ;
factor         -> unary ( ( "/" | "*" | "%" ) unary )* ;
unary          -> ( "!" | "-" ) unary
                | call_or_index ;
call_or_index  -> primary ( "(" arguments? ")" | "[" expression "]" | "." IDENTIFIER )* ;
arguments      -> expression ( "," expression )* ;

primary        -> NUMBER | STRING | "true" | "false" | "nil" | "null"
                | IDENTIFIER
                | "[" ( expression ( "," expression )* )? "]"
                | "{" ( STRING ":" expression ( "," STRING ":" expression )* )? "}"
                | "fn" "(" parameters? ")" block
                | "match" "("? expression ")"? "{" matchArm* "}"
                | "(" expression ")" ;

matchArm       -> ( expression | "_" ) "=>" expression ( "," | ";" )? ;
```

---

## Lexical Structure

### Keywords

```
let  fn  if  else  while  for  in  return  break  continue
match  struct  import  try  catch  throw  true  false  nil  null
```

### Operators

| Operator | Meaning |
|---|---|
| `+`, `-`, `*`, `/`, `%` | Arithmetic |
| `==`, `!=`, `<`, `<=`, `>`, `>=` | Comparison |
| `&&`, `\|\|`, `!` | Logical |
| `=` | Assignment |
| `+=`, `-=`, `*=`, `/=`, `%=` | Compound Assignment |
| `\|>` | Pipeline operator |
| `.` | Property access |
| `[]` | Index access |

### Literals

- **Number**: `42`, `3.14`, `-7`, `0.001`
- **String**: `"Hello, World!"`, `"escape: \n \t \\"` 
- **String interpolation**: `"Hello, ${name}!"` — expressions inside `${}` are evaluated
- **Boolean**: `true`, `false`
- **Nil**: `nil`, `null`
- **Array**: `[1, 2, 3]`, `[]`
- **Map**: `{"key": "value"}`, `{}`

### Comments

```raven
// Single-line comment
/* Multi-line
   comment */
```

---

## Data Types

| Type | Examples | Description |
|---|---|---|
| `Number` | `0`, `42`, `3.14`, `-0.5` | IEEE 754 64-bit double-precision floating point |
| `String` | `"Raven"`, `"Hello\nWorld"` | UTF-8 string literals with full escape sequence support |
| `Boolean` | `true`, `false` | Logical Boolean values |
| `Array` | `[1, 2, "three", [4]]` | Dynamic mutable arrays with O(1) index, O(1) push/pop |
| `Map` | `{"name": "Ran", "age": 20}` | Hash map dictionaries; access via `.prop` or `["key"]` |
| `Nil` | `nil`, `null` | Absence of value |
| `Function` | `fn foo(x) { ... }` | First-class functions with full closures |
| `Struct` | `Point { x, y }` | Named value types with field access via `.field` |

---

## Variables & Scope

```raven
let x = 10;
let name = "Raven";
let items = [1, 2, 3];
let unset;           // nil by default

x = 20;             // reassignment
x += 5;             // compound assignment: 25
```

Variables are **lexically scoped** with closures capturing their enclosing environment. Block `{}` creates a new scope.

---

## Operators

### Pipeline Operator

The `|>` operator passes the left value as the sole argument to the right function:

```raven
let result = [3, 1, 2] |> sort_arr |> to_json;
print(result);  // "[1, 2, 3]"
```

### Compound Assignment

```raven
let n = 10;
n += 5;   // 15
n -= 3;   // 12
n *= 2;   // 24
n /= 4;   // 6
n %= 4;   // 2
```

---

## Control Flow

### If / Else

```raven
if (score >= 90) {
    print("A");
} else if (score >= 80) {
    print("B");
} else {
    print("C");
}
```

### While

```raven
let i = 0;
while (i < 5) {
    print(i);
    i += 1;
}
```

### For-In

```raven
let fruits = ["apple", "banana", "cherry"];
for (let fruit in fruits) {
    print(fruit);
}

for (let i in range(0, 10)) {
    print(i);
}
```

### Break & Continue

```raven
let i = 0;
while (i < 10) {
    if (i == 5) { break; }
    if (i % 2 == 0) { i += 1; continue; }
    print(i);
    i += 1;
}
```

---

## Functions

```raven
fn greet(name) {
    return "Hello, " + name + "!";
}

fn factorial(n) {
    if (n <= 1) { return 1; }
    return n * factorial(n - 1);
}
```

Functions are **first-class values** — stored in variables, passed as arguments, returned from other functions.

---

## Closures & Lambdas

```raven
fn make_counter(start) {
    let count = start;
    return fn() {
        count += 1;
        return count;
    };
}

let counter = make_counter(0);
print(counter());  // 1
print(counter());  // 2

let doubled = map([1, 2, 3], fn(x) { return x * 2; });  // [2, 4, 6]
```

---

## Pattern Matching

```raven
let desc = match (val) {
    0    => "zero",
    42   => "the answer",
    _    => "something else"
};

match (status) {
    "ok"    => print("All good"),
    "error" => print("Something failed"),
    _       => print("Unknown status")
}
```

---

## Structs

```raven
struct Point { x, y };

let p = Point { x: 10, y: 20 };
print(p.x);  // 10
p.x = 99;
print(p.x);  // 99
```

---

## Error Handling

```raven
try {
    let result = risky_operation();
} catch (err) {
    print("Caught: " + str(err));
}

fn validate(n) {
    if (n < 0) { throw "negative not allowed"; }
    return n;
}
```

**Stack traces** are automatically captured and printed for uncaught runtime errors:

```
[line 12, col 5] Runtime Error: Division by zero.
    at divide (line 3, col 14)
    at calculate (line 7, col 8)
    at main (line 12, col 5)
```

---

## Modules & Imports

```raven
import "stdlib/fmt";
import "stdlib/strings";
import "utils";       // local utils.rvn
```

Resolution order: CWD → stdlib/ → raven_modules/ → executable-relative.

---

## Concurrency

```raven
import "stdlib/sync";

let ch = create_channel(1);

go(fn() {
    ch.send(expensive_computation());
});

print(ch.recv());
```

### WaitGroups

```raven
import "stdlib/sync";

let wg = create_wait_group();
wg.add(2);
go(fn() { do_work_1(); wg.done(); });
go(fn() { do_work_2(); wg.done(); });
wg.wait();
print("done");
```

---

## String Interpolation

```raven
let name = "Raven";
let version = 1;
print("Welcome to ${name} v${version}.0!");  // Welcome to Raven v1.0!
print("${5} squared is ${5 * 5}");           // 5 squared is 25
```

---

## Built-in Functions

### I/O
| Function | Description |
|---|---|
| `print(v)` | Print to stdout |
| `print_err(v)` | Print to stderr |
| `input()` | Read line from stdin |

### Type Conversion
| Function | Description |
|---|---|
| `str(v)` | To string |
| `num(v)` | Parse number |
| `int(v)` | Truncate to integer |
| `bool(v)` | To boolean |
| `type(v)` | Runtime type name |

### Math
| Function | Description |
|---|---|
| `abs(x)` | Absolute value |
| `min(a,b)` / `max(a,b)` | Min/max |
| `clamp(v, lo, hi)` | Clamp to range |
| `sqrt(x)` / `pow(b,e)` | Square root / power |
| `floor(x)` / `ceil(x)` / `round(x)` | Rounding |
| `sin(x)` / `cos(x)` / `tan(x)` | Trig (radians) |
| `log(x)` / `exp(x)` | Log / exponential |
| `gcd(a,b)` / `lcm(a,b)` | GCD / LCM |
| `rand()` / `rand_int(lo,hi)` | Random |

### Strings
| Function | Description |
|---|---|
| `len(s)`, `slice(s,a,b)`, `split(s,sep)`, `join(arr,sep)` | Core |
| `contains(s,sub)`, `index_of(s,sub)` | Search |
| `to_upper(s)`, `to_lower(s)`, `trim(s)`, `replace(s,a,b)` | Transform |
| `str_repeat(s,n)`, `str_reverse(s)`, `str_count(s,sub)` | Extras |
| `has_prefix(s,p)`, `has_suffix(s,s)` | Checks |
| `pad_left(s,w,ch)`, `pad_right(s,w,ch)` | Padding |
| `format(tmpl, ...)` | Positional `{}` formatting |
| `char_code(s)`, `char(n)` | ASCII conversions |
| `is_digit(c)`, `is_alpha(c)`, `is_space(c)`, `is_upper(c)`, `is_lower(c)` | Char class |

### Arrays
| Function | Description |
|---|---|
| `push(arr,v)`, `pop(arr)`, `sort_arr(arr)` | Mutation |
| `arr_reverse(arr)`, `flatten(arr)`, `unique(arr)` | Transformation |
| `sum(arr)`, `arr_min(arr)`, `arr_max(arr)` | Aggregation |
| `arr_contains(arr,v)`, `count_if(arr,fn)` | Query |
| `map(arr,fn)`, `filter(arr,fn)`, `reduce(arr,fn,init)` | Functional |

### Maps
| Function | Description |
|---|---|
| `keys(m)`, `values(m)`, `map_has_key(m,k)` | Inspection |
| `map_merge(m1,m2)`, `map_delete(m,k)` | Modification |

### JSON & Encoding
| Function | Description |
|---|---|
| `to_json(v)`, `parse_json(s)` | JSON |
| `base64_encode(s)`, `base64_decode(s)` | Base64 |
| `hash(s)` | FNV-1a 64-bit hash |
| `regex_match(s,p)`, `regex_replace(s,p,r)` | Regex |

### Filesystem
| Function | Description |
|---|---|
| `read_file(p)`, `write_file(p,c)`, `append_file(p,c)` | File I/O |
| `read_lines(p)`, `file_exists(p)`, `is_dir(p)`, `list_dir(p)` | Inspection |
| `mkdir(p)`, `remove_file(p)`, `get_cwd()` | Management |

### System
| Function | Description |
|---|---|
| `time()`, `sleep(ms)` | Clock / sleep |
| `env(name)`, `exec(cmd)` | OS interface |
| `http_get(url)` | HTTP GET |

### Concurrency
| Function | Description |
|---|---|
| `spawn(fn)` | Run fn in new OS thread (detached) |
| `chan_create(cap)` | Create buffered channel |
| `chan_send(id, v)`, `chan_recv(id)` | Channel I/O |
| `chan_close(id)`, `chan_len(id)` | Channel management |

### Assertions
| Function | Description |
|---|---|
| `assert(cond, msg?)` | Assert truthy |
| `panic(msg)` | Unrecoverable abort with message |

---

## Standard Library

| Module | Key Exports |
|---|---|
| `stdlib/fmt` | `println`, `sprintf1`, `eprintln` |
| `stdlib/strings` | `str_title`, `str_words`, `str_lines`, `str_pad_left/right` |
| `stdlib/strconv` | `atoi`, `itoa`, `parse_float`, `format_bool` |
| `stdlib/errors` | `new_error`, `is_error`, `wrap_error`, `error_msg` |
| `stdlib/slices` | `slices_unique`, `slices_chunk`, `slices_zip`, `slices_index_of` |
| `stdlib/maps` | `map_invert`, `map_size` |
| `stdlib/math` | `PI`, `E`, `factorial`, `mean`, `variance`, `std_dev`, `clamp` |
| `stdlib/sort` | `bubble_sort`, `insertion_sort`, `min_of_arr`, `max_of_arr` |
| `stdlib/io` | `io_read_file`, `io_write_file`, `io_read_lines` |
| `stdlib/bufio` | `bufio_read_lines`, `bufio_scan_words` |
| `stdlib/path` | `basename`, `dirname`, `extname`, `path_join`, `is_abs` |
| `stdlib/log` | `log_debug`, `log_info`, `log_warn`, `log_error`, `log_fatal`, `set_log_level` |
| `stdlib/flag` | `flag_parse`, `flag_get`, `flag_bool`, `flag_string`, `flag_int` |
| `stdlib/time` | `now`, `since`, `format_duration`, `sleep_secs` |
| `stdlib/os` | `get_env`, `path_exists`, `run_command` |
| `stdlib/context` | `ctx_new`, `ctx_cancel`, `ctx_with_timeout`, `ctx_is_done` |
| `stdlib/sync` | `go`, `create_channel`, `create_wait_group`, `create_mutex`, `create_once` |
| `stdlib/encoding/json` | `marshal`, `unmarshal` |
| `stdlib/encoding/base64` | `encode`, `decode` |
| `stdlib/encoding/csv` | `csv_parse`, `csv_stringify`, `csv_headers` |
| `stdlib/net/http` | `http_get_url`, `http_post`, `http_post_json`, `url_encode` |
| `stdlib/html` | `html_escape`, `html_unescape`, `html_tag`, `html_attr` |
| `stdlib/unicode` | `unicode_is_upper/lower/digit/alpha`, `unicode_to_upper/lower` |
| `stdlib/crypto` | `crypto_hash`, `crypto_sha256`, `crypto_base64_encode` |
| `stdlib/regexp` | `compile(pattern)`, `.match(s)`, `.test(s)`, `.replace(s, r)` |
| `stdlib/container/heap` | `create_min_heap` with `.push/.pop/.peek/.len` |
| `stdlib/container/list` | `create_linked_list` with `.push_back/.pop_front/.len` |
| `stdlib/container/ring` | `create_ring(size)` |
| `stdlib/collections` | `create_stack`, `create_queue`, `create_set` |
| `stdlib/iter` | `zip`, `take`, `drop`, `chunk`, `flatten`, `any`, `all`, `find`, `reversed` |
| `stdlib/testing` | `describe`, `test`, `expect_eq`, `expect_true`, `expect_false` |
| `stdlib/reflect` | `type_of`, `is_nil`, `is_string`, `fields` |
| `stdlib/uuid` | `uuid_v4` (RFC 4122) |
| `stdlib/embed` | `embed_file`, `embed_read` |
| `stdlib/arena` | `create_arena` with `.allocate/.reset/.count` |
| `stdlib/unique` | `unique_make` (interned values) |
| `stdlib/strconv` | `atoi`, `itoa`, `parse_int`, `parse_float`, `format_int` |
| `stdlib/event` | `create_emitter` with `.on`, `.emit`, `.off`, `.listener_count` |
| `stdlib/cache` | `create_lru_cache`, `create_ttl_cache` |
| `stdlib/validate` | `validate_schema`, `validate_string`, `validate_number` |
| `stdlib/promise` | `create_promise`, `promise_resolve`, `promise_reject` |
| `stdlib/cli` | `create_cli` with `.command`, `.flag`, `.run` |

---

## Raven v1.0.0 — What's New

- **Native CSP Concurrency**: `spawn()` creates real OS threads; `chan_*` builtins provide typed, thread-safe channels; `stdlib/sync` wraps them into `go()`, `create_channel()`, `create_wait_group()`.
- **Default Function Parameters**: `fn greet(name = "World") { ... }` with default argument binding for functions and lambdas.
- **Null Coalescing Operator (`??`)**: `x ?? fallback` short-circuiting operator.
- **Optional Chaining (`?.`)**: Safe navigation `obj?.prop` and `arr?.[index]` without throwing when targeting `nil`.
- **`is` Type Checking Operator**: Type verification: `val is Number`, `val is String`, `val is Array`, etc.
- **Spread Operator (`...`)**: Modern inline expansion: `[...prefix, 4, 5]` and `{...m1, "k": v}`.
- **`loop {}` Infinite Loop**: Ergonomic infinite loop construct with `break` and `continue`.
- **Multi-line Triple-Quoted Strings**: `""" ... """` strings with preserved newlines.
- **"Did You Mean?" Suggestions**: Levenshtein distance error hints for undefined variables.
- **Full Call Stack Traces**: Runtime errors now include the full chain of function calls with name, line and column.
- **Package Manager**: `raven pkg install <git-url>[@tag]` clones packages into `raven_modules/` and produces a `raven.lock` file for reproducible builds. `raven pkg list` inspects installed packages.
- **Release Distributions**: `dist.bash` builds, tests, assembles, and SHA-256-checksums a `raven-v1.0.0-linux-x86_64.tar.gz` archive ready for upload.
- **68/68 Tests Passing**: Full test coverage across lexer, parser, interpreter, bytecode VM, stdlib, concurrency, and toolchain integration.
