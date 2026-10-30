# Corvus Virtual Machine Architecture

Corvus is Raven's high-performance, stack-based bytecode virtual machine. It interprets `.rvc` bytecode chunks with minimal memory footprint and zero AST overhead.

---

## 1. Virtual Machine Design

Corvus utilizes an explicit value evaluation stack and activation frame array (`CallFrame`):

```cpp
struct CallFrame {
    size_t return_ip;
    size_t slots;       // Base offset into stack_ for this frame's locals
};
```

---

## 2. OpCode Set Reference

| OpCode | Operands | Stack Effect | Description |
|---|---|---|---|
| `Constant` | `<index:uint8>` | `-> val` | Push constant from chunk pool |
| `Nil` | None | `-> nil` | Push nil literal |
| `True` / `False` | None | `-> bool` | Push boolean literal |
| `Pop` | None | `val ->` | Discard top of stack |
| `GetLocal` / `SetLocal` | `<slot:uint8>` | `[slot]` | Read/write local variable slot |
| `GetGlobal` / `SetGlobal` | `<idx:uint8>` | `[name]` | Read/write global variable table |
| `Add`, `Subtract`, `Multiply`, `Divide`, `Modulo` | None | `a, b -> (a op b)` | Binary arithmetic |
| `Equal`, `Greater`, `Less` | None | `a, b -> bool` | Comparison operators |
| `Not`, `Negate` | None | `a -> (!a)` | Unary operators |
| `Jump`, `JumpIfFalse` | `<offset:uint16>` | Control | Direct and conditional branch |
| `JumpIfNotNil` | `<offset:uint16>` | Peek & branch | Efficient null coalescing evaluation |
| `CheckType` | `<idx:uint8>` | `val -> bool` | Runtime type check (`is` operator) |
| `Call` | `<arg_count:uint8>` | `args..., fn -> res` | Invoke callable object |
| `Return` | None | Control | Pop call frame and push result |
| `MakeArray` | `<count:uint8>` | `items... -> arr` | Allocate array from stack values |
| `MakeMap` | `<count:uint8>` | `k1, v1... -> map` | Allocate map from stack entries |
| `GetIndex` / `SetIndex` | None | `obj, idx -> val` | Array and map indexing |

---

## 3. Bytecode Verification

Prior to execution, Corvus optionally passes chunks through `BytecodeVerifier` (`include/raven/bytecode_verifier.hpp`):
- Verifies jump targets are within legal chunk boundaries.
- Confirms constant pool indices do not exceed vector capacity.
- Checks stack underflow invariants.
