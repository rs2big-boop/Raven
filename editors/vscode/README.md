# Raven VS Code Extension

Official Visual Studio Code extension providing syntax highlighting and language configuration for the **Raven Programming Language** (`.rvn`).

## Features
- **Syntax Highlighting**: Keywords (`let`, `fn`, `match`, `import`), pipeline operators (`|>`), strings, functions, numbers, and built-ins.
- **Bracket Matching & Auto-closing**: Auto-close pairs for `{ }`, `[ ]`, `( )`, and `" "`.
- **Comment Toggling**: Quick line comments with `//`.

## Installation
Copy this folder to your VS Code extensions directory:
```bash
cp -r editors/vscode ~/.vscode/extensions/raven-lang
```
Or package with `vsce package` and install the resulting `.vsix`.
