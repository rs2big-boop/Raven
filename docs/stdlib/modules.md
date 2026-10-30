# Raven Standard Library — Module Reference

This document provides a detailed per-module reference for every package in the Raven standard library. Each module is written in Raven itself and lives under the `stdlib/` directory.

---

## `stdlib/math`

**Import:** `import "stdlib/math";`

Advanced mathematical functions and constants that build on the native math builtins.

### Constants

| Name | Value | Description |
|------|-------|-------------|
| `PI` | `3.141592653589793` | The mathematical constant π |
| `E` | `2.718281828459045` | Euler's number |

### Functions

| Function | Signature | Description |
|----------|-----------|-------------|
| `abs` | `abs(x)` | Absolute value of `x` |
| `max` | `max(a, b)` | Larger of two values |
| `min` | `min(a, b)` | Smaller of two values |
| `pow` | `pow(base, exp)` | Integer power (`base^exp`) |
| `factorial` | `factorial(n)` | Recursive factorial of `n` |
| `clamp` | `clamp(x, lo, hi)` | Clamp `x` to `[lo, hi]` |
| `deg_to_rad` | `deg_to_rad(deg)` | Convert degrees to radians |
| `rad_to_deg` | `rad_to_deg(rad)` | Convert radians to degrees |
| `hypot` | `hypot(a, b)` | Hypotenuse: `sqrt(a²+b²)` |
| `sum` | `sum(arr)` | Sum of all elements in an array |
| `mean` | `mean(arr)` | Arithmetic mean of an array |
| `variance` | `variance(arr)` | Population variance of an array |
| `std_dev` | `std_dev(arr)` | Standard deviation of an array |

### Example

```raven
import "stdlib/math";

print(PI);                         // 3.141592653589793
print(factorial(6));               // 720
print(clamp(150, 0, 100));         // 100
print(deg_to_rad(180));            // 3.141592653589793

let data = [2, 4, 4, 4, 5, 5, 7, 9];
print(mean(data));                 // 5
print(std_dev(data));              // approximately 2
```

---

## `stdlib/strings`

**Import:** `import "stdlib/strings";`

String manipulation utilities inspired by Go's `strings` package.

### Functions

| Function | Signature | Description |
|----------|-----------|-------------|
| `str_repeat` | `str_repeat(s, n)` | Repeat `s` exactly `n` times |
| `str_contains` | `str_contains(s, substr)` | Returns `true` if `s` contains `substr` |
| `str_has_prefix` | `str_has_prefix(s, prefix)` | Returns `true` if `s` starts with `prefix` |
| `str_has_suffix` | `str_has_suffix(s, suffix)` | Returns `true` if `s` ends with `suffix` |
| `str_count` | `str_count(s, substr)` | Count non-overlapping occurrences of `substr` in `s` |
| `str_index` | `str_index(s, substr)` | Index of first occurrence of `substr`, or `-1` |
| `str_to_lower` | `str_to_lower(s)` | Lowercase version of `s` |
| `str_to_upper` | `str_to_upper(s)` | Uppercase version of `s` |
| `str_trim` | `str_trim(s)` | Strip leading/trailing whitespace |
| `str_replace` | `str_replace(s, old, new)` | Replace first occurrence of `old` with `new` |
| `str_split` | `str_split(s, sep)` | Split `s` on separator, returns array |
| `str_join` | `str_join(arr, sep)` | Join array elements with separator |
| `str_lines` | `str_lines(s)` | Split `s` into lines (`"\n"` separator) |
| `str_words` | `str_words(s)` | Split `s` into words, trimming blanks |
| `str_title` | `str_title(s)` | Title-case: capitalize first letter of each word |
| `str_pad_left` | `str_pad_left(s, width, ch)` | Left-pad `s` to `width` with character `ch` |
| `str_pad_right` | `str_pad_right(s, width, ch)` | Right-pad `s` to `width` with character `ch` |
| `str_reverse` | `str_reverse(s)` | Reverse the characters of `s` |

### Example

```raven
import "stdlib/strings";

print(str_title("hello world"));     // Hello World
print(str_repeat("ab", 3));          // ababab
print(str_pad_left("42", 6, "0"));   // 000042
print(str_words("  foo   bar  "));   // ["foo", "bar"]
print(str_reverse("raven"));         // nevar
```

---

## `stdlib/collections`

**Import:** `import "stdlib/collections";`

Classic data structure implementations: Stack, Queue, and Set — all backed by Raven arrays and maps and returned as closured objects.

### Functions

| Function | Signature | Description |
|----------|-----------|-------------|
| `create_stack` | `create_stack()` | Create a new Stack object |
| `create_queue` | `create_queue()` | Create a new Queue object |
| `create_set` | `create_set()` | Create a new Set object |

