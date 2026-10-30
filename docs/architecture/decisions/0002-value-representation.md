# ADR 0002: Dynamic Value Representation via C++20 `std::variant`

- **Status**: ACCEPTED [IMPLEMENTED]
- **Date**: 2026-10-28
- **Author**: Raven Language Core Architecture Team

## Context
A dynamic or gradually typed runtime requires a uniform container (`Value`) capable of storing diverse runtime types: primitive numbers, booleans, nil, strings, arrays, maps, and callable objects (functions, closures, native builtins).

Options considered:
1. **Classic Object Hierarchy**: Abstract base class `Object*` with dynamic downcasting (`dynamic_cast`). High heap allocation rate and pointer dereferencing overhead for simple numbers and booleans.
2. **NaN-Boxing**: Packing 64-bit IEEE-754 floats with pointer payloads into unused quiet-NaN bit patterns. High performance, but platform-specific endianness dependencies and limits object references to 48-bit address space.
3. **Type-Safe Tagged Union (`std::variant`)**: Standard C++20 `std::variant` holding primitive scalars inline and reference-counted heap pointers (`std::shared_ptr`) for composite data structures.

## Decision
Raven adopts `std::variant` for runtime values:
```cpp
using ArrayPtr = std::shared_ptr<std::vector<Value>>;
using MapPtr   = std::shared_ptr<std::unordered_map<std::string, Value>>;

using ValueBase = std::variant<
    Nil,
    bool,
    double,
    std::string,
    std::shared_ptr<Callable>,
    ArrayPtr,
    MapPtr
>;
```

## Consequences

### Positive
- Strict C++20 type safety with compile-time index matching (`std::holds_alternative`, `std::get`).
- Zero manual bit manipulation, highly portable across x86_64, ARM64, and WebAssembly.
- Immediate value semantics for numbers, booleans, and nil without heap allocation.
- Reference counting for collections ensures deterministic cleanup upon scope termination.

### Negative / Trade-offs
- `sizeof(Value)` is ~32 bytes (variant overhead + string / shared_ptr size), which is larger than 8-byte NaN-boxed representations.
- Deep nested structures rely on `std::shared_ptr` which introduces atomic increment/decrement overhead.
