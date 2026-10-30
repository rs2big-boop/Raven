# ADR 0003: Communicating Sequential Processes (CSP) Concurrency Model

- **Status**: ACCEPTED [IMPLEMENTED]
- **Date**: 2026-10-29
- **Author**: Raven Language Core Architecture Team

## Context
Concurrency models in modern programming languages broadly fall into three categories:
1. **Shared-Memory Multi-Threading with Mutexes** (Java / C++): Error-prone, race conditions, complex deadlocks.
2. **Single-Threaded Event Loop with Async/Await** (Node.js / Python asyncio): Simple, but cannot utilize multi-core CPUs without multi-process clustering.
3. **Communicating Sequential Processes (CSP)** (Go / Erlang): Independent execution threads communicating via message passing over channels, without shared mutable state.

## Decision
Raven implements native **CSP Concurrency**:
- `spawn(callable)`: Launches an asynchronous execution on a dedicated OS thread.
- Thread-safe channels (`NativeChannel`): Thread-safe FIFO message queues protected by `std::mutex` and signaled with `std::condition_variable`.
- High-level `stdlib/sync` module providing:
  - `go(fn)`: Ergonomic task spawner.
  - `create_channel(capacity)`: Buffered and unbuffered channels with `.send()`, `.recv()`, `.close()`, `.len()`.
  - `create_wait_group()`: Deterministic synchronization barrier (`.add()`, `.done()`, `.wait()`).
  - `create_mutex()`: Reentrant mutual exclusion lock for critical sections.
  - `create_once()`: Idempotent initialization primitive.

## Consequences

### Positive
- True multi-core parallelism across all CPU cores.
- Predictable message passing semantics minimizing data race hazards.
- Familiar, proven developer ergonomics inspired by the Go concurrency model.

### Negative / Trade-offs
- Each `spawn()` invokes a native POSIX thread (`std::thread`), which has higher memory overhead than M:N green threads (goroutines).
- Unbounded thread creation can exhaust OS thread limits; future roadmap specifies an M:N fiber scheduler.
