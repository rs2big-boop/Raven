# ADR 0007: Phased Hybrid Memory Management & Cycle Collection

- **Status**: ACCEPTED [IN PROGRESS / DESIGN PHASE]
- **Date**: 2026-10-30
- **Author**: Raven Language Core Architecture Team

## Context
Raven balances the rapid developer ergonomics of dynamic scripting languages with the explicit concurrency and runtime predictability of modern systems languages. As applications scale to long-running services, recursive closures, and complex cyclic graph structures (e.g., doubly linked nodes or mutual map references), simple reference counting leaves circular references uncollected.

Conversely, adopting a monolithic stop-the-world tracing garbage collector would degrade prompt RAII resource cleanup, complicate integration with C++20 concurrency primitives, and introduce latency jitter.

## Decision
Raven formally adopts a **Phased Hybrid Memory Management Model**:
1. **Primary Lifetime Management (Reference Counting)**:
   - Everyday object lifespans are governed by reference counts (`std::shared_ptr` in Phase 1, transitioning to a dedicated Raven Managed Heap Header in Phase 2).
   - Instantaneous, deterministic deallocation occurs the moment an object's reference count drops to zero.
2. **Auxiliary Cycle Reclamation (Tracing Cycle Collector)**:
   - An independent cycle detection pass runs asynchronously or at scheduled safe points without stopping the world for normal allocations.
   - Live objects are identified via a **Root Registry**: Global environment, Interpreter evaluation stack, VM operand stack, active closures, and spawned task contexts.
   - Candidate containers (Maps, Arrays, Closures) with non-zero reference counts that become unreachable from roots are identified and reclaimed.
3. **Deterministic System Resource Management (RAII)**:
   - Operating system resources (file descriptors, sockets, POSIX synchronization locks, channels) are tied to native C++ object destructors and never deferred to garbage collection cycles.
4. **Explicit Cross-Thread Boundaries**:
   - Reference-count synchronization (thread safety of the counters) is strictly decoupled from object mutation safety (safeguarded by channels and mutexes).

## Architectural Hierarchy
```
Raven Runtime
│
├── Managed Heap
│   ├── Object Header
│   ├── Reference Count
│   ├── Object Type
│   └── Object Metadata
│
├── Memory Manager
│   ├── Allocate
│   ├── Retain
│   ├── Release
│   └── Collect Cycles
│
├── Root Registry
│   ├── Global Variables
│   ├── Interpreter Stack
│   ├── VM Stack
│   ├── Closures
│   └── Active Tasks
│
└── Cycle Collector
    ├── Root Tracing
    ├── Reachability Analysis
    └── Unreachable Cycle Cleanup
```

## Phased Implementation Plan
- **Phase 1 (v1.0.0 — COMPLETED)**: C++20 `std::shared_ptr` reference counting for composite values (`Array`, `Map`, `Callable`, `Environment`), inline stack values for scalars, RAII for OS handles.
- **Phase 2 (v1.1.0 — IN PROGRESS / RESEARCH)**: Introduce Root Registry interface and experimental cycle detection on isolated subgraphs.
- **Phase 3 (v1.2.0 — PLANNED)**: Unify heap headers under custom Raven runtime allocator to eliminate `std::shared_ptr` control block overhead.

## Consequences

### Positive
- Prompt, deterministic destruction for 99%+ of objects without stop-the-world pauses.
- Complete cyclic reference safety without forcing Rust-like borrow checker ceremony on scripting developers.
- Clean separation between memory deallocation and concurrency safety.

### Trade-offs
- Requires maintaining the Root Registry across both the Tree-Walk Interpreter and Corvus VM.
- Cycle detector must carefully avoid race conditions when traversing containers accessed by background worker threads.
