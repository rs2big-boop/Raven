# Raven Language Specification

## File Extension
- Raven source files use the `.rvn` extension (e.g., `main.rvn`).

## Formal Grammar (EBNF)

```ebnf
program        -> declaration* EOF ;

declaration    -> funDecl
                | varDecl
                | importStmt
                | statement ;

funDecl        -> "fn" IDENTIFIER "(" parameters? ")" block ;
parameters     -> IDENTIFIER ( "," IDENTIFIER )* ;

varDecl        -> "let" IDENTIFIER ( "=" expression )? ";" ;
importStmt     -> "import" ( STRING | IDENTIFIER ) ";" ;

statement      -> exprStmt
                | printStmt
                | ifStmt
                | whileStmt
                | forInStmt
                | tryCatchStmt
                | throwStmt
                | importStmt
                | returnStmt
                | block ;

exprStmt       -> expression ";" ;
printStmt      -> "print" ( "(" expression ")" | expression ) ";" ;
ifStmt         -> "if" "(" expression ")" statement ( "else" statement )? ;
whileStmt      -> "while" "(" expression ")" statement ;
forInStmt      -> "for" "(" "let"? IDENTIFIER "in" expression ")" statement ;
tryCatchStmt   -> "try" statement "catch" "(" IDENTIFIER ")" statement ;
throwStmt      -> "throw" expression ";" ;
returnStmt     -> "return" expression? ";" ;
block          -> "{" declaration* "}" ;

expression     -> assignment ;
assignment     -> ( IDENTIFIER | indexExpr | propertyExpr ) "=" assignment
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

primary        -> NUMBER | STRING | "true" | "false"
                | IDENTIFIER
                | "[" ( expression ( "," expression )* )? "]"
                | "{" ( expression ":" expression ( "," expression ":" expression )* )? "}"
                | "fn" "(" parameters? ")" block
                | "match" "("? expression ")"? "{" matchArm* "}"
                | "(" expression ")" ;

matchArm       -> ( expression | "_" ) "=>" expression ( "," | ";" )? ;
```

## Data Types

| Type | Examples | Description |
|---|---|---|
| `Number` | `0`, `42`, `3.14` | IEEE 754 64-bit floating-point numbers |
| `String` | `"Ran"`, `"Hello\nWorld"` | UTF-8 string literals with escape sequences |
| `Boolean` | `true`, `false` | Boolean values |
| `Array` | `[1, 2, "three", [4]]` | Dynamic mutable arrays with negative indexing |
| `Map` | `{"name": "Ran", "age": 20}` | Hash map dictionaries supporting `.prop` and `["key"]` |
| `Nil` | uninitialized variables | Absence of value |
| `Function` | `fn(x) { ... }`, named functions | First-class callable objects with closures |

## Built-in Functions

### Collections & Sequence Functions
- `len(collection)`: Length of string, array, or map.
- `push(array, item)`: Appends an item to an array.
- `pop(array)`: Removes and returns the last element of an array.
- `keys(map)`: Returns an array of keys in a map.
- `values(map)`: Returns an array of values in a map.
- `range(start, end)`: Returns an array of numbers `[start, ..., end - 1]`.
- `map(array, fn)`: Returns a new array with `fn` applied to each element.
- `filter(array, fn)`: Returns a new array of elements that satisfy `fn`.
- `reduce(array, fn, initial)`: Folds an array with `fn`.
- `contains(haystack, needle)`: Checks if string, array, or map contains element.
- `index_of(haystack, needle)`: Returns index of element/substring or -1.

### Math Functions
- `min(a, b)`, `max(a, b)`: Minimum and maximum of two numbers.
- `sqrt(x)`: Square root.
- `sin(x)`, `cos(x)`, `tan(x)`: Trigonometric functions.
- `floor(x)`, `ceil(x)`, `round(x)`: Number rounding.
- `log(x)`, `exp(x)`: Natural logarithm and exponential.
- `rand()`: Random floating point number between 0.0 and 1.0.
- `rand_int(min, max)`: Uniform random integer between min and max.

### String Processing
- `split(str, delim)`: Splits string by delimiter into an array of strings.
- `join(arr, delim)`: Joins elements of an array with a delimiter string.
- `to_upper(str)`: Converts string to uppercase.
- `to_lower(str)`: Converts string to lowercase.
- `trim(str)`: Strips leading and trailing whitespace.
- `replace(str, old_sub, new_sub)`: Replaces occurrences of substring.

### File I/O & System
- `read_file(path)`: Reads entire file as string.
- `write_file(path, content)`: Writes string to file (returns bool).
- `file_exists(path)`: Checks if file exists on disk.
- `sleep(ms)`: Pauses execution for milliseconds.
- `env(name)`: Retrieves environment variable value.
- `exec(cmd)`: Executes a shell command and captures its standard output.

### JSON Serialization
- `to_json(val)`: Serializes Raven data into a JSON string.
- `parse_json(str)`: Parses a JSON string into Raven maps, arrays, numbers, and strings.

### Introspection & Utilities
- `type(val)`: Returns the type name as a string (`"number"`, `"string"`, `"array"`, `"map"`, `"bool"`, `"nil"`, `"function"`).
- `time()`: Returns Unix time in milliseconds.
- `assert(condition, message)`: Asserts condition is truthy.
- `str(val)`: Converts value to string.
- `num(val)`: Converts value to number.

---

## Standard Library Modules (`stdlib/`)

- `import "stdlib/math";` — Constants (`PI`, `E`), `deg_to_rad`, `rad_to_deg`, `hypot`, statistics (`sum`, `mean`, `variance`, `std_dev`).
- `import "stdlib/collections";` — Object-oriented data structures: `create_stack()`, `create_queue()`, `create_set()`.
- `import "stdlib/iter";` — Sequence helpers: `zip`, `take`, `drop`, `chunk`, `flatten`, `any`, `all`, `find`, `reversed`.
- `import "stdlib/string";` — String helpers: `repeat`, `pad_left`, `pad_right`.
- `import "stdlib/testing";` — Unit test suite runner: `describe`, `test`, `expect_eq`, `expect_true`, `expect_false`.