### Stack Interface

| Method | Description |
|--------|-------------|
| `.push(val)` | Push a value onto the stack |
| `.pop()` | Remove and return the top value |
| `.peek()` | Return the top value without removing |
| `.size()` | Number of elements |
| `.is_empty()` | Returns `true` if empty |
| `.to_array()` | Returns underlying array |

### Queue Interface

| Method | Description |
|--------|-------------|
| `.enqueue(val)` | Enqueue a value at the back |
| `.dequeue()` | Dequeue and return the front value |
| `.peek()` | Return the front value without removing |
| `.size()` | Number of elements |
| `.is_empty()` | Returns `true` if empty |
| `.to_array()` | Returns underlying array |

### Set Interface

| Method | Description |
|--------|-------------|
| `.add(val)` | Add a value (keys are stringified) |
| `.has(val)` | Check membership |
| `.remove(val)` | Remove a value |
| `.size()` | Number of distinct elements |
| `.to_array()` | Return all elements as an array |

### Example

```raven
import "stdlib/collections";

let s = create_stack();
s.push(10);
s.push(20);
s.push(30);
print(s.pop());    // 30
print(s.peek());   // 20

let q = create_queue();
q.enqueue("a");
q.enqueue("b");
print(q.dequeue()); // a

let set = create_set();
set.add("x");
set.add("y");
set.add("x");
print(set.size());  // 2
```

---

## `stdlib/iter`

**Import:** `import "stdlib/iter";`

Iterator and sequence utilities for working with arrays. Depends on `stdlib/math`.

### Functions

| Function | Signature | Description |
|----------|-----------|-------------|
| `zip` | `zip(a, b)` | Zip two arrays into an array of `[a[i], b[i]]` pairs |
| `take` | `take(arr, count)` | Take the first `count` elements |
| `drop` | `drop(arr, count)` | Drop the first `count` elements |
| `chunk` | `chunk(arr, size)` | Split array into chunks of `size` |
| `flatten` | `flatten(matrix)` | Flatten one level of nested arrays |
| `any` | `any(arr, pred)` | Returns `true` if any element satisfies predicate |
| `all` | `all(arr, pred)` | Returns `true` if all elements satisfy predicate |
| `find` | `find(arr, pred)` | Return first element satisfying predicate, or `nil` |
| `reversed` | `reversed(arr)` | Return a reversed copy of the array |

### Example

```raven
import "stdlib/iter";

let pairs = zip([1, 2, 3], ["a", "b", "c"]);
// [[1, "a"], [2, "b"], [3, "c"]]

let chunks = chunk([1, 2, 3, 4, 5], 2);
// [[1, 2], [3, 4], [5]]

print(any([1, 2, 3], fn(x) { return x > 2; }));  // true
print(all([2, 4, 6], fn(x) { return x % 2 == 0; }));  // true

let first_even = find([1, 3, 4, 5], fn(x) { return x % 2 == 0; });
print(first_even);  // 4
```

---

## `stdlib/sort`

**Import:** `import "stdlib/sort";`

Sorting algorithms and array aggregation utilities.

### Functions

| Function | Signature | Description |
|----------|-----------|-------------|
| `bubble_sort` | `bubble_sort(arr)` | In-place bubble sort, returns sorted array |
| `insertion_sort` | `insertion_sort(arr)` | In-place insertion sort, returns sorted array |
| `min_of_arr` | `min_of_arr(arr)` | Minimum element of an array (or `nil` if empty) |
| `max_of_arr` | `max_of_arr(arr)` | Maximum element of an array (or `nil` if empty) |

### Example

```raven
import "stdlib/sort";

let nums = [5, 2, 8, 1, 9, 3];
bubble_sort(nums);
print(nums);          // [1, 2, 3, 5, 8, 9]

print(min_of_arr([3, 1, 4, 1, 5]));   // 1
print(max_of_arr([3, 1, 4, 1, 5]));   // 5
```

---

## `stdlib/errors`

**Import:** `import "stdlib/errors";`

Error creation and manipulation utilities, inspired by Go's `errors` package. Errors are represented as maps with `"message"` and `"is_error"` keys.

### Functions

| Function | Signature | Description |
|----------|-----------|-------------|
| `new_error` | `new_error(msg)` | Create a new error map from a message |
| `error_message` | `error_message(err)` | Extract the message string from an error |
| `is_error_val` | `is_error_val(err)` | Returns `true` if the value is an error map |
| `wrap_error` | `wrap_error(err, prefix)` | Wrap an existing error with an additional context prefix |

### Example

