# MiniLang Language Specification

## Tokens & Lexical Rules (Phase 1)

### Keywords
- `let`: Variable declaration keyword.

### Identifiers
- Format: `[a-zA-Z_][a-zA-Z0-9_]*`
- Must not collide with reserved keywords.

### Numbers
- Format: `[0-9]+`
- Currently supports non-negative integers.

### Operators and Punctuation
- `+` : Addition
- `-` : Subtraction
- `*` : Multiplication
- `/` : Division
- `=` : Variable assignment
- `;` : Statement terminator

### Whitespace
- Spaces, tabs (`\t`), and carriage returns (`\r`) are ignored.
- Newlines (`\n`) increment the line number and reset the column counter to 1.
