# Raven Standard Library

The Raven standard library provides a rich set of packages for common programming tasks, comparable in breadth to Go's standard library.

## Import Syntax

```raven
import "stdlib/math";
import "stdlib/strings";
import "stdlib/encoding/json";
```

## Package Index

### Core Packages

| Package | Import Path | Description |
|---------|-------------|-------------|
| **math** | `stdlib/math` | Mathematical functions and constants |
| **strings** | `stdlib/strings` | String manipulation utilities |
| **strconv** | `stdlib/strconv` | String conversion functions |
| **fmt** | `stdlib/fmt` | Formatted I/O |
| **io** | `stdlib/io` | I/O primitives |
| **bufio** | `stdlib/bufio` | Buffered I/O |
| **os** | `stdlib/os` | Operating system interface |
| **path** | `stdlib/path` | Filepath manipulation |
| **time** | `stdlib/time` | Time utilities |
| **log** | `stdlib/log` | Logging |

### Collections & Data

| Package | Import Path | Description |
|---------|-------------|-------------|
| **collections** | `stdlib/collections` | Stack, Queue, Set implementations |
| **slices** | `stdlib/slices` | Slice/array utilities |
| **maps** | `stdlib/maps` | Map utilities |
| **sort** | `stdlib/sort` | Sorting algorithms |
| **iter** | `stdlib/iter` | Iterator utilities |
| **unique** | `stdlib/unique` | Deduplication |

### Encoding & Serialization

| Package | Import Path | Description |
|---------|-------------|-------------|
| **encoding/json** | `stdlib/encoding/json` | JSON encoding/decoding |
| **encoding/base64** | `stdlib/encoding/base64` | Base64 encoding/decoding |
| **encoding/csv** | `stdlib/encoding/csv` | CSV parsing |

### Networking

| Package | Import Path | Description |
|---------|-------------|-------------|
| **net/http** | `stdlib/net/http` | HTTP client |

### Concurrency & Sync

| Package | Import Path | Description |
|---------|-------------|-------------|
| **sync** | `stdlib/sync` | Synchronization primitives |
| **context** | `stdlib/context` | Context propagation |

### Utilities

| Package | Import Path | Description |
|---------|-------------|-------------|
| **errors** | `stdlib/errors` | Error handling patterns |
| **testing** | `stdlib/testing` | Testing framework |
| **flag** | `stdlib/flag` | Command-line flag parsing |
| **html** | `stdlib/html` | HTML escaping |
| **crypto** | `stdlib/crypto` | Hashing utilities |
| **regexp** | `stdlib/regexp` | Regular expressions |
| **unicode** | `stdlib/unicode` | Unicode utilities |
| **validate** | `stdlib/validate` | Input validation |
| **uuid** | `stdlib/uuid` | UUID generation |

## Design Philosophy

The Raven standard library is architected in two complementary layers:

1. **Native Builtins Core** — 50+ high-performance functions implemented directly in C++ and always available without any import. These cover I/O, math, string manipulation, collections, JSON, regex, and concurrency primitives.

2. **Raven Standard Library Modules** — Higher-level packages written entirely in Raven itself, building on the native builtins. Because they are authored in Raven, they are readable, composable, and extensible. Any user can inspect a standard library file (e.g. `stdlib/math.rvn`) to understand its implementation or copy and modify it.

This design means stdlib functions are thin, expressive wrappers that add convenience and composability on top of performant primitives — exactly the ergonomics you expect from a modern language.

## Native Builtins

These functions are always available without any import:

### I/O
| Function | Signature | Description |
|----------|-----------|-------------|
| `print` | `print(v)` | Print value to stdout |
| `print_err` | `print_err(s)` | Print to stderr |
| `input` | `input()` | Read line from stdin |
| `read_file` | `read_file(path)` | Read file as string |
| `write_file` | `write_file(path, content)` | Write file |
| `append_file` | `append_file(path, content)` | Append to file |
| `read_lines` | `read_lines(path)` | Read file as array of lines |
| `file_exists` | `file_exists(path)` | Check path exists |

### Type Conversion
| Function | Signature | Description |
|----------|-----------|-------------|
| `str` | `str(v)` | Convert to string |
| `num` | `num(v)` | Convert to number |
| `int` | `int(v)` | Truncate to integer |
| `bool` | `bool(v)` | Convert to boolean |
| `type` | `type(v)` | Get type name |

### Math
| Function | Signature | Description |
|----------|-----------|-------------|
| `abs` | `abs(x)` | Absolute value |
| `sqrt` | `sqrt(x)` | Square root |
| `pow` | `pow(b, e)` | Power |
| `floor` | `floor(x)` | Floor |
| `ceil` | `ceil(x)` | Ceiling |
| `round` | `round(x)` | Round |
| `min` | `min(a, b)` | Minimum of two values |
| `max` | `max(a, b)` | Maximum of two values |
| `clamp` | `clamp(v, lo, hi)` | Clamp value to range |
| `gcd` | `gcd(a, b)` | Greatest common divisor |
| `lcm` | `lcm(a, b)` | Least common multiple |
| `sin` / `cos` / `tan` | `sin(x)` | Trigonometry |
| `log` / `exp` | `log(x)` | Logarithm / exponential |
| `rand` | `rand()` | Random float [0, 1) |
| `rand_int` | `rand_int(lo, hi)` | Random integer in range |