```raven
import "stdlib/errors";

fn divide(a, b) {
    if (b == 0) {
        return new_error("division by zero");
    }
    return a / b;
}

let result = divide(10, 0);
if (is_error_val(result)) {
    print("Error: " + error_message(result));
}

let wrapped = wrap_error(result, "math operation failed");
print(error_message(wrapped));
// "math operation failed: division by zero"
```

---

## `stdlib/testing`

**Import:** `import "stdlib/testing";`

A lightweight unit testing framework with suite grouping and assertion utilities, inspired by popular test libraries.

### Functions

| Function | Signature | Description |
|----------|-----------|-------------|
| `describe` | `describe(name, fn)` | Define a test suite |
| `test` | `test(name, fn)` | Define an individual test case |
| `expect_eq` | `expect_eq(actual, expected)` | Assert `actual == expected` (throws on failure) |
| `expect_true` | `expect_true(cond)` | Assert condition is `true` |
| `expect_false` | `expect_false(cond)` | Assert condition is `false` |
| `assert_equal` | `assert_equal(expected, actual)` | Alias for `expect_eq` (testify style) |
| `assert_not_equal` | `assert_not_equal(a, b)` | Assert `a != b` |
| `assert_true` | `assert_true(cond)` | Assert truthy |
| `assert_false` | `assert_false(cond)` | Assert falsy |
| `assert_nil` | `assert_nil(val)` | Assert value is `nil` |
| `assert_not_nil` | `assert_not_nil(val)` | Assert value is not `nil` |
| `assert_contains` | `assert_contains(container, elem)` | Assert `container` contains `elem` |

### Example

```raven
import "stdlib/testing";

describe("Math tests", fn() {
    test("addition", fn() {
        expect_eq(2 + 2, 4);
    });

    test("string contains", fn() {
        assert_contains("hello world", "world");
    });

    test("nil check", fn() {
        let x = nil;
        assert_nil(x);
    });
});
```

---

## `stdlib/flag`

**Import:** `import "stdlib/flag";`

Command-line flag parsing, inspired by Go's `flag` package. Supports `--name=value`, `--name` (boolean), and `-name` (short boolean flags).

### Functions

| Function | Signature | Description |
|----------|-----------|-------------|
| `flag_parse` | `flag_parse(args_list)` | Parse an array of argument strings into `{flags, args}` |
| `flag_string` | `flag_string(parsed, name, default)` | Get a string flag value, with a default |
| `flag_bool` | `flag_bool(parsed, name, default)` | Get a boolean flag value, with a default |
| `flag_int` | `flag_int(parsed, name, default)` | Get an integer flag value, with a default |

### Example

```raven
import "stdlib/flag";

let args = ["--output=report.txt", "--verbose", "input.csv"];
let parsed = flag_parse(args);

let output  = flag_string(parsed, "output", "out.txt");   // "report.txt"
let verbose = flag_bool(parsed, "verbose", false);         // true
let port    = flag_int(parsed, "port", 8080);             // 8080
let files   = parsed["args"];                              // ["input.csv"]

print(output);   // report.txt
print(verbose);  // true
```

---

## `stdlib/log`

**Import:** `import "stdlib/log";`

Structured leveled logging, inspired by Go's `log/slog` package. Default log level is `INFO (1)`.

### Log Levels

| Level | Value | Constant |
|-------|-------|----------|
| DEBUG | 0 | Verbose diagnostics |
| INFO  | 1 | Normal operational messages (default) |
| WARN  | 2 | Warnings |
| ERROR | 3 | Errors written to stderr |

### Functions

| Function | Signature | Description |
|----------|-----------|-------------|
| `set_log_level` | `set_log_level(level)` | Set the minimum log level (0–3) |
| `log_debug` | `log_debug(msg)` | Log at DEBUG level |
| `log_info` | `log_info(msg)` | Log at INFO level |
| `log_warn` | `log_warn(msg)` | Log at WARN level |
| `log_error` | `log_error(msg)` | Log at ERROR level (stderr) |
| `log_fatal` | `log_fatal(msg)` | Log at FATAL level (stderr) then `panic` |

### Example

```raven
import "stdlib/log";

set_log_level(0);  // enable DEBUG

log_debug("starting application");
log_info("server listening on :8080");
log_warn("disk usage above 80%");
log_error("failed to open config file");
```

---

## `stdlib/time`

**Import:** `import "stdlib/time";`

Time and duration utilities, inspired by Go's `time` package.

### Functions

