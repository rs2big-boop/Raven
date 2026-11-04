# Raven Language for VS Code / Cursor

Official editor extension for the **Raven Programming Language** (`.rvn`).

Modeled after the [C/C++ extension](https://github.com/microsoft/vscode-cpptools) UX:

- IntelliSense via the built-in `raven lsp` engine
- Error squiggles, hover, completion, format-on-save
- Status bar (toolchain + IntelliSense health)
- Run / Debug / Build-and-Run title actions
- Code analysis (`raven vet`), docs, benchmarks
- Configuration UI + executable picker

## Requirements

A Raven toolchain on `PATH`, or set **Raven: Select Raven Executable**.

Typical install locations:

- Windows: `%USERPROFILE%\.raven\bin\raven.exe`
- Linux/macOS: `~/.local/bin/raven` or `/usr/local/bin/raven`

## Features

| Feature | Command / Setting |
|---|---|
| Run file | `Raven: Run File` / title play button |
| Debug | `F5` / `Raven: Debug File` |
| Build + run | `Raven: Build and Run File` |
| Format | Format Document / format on save |
| Analysis | `Raven: Run Code Analysis on Active File` |
| Restart IntelliSense | `Raven: Restart IntelliSense for Active File` |
| Pick binary | `Raven: Select Raven Executable` |
| Squiggles | `raven.errorSquiggles` |

## Develop

```bash
cd editors/vscode
npm install
npm run compile
```

Then press **F5** in VS Code with this folder open, or copy the folder into your extensions directory (must include `out/`).

## Install from this repo

```bash
# Linux / macOS
./scripts/install_vscode_ext.sh

# Windows (PowerShell)
./scripts/install_vscode_ext.ps1
```

The main Raven installers (`install.sh` / `install.ps1`) also install this extension into VS Code and Cursor when present.
