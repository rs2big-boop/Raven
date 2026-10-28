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
                | returnStmt
                | block ;

exprStmt       -> expression ";" ;
printStmt      -> "print" ( "(" expression ")" | expression ) ";" ;
ifStmt         -> "if" "(" expression ")" statement ( "else" statement )? ;
whileStmt      -> "while" "(" expression ")" statement ;
returnStmt     -> "return" expression? ";" ;
block          -> "{" declaration* "}" ;

expression     -> assignment ;
assignment     -> IDENTIFIER "=" assignment
                | logic_or ;
logic_or       -> logic_and ( "||" logic_and )* ;
logic_and      -> equality ( "&&" equality )* ;
equality       -> comparison ( ( "!=" | "==" ) comparison )* ;
comparison     -> term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
term           -> factor ( ( "-" | "+" ) factor )* ;
factor         -> unary ( ( "/" | "*" | "%" ) unary )* ;
unary          -> ( "!" | "-" ) unary
                | call ;
call           -> primary ( "(" arguments? ")" )* ;
arguments      -> expression ( "," expression )* ;
primary        -> NUMBER | STRING | "true" | "false"
                | IDENTIFIER
                | "(" expression ")" ;
```

## Data Types

| Type | Examples | Description |
|---|---|---|
| `Number` | `0`, `42`, `3.14` | IEEE 754 64-bit floating-point numbers |
| `String` | `"Ran"`, `"Hello\nWorld"` | UTF-8 string literals with escape sequences |
| `Boolean` | `true`, `false` | Boolean values |
| `Nil` | uninitialized variables | Represents absence of a value |
| `Function` | `fn add(a, b) { ... }` | First-class callable objects |

## Control Flow & Scopes
- Block scopes `{ ... }` support variable shadowing.
- `if (condition) { ... } else { ... }` executes conditionally based on truthiness.
- `while (condition) { ... }` loops while condition is truthy.
- `fn` definitions support recursion and lexical closures.
