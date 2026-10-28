# Raven Language Specification

## File Extension
- Raven source files use the `.rvn` extension (e.g., `main.rvn`).

## Formal Grammar (EBNF)

```ebnf
program        -> declaration* EOF ;

declaration    -> funDecl
                | varDecl
                | statement ;

funDecl        -> "fn" IDENTIFIER "(" parameters? ")" block ;
parameters     -> IDENTIFIER ( "," IDENTIFIER )* ;

varDecl        -> "let" IDENTIFIER ( "=" expression )? ";" ;

statement      -> exprStmt
                | printStmt
                | ifStmt
                | whileStmt
                | forInStmt
                | tryCatchStmt
                | throwStmt
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
                | "(" expression ")" ;
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

## Built-in Standard Library Functions

- `len(collection)`: Length of string, array, or map.
- `push(array, item)`: Appends an item to an array.
- `pop(array)`: Removes and returns the last element of an array.
- `keys(map)`: Returns an array of keys in a map.
- `values(map)`: Returns an array of values in a map.
- `range(start, end)`: Returns an array of numbers `[start, ..., end - 1]`.
- `map(array, fn)`: Returns a new array with `fn` applied to each element.
- `filter(array, fn)`: Returns a new array of elements that satisfy `fn`.
- `reduce(array, fn, initial)`: Folds an array with `fn`.
- `type(val)`: Returns the type name as a string.
- `time()`: Returns Unix time in milliseconds.
- `assert(condition, message)`: Asserts condition is truthy.
- `str(val)`: Converts value to string.
- `num(val)`: Converts value to number.