### Collections
| Function | Signature | Description |
|----------|-----------|-------------|
| `len` | `len(v)` | Length of string/array/map |
| `range` | `range(n)` | Array [0..n-1] |
| `push` | `push(arr, v)` | Append to array |
| `pop` | `pop(arr)` | Remove last element |
| `slice` | `slice(arr, from, to)` | Sub-array/sub-string |
| `keys` | `keys(map)` | Map keys as array |
| `values` | `values(map)` | Map values as array |
| `sort_arr` | `sort_arr(arr)` | Sort array in-place |
| `arr_reverse` | `arr_reverse(arr)` | Reverse array |
| `arr_min` | `arr_min(arr)` | Minimum element |
| `arr_max` | `arr_max(arr)` | Maximum element |
| `sum` | `sum(arr)` | Sum of numeric array |
| `flatten` | `flatten(arr)` | Flatten one level |
| `unique` | `unique(arr)` | Deduplicate array |
| `arr_contains` | `arr_contains(arr, v)` | Array membership test |
| `map_has_key` | `map_has_key(m, k)` | Map key existence |
| `map_merge` | `map_merge(m1, m2)` | Merge two maps |
| `map_delete` | `map_delete(m, k)` | Delete key from map |

### Higher-Order Functions
| Function | Signature | Description |
|----------|-----------|-------------|
| `map` | `map(arr, fn)` | Transform array |
| `filter` | `filter(arr, fn)` | Filter array |
| `reduce` | `reduce(arr, fn, init)` | Fold array |
| `count_if` | `count_if(arr, fn)` | Count matching elements |

### Strings
| Function | Signature | Description |
|----------|-----------|-------------|
| `split` | `split(s, sep)` | Split string |
| `join` | `join(arr, sep)` | Join array |
| `contains` | `contains(s, sub)` | Substring check |
| `index_of` | `index_of(s, sub)` | Find substring index |
| `replace` | `replace(s, from, to)` | Replace first occurrence |
| `to_upper` | `to_upper(s)` | Uppercase |
| `to_lower` | `to_lower(s)` | Lowercase |
| `trim` | `trim(s)` | Strip whitespace |
| `has_prefix` | `has_prefix(s, prefix)` | Prefix check |
| `has_suffix` | `has_suffix(s, suffix)` | Suffix check |
| `str_count` | `str_count(s, sub)` | Count occurrences |
| `str_repeat` | `str_repeat(s, n)` | Repeat string |
| `str_reverse` | `str_reverse(s)` | Reverse string |
| `pad_left` | `pad_left(s, w, ch)` | Left-pad string |
| `pad_right` | `pad_right(s, w, ch)` | Right-pad string |
| `char_code` | `char_code(s)` | Char to ASCII code |
| `char` | `char(n)` | ASCII code to char |
| `is_digit` | `is_digit(c)` | Is digit char |
| `is_alpha` | `is_alpha(c)` | Is alpha char |
| `is_upper` | `is_upper(c)` | Is uppercase char |
| `is_lower` | `is_lower(c)` | Is lowercase char |
| `format` | `format(tmpl, ...)` | String formatting with `{}` |

### System
| Function | Signature | Description |
|----------|-----------|-------------|
| `time` | `time()` | Unix timestamp (milliseconds) |
| `sleep` | `sleep(ms)` | Sleep milliseconds |
| `env` | `env(name)` | Environment variable |
| `exec` | `exec(cmd)` | Run shell command |
| `panic` | `panic(msg)` | Throw fatal error |
| `assert` | `assert(cond)` | Assert condition |

### JSON / Encoding
| Function | Signature | Description |
|----------|-----------|-------------|
| `to_json` | `to_json(v)` | Encode to JSON string |
| `parse_json` | `parse_json(s)` | Parse JSON string |
| `base64_encode` | `base64_encode(s)` | Base64 encode |
| `base64_decode` | `base64_decode(s)` | Base64 decode |
| `hash` | `hash(s)` | FNV-64 hash |

### Regex
| Function | Signature | Description |
|----------|-----------|-------------|
| `regex_match` | `regex_match(pattern, s)` | Test regex match |
| `regex_replace` | `regex_replace(s, pattern, repl)` | Regex replace |

### Concurrency
| Function | Signature | Description |
|----------|-----------|-------------|
| `spawn` | `spawn(fn)` | Spawn goroutine-like task |
| `chan_make` | `chan_make(cap)` | Create channel |
| `chan_send` | `chan_send(ch, v)` | Send to channel |
| `chan_recv` | `chan_recv(ch)` | Receive from channel |
| `chan_recv_timeout` | `chan_recv_timeout(ch, ms)` | Receive with timeout |
| `chan_close` | `chan_close(ch)` | Close channel |

## See Also

- [Language Specification](../language/specification.md)
- [Concurrency Guide](../language/concurrency.md)
- [Architecture Overview](../architecture.md)
