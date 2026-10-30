# ADR 0004: `.rvc` Binary Bytecode File Format Specification

- **Status**: ACCEPTED [IMPLEMENTED]
- **Date**: 2026-10-28
- **Author**: Raven Language Core Architecture Team

## Context
Compiled Raven programs require a binary serialization format that can be written by `ravenc` and executed by `rvm` with minimal parsing overhead and complete integrity validation.

## Decision
Raven specifies the `.rvc` (Raven Bytecode) format:
- **Magic Bytes**: 4 bytes `0x52 0x56 0x4E 0x43` ("RVNC").
- **Format Version**: 4 bytes little-endian integer (`uint32_t`).
- **Constant Pool**: Table of numbers, strings, booleans, and bytecode functions.
  - Count: `uint32_t`.
  - Type Tag: `uint8_t` (0=Nil, 1=Bool, 2=Number, 3=String, 4=BytecodeFunction).
- **Bytecode Stream**: Sequence of 1-byte opcodes followed by variable-length operands (e.g. constant pool indices, jump offsets, local slot numbers).
- **Line Number Table**: Run-length encoded instruction-to-source-line mapping for stack traces.

## Consequences

### Positive
- Compact binary file footprint.
- Fast loading directly into memory without AST reparsing.
- Magic bytes and version checking prevent execution of incompatible or corrupted files.

### Negative / Trade-offs
- Format is stack-based; migration to register-based VM in future versions will require format version increment (v2).