| Function | Signature | Description |
|----------|-----------|-------------|
| `now_ms` | `now_ms()` | Current Unix time in milliseconds |
| `now_unix` | `now_unix()` | Current Unix time in seconds |
| `sleep_duration_ms` | `sleep_duration_ms(ms)` | Sleep for `ms` milliseconds |
| `sleep_seconds` | `sleep_seconds(secs)` | Sleep for `secs` seconds |
| `since_ms` | `since_ms(start_ms)` | Elapsed milliseconds since `start_ms` |
| `format_duration` | `format_duration(ms)` | Format milliseconds as `"Xms"`, `"Xs"`, or `"XmYs"` |

### Example

```raven
import "stdlib/time";

let start = now_ms();
sleep_seconds(1);
let elapsed = since_ms(start);
print(format_duration(elapsed));  // "1s" (approximately)

print(now_unix());  // e.g. 1730000000
```

---

## `stdlib/path`

**Import:** `import "stdlib/path";`

Filesystem path manipulation utilities, inspired by Go's `path/filepath` package.

### Functions

| Function | Signature | Description |
|----------|-----------|-------------|
| `path_join` | `path_join(parts)` | Join an array of path segments with `/` |
| `path_base` | `path_base(p)` | Return the last component of a path |
| `path_dir` | `path_dir(p)` | Return the directory portion of a path |
| `path_ext` | `path_ext(p)` | Return the file extension (including `.`) |
| `path_is_abs` | `path_is_abs(p)` | Returns `true` if path is absolute |

### Example

```raven
import "stdlib/path";

print(path_join(["home", "user", "file.txt"]));  // home/user/file.txt
print(path_base("/home/user/file.txt"));          // file.txt
print(path_dir("/home/user/file.txt"));           // /home/user
print(path_ext("report.pdf"));                    // .pdf
print(path_is_abs("/etc/hosts"));                 // true
print(path_is_abs("relative/path"));              // false
```

---

## `stdlib/context`

**Import:** `import "stdlib/context";`

Context objects for cancellation, deadlines, and request-scoped value propagation, inspired by Go's `context` package.

### Functions

| Function | Signature | Description |
|----------|-----------|-------------|
| `create_context` | `create_context()` | Create a new cancellable context object |

### Context Object Interface

| Method | Signature | Description |
|--------|-----------|-------------|
| `cancel` | `ctx.cancel()` | Cancel the context |
| `is_done` | `ctx.is_done()` | Returns `true` if cancelled or deadline exceeded |
| `set_deadline_ms` | `ctx.set_deadline_ms(ms)` | Set a deadline `ms` milliseconds from now |
| `set_value` | `ctx.set_value(k, v)` | Store a key-value pair in the context |
| `get_value` | `ctx.get_value(k)` | Retrieve a stored value by key |

### Example

```raven
import "stdlib/context";

let ctx = create_context();
ctx.set_value("user_id", 42);
ctx.set_deadline_ms(5000);  // 5 second timeout

// later...
if (ctx.is_done()) {
    print("context expired");
} else {
    let uid = ctx.get_value("user_id");
    print("processing request for user " + str(uid));
}

ctx.cancel();
print(ctx.is_done());  // true
```

---

## `stdlib/validate`

**Import:** `import "stdlib/validate";`

Data validation utilities for strings, numbers, and structured map schemas.

### Functions

| Function | Signature | Description |
|----------|-----------|-------------|
| `validate_string` | `validate_string(val, opts)` | Validate a string against `min_len`, `max_len`, `pattern` options |
| `validate_number` | `validate_number(val, opts)` | Validate a number against `min`, `max` options |
| `validate_schema` | `validate_schema(data, schema)` | Validate a map against a schema definition |

### Schema Format

Each key in the schema map is a field name; its value is a rule map with:

| Key | Type | Description |
|-----|------|-------------|
| `"required"` | bool | Field must be present |
| `"type"` | string | Expected type: `"string"`, `"number"`, `"bool"`, etc. |
| `"min_len"` | number | Minimum string length |
| `"max_len"` | number | Maximum string length |
| `"min"` | number | Minimum numeric value |
| `"max"` | number | Maximum numeric value |
| `"pattern"` | string | Regex pattern the string must match |

### Example

```raven
import "stdlib/validate";

let schema = {
    "name":  {"required": true, "type": "string", "min_len": 2},
    "age":   {"required": true, "type": "number", "min": 0, "max": 150},
    "email": {"required": false, "type": "string"}
};

let data = {"name": "A", "age": 25};

let errors = validate_schema(data, schema);
// ["name: string too short (min 2)"]

let str_err = validate_string("hi", {"min_len": 5});
// "string too short (min 5)"
```

---

## See Also

- [Standard Library Overview](overview.md)
- [Language Specification](../language/specification.md)
- [Architecture Overview](../architecture.md)
