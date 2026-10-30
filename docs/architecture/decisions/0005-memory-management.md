# ADR 0005: Memory Management Architecture

- **Status**: ACCEPTED [IMPLEMENTED] (with Planned Tracing GC)
- **Date**: 2026-10-30
- **Author**: Raven Language Core Architecture Team

## Context
Automatic memory management is essential for a productive high-level language. Three primary paradigms exist:
1. **Tracing Garbage Collector (Stop-The-World or Concurrent)**: High complexity, handles cyclic data structures naturally, but introduces non-deterministic pause times.
2. **Deterministic Reference Counting (`std::shared_ptr`)**: Predictable prompt deallocation upon last owner destruction, low runtime implementation complexity, but vulnerable to cycles.
3. **Region / Arena Allocation**: Extremely fast bulk allocation and deallocation, but strictly tied to bounded lifetimes.

## Decision
Raven implements a **Hybrid Memory Architecture**:
1. **Core Runtime [IMPLEMENTED]**:
   - Reference Counting via C++20 `std::shared_ptr` for composite objects (`Array`, `Map`, `Callable`, `Environment`).
   - Scalar values (`Number`, `Boolean`, `Nil`) are allocated entirely on the stack within `Value` variants with zero heap overhead.
   - Deterministic resource cleanup using RAII: file descriptors, locks, and channel buffers are destroyed as soon as their enclosing environment goes out of scope.
2. **Arena Allocator [IMPLEMENTED via `stdlib/arena`]**:
   - High-throughput batch allocation for short-lived workflows (compilation passes, request contexts) with bulk `.reset()`.
3. **Cycle Collection [PLANNED for v1.1.0]**:
   - Cycle detection pass using Mark-and-Sweep on global and closure environment graphs.

## Consequences

### Positive
- Immediate destruction of resources (file handles, channels) without waiting for a GC cycle.
- Predictable execution latency suitable for real-time and CLI tooling.
- Simple, thread-safe memory cleanup leveraging standard library atomic refcounts.

### Negative / Trade-offs
- Circular object references (e.g., node A holding node B, holding node A) remain in memory until process termination unless manually cleared or collected by the planned cycle collector.
