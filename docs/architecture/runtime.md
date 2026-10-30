# Raven Runtime Architecture

The Raven Runtime encapsulates global system services, memory management, CSP concurrency primitives, and OS system interfaces shared across execution engines.

---

## 1. Concurrency Model (CSP)

Raven implements Communicating Sequential Processes (CSP) modeled after Go:
- **`spawn(fn)`**: Launches a callable inside an OS managed thread registered with `SpawnRegistry`.
- **Channel Registry**: Synchronized thread-safe channels (`NativeChannel`) with condition variables (`cv_recv`, `cv_send`) supporting buffered and unbuffered queues.
- **WaitGroups & Mutexes**: Native synchronization primitives exposed to Raven code via IDs.

---

## 2. Memory Management (ADR-0007)

Raven adopts a phased Hybrid Memory Strategy:
1. **Immediate Reclamation**: Reference counting via `std::shared_ptr` on `ArrayPtr` and `MapPtr`. Unshared or acyclic objects are destroyed immediately upon exiting scope.
2. **Cycle Collector Prototype (`include/raven/gc.hpp`)**:
   - `GCRegistry` tracks candidate object groups and GC roots (`ScopedRoot`).
   - `gc_collect()` runs a mark-and-sweep cycle detection pass to break unreachable reference cycles.
   - Built-in diagnostics available via `gc_stats()`.
3. **Graceful Shutdown**: `SpawnRegistry` ensures spawned background tasks unwind cleanly without memory faults or unhandled OS aborts.

---

## 3. Module Resolution

When an `import "path/to/mod"` statement is evaluated:
1. Checks the module cache (`imported_modules_`) to avoid duplicate execution.
2. Searches in precedence order:
   - Relative to currently executing source file.
   - Standard library root directory (`stdlib/`).
   - Installation system paths.
3. Parses and executes the module within the shared global environment.
