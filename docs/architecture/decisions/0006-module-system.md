# ADR 0006: Module Resolution and Package Management Architecture

- **Status**: ACCEPTED [IMPLEMENTED]
- **Date**: 2026-10-29
- **Author**: Raven Language Core Architecture Team

## Context
Code reuse and third-party dependency management require a clear, predictable module resolution algorithm and package management toolchain.

## Decision
Raven implements:
1. **Module Import Resolution**:
   - Syntax: `import "<module-path>";`
   - Prefixes starting with `stdlib/` search standard library directories:
     1. Current working directory `./stdlib/`
     2. Parent `./../stdlib/`
     3. Dedicated `/home/ran/Raven/stdlib/`
     4. System installation `/usr/local/share/raven/stdlib/`
     5. Executable-relative path `../share/raven/stdlib/`
   - Local relative paths `import "./my_file";` search relative to the calling source file.
   - External dependencies `import "package-name/submodule"` search `raven_modules/`.
2. **Package Tooling**:
   - `raven init [name]`: Generates a `raven.json` package manifest.
   - `raven pkg install <repo>[@tag]`: Clones the repository into `raven_modules/` and locks the exact commit in `raven.lock`.
   - `raven pkg list`: Displays all installed packages and versions.

## Consequences

### Positive
- Zero external package manager needed; fully built into the `raven` binary.
- Deterministic builds via `raven.lock` cryptographic/tag tracking.
- Standard library modules are directly importable without installation.

### Negative / Trade-offs
- Package downloads currently rely on system `git clone`. A centralized package registry API is planned for future versions.
