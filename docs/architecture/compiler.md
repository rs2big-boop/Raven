# Bytecode Compiler Architecture

The Raven Compiler translates AST representations into linear bytecode chunks executed by the Corvus Virtual Machine or serialized to `.rvc` binary artifacts.

---

## 1. Compilation Pipeline

```
Raven Source (.rvn)
        │
        ▼
   Lexer & Parser
        │
        ▼
  Raven AST Root
        │
        ▼
Bytecode Compiler (`Compiler::compile`)
        │
        ▼
BytecodeChunk (Code + Constants + Debug Lines)
   ┌────┴──────────────────────────┐
   ▼                               ▼
In-Memory VM Execution     Serialized .rvc File
(`Corvus VM`)             (`BytecodeSerializer`)
```

---

## 2. Bytecode Chunk Structure

A `BytecodeChunk` contains:
1. `std::vector<uint8_t> code`: Linear sequence of opcodes and immediate byte operands.
2. `std::vector<Value> constants`: Pool of literals (strings, floats, callable functions).
3. `std::vector<size_t> lines`: Line-number mappings for runtime error reporting and stack traces.

---

## 3. Scope Resolution & Local Slots

Variables within functions are resolved to static stack slots rather than runtime hash lookups:
- `OpCode::GetLocal <slot>`
- `OpCode::SetLocal <slot>`

Top-level global variables use indexed constant table names:
- `OpCode::DefineGlobal <const_idx>`
- `OpCode::GetGlobal <const_idx>`
- `OpCode::SetGlobal <const_idx>`

---

## 4. Control Flow & Jump Patching

Control structures (`if`, `while`, `??`, `||`, `&&`) use backpatching:
1. Emit conditional jump with dummy operand `0x0000`.
2. Compile condition and block statements.
3. Patch the target jump offset to point to current instruction pointer.

Opcodes involved:
- `OpCode::Jump <offset>`
- `OpCode::JumpIfFalse <offset>`
- `OpCode::JumpIfNotNil <offset>` (specifically for `??` null coalescing)
- `OpCode::Loop <offset>`
