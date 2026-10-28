# MiniLang Architecture

This document describes the architectural design and roadmap for MiniLang.

```
+-----------------------------------------------------------+
|                       Source Code                         |
|                   (e.g., main.mini)                       |
+-----------------------------+-----------------------------+
                              |
                              v
+-----------------------------------------------------------+
|                          Lexer                            |
|             Converts raw text into Token stream           |
+-----------------------------+-----------------------------+
                              |
                              v
+-----------------------------------------------------------+
|                         Parser                            |
|        Validates syntax and builds the Abstract           |
|                  Syntax Tree (AST)                        |
+-----------------------------+-----------------------------+
                              |
                              v
+-----------------------------------------------------------+
|                      Interpreter                          |
|         Evaluates expressions, manages scopes             |
|                 and executes statements                   |
+-----------------------------------------------------------+
```

## Stage 1: The Lexer (Lexical Analysis)

The Lexer's job is to scan the raw source characters linearly and group them into logical units called **Tokens**.

### Responsibilities:
1. **Tokenization**: Classifying sequences of characters into keywords (`let`), identifiers (`age`), literals (numbers like `20`), operators (`+`, `-`, `*`, `/`, `=`), and delimiters (`;`).
2. **Whitespace and Comment Stripping**: Filtering out meaningless whitespace (spaces, tabs, carriage returns) while keeping track of line and column counters.
3. **Source Location Tracking**: Storing the 1-based `line` and `column` for each token so subsequent stages (Parser, Interpreter) can report clear, user-friendly compiler errors.
4. **Invalid Character Reporting**: Instead of crashing on unexpected characters, the lexer yields `TokenType::Invalid` tokens, allowing the diagnostic pipeline to aggregate errors gracefully.
