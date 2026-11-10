# ADR 0008.1: Ownership, Root Tracking & Cleanup Semantics (Hardened Specification)

- **Status**: ACCEPTED [Engineering Contract & Implementation Specification]
- **Date**: 2026-11-10
- **Version**: 1.1 (Revises and hardens ADR 0008)
- **Scope**: Compiler, Corvus VM, Tree-Walk Interpreter, Memory Allocator, Garbage Collector

---

## 1. Specification Maturity & Status Notice

This document establishes the binding implementation contract for Raven's memory, resource, and concurrency model.

```
Target Architecture: Generational Tracing GC + Exact Stack Maps + Move-Only Resources + RAII Cleanup Stacks + Arc Root Registration
Current Implementation Status: Tracked in Section 10. Features marked [PLANNED] or [IN-PROGRESS] must not be advertised as production-ready until verified under ASan/TSan.
```

---

## 2. Uniquely Owned Resources & Ownership Invariants

### 2.1 The Non-Copyable Resource Invariant
Any type encapsulating an operating system resource (`File`, `Socket`, `MutexGuard`, `DBConnection`) is **Uniquely Owned** and **Move-Only**.

```
Let R be a Uniquely Owned Resource.
1. Copy Prohibition: Assignment `let alias = R` is a COMPILE-TIME ERROR.
2. Explicit Transfer: Ownership is transferred solely via `let R2 = move R;`.
3. Invalidation: Once moved, the identifier `R` enters the `Moved` (Tombstone) state. Any subsequent access is rejected at compile-time (or throws `UseAfterMoveError` if dynamically typed).
4. Scoped Borrowing: Temporary usage is granted via explicit borrow references (`&R`). Borrows are strictly bounded by lexical lifetime and CANNOT escape or be returned.
```

```raven
// Example: Valid Ownership Transfer & Borrowing
using file = File.open("report.bin") {
    // let alias = file;       // COMPILE ERROR: Cannot copy uniquely owned resource 'File'
    read_header(&file);         // VALID: Borrowed reference, strictly scope-limited
    let file2 = move file;      // VALID: Explicit transfer
    // file.read();             // COMPILE ERROR: 'file' used after move
    file2.read();               // VALID: 'file2' is the sole active owner
}
```

### 2.2 Atomic Resource Control Block (`ResourceControlBlock`)
Every native resource is governed by an atomic state machine in C++:

```cpp
enum class ResourceState : uint8_t {
    Open,
    Closing,
    Closed,
    Moved
};

struct ResourceControlBlock {
    std::atomic<ResourceState> state{ResourceState::Open};
    std::atomic<int32_t> active_borrows{0};
    int native_fd{-1};
    std::mutex cleanup_mtx;

    bool try_transition(ResourceState expected, ResourceState desired) {
        return state.compare_exchange_strong(expected, desired,
                                            std::memory_order_acq_rel);
    }
};
```

**State Invariants:**
1. **Deterministic Close**: Exactly **one** thread successfully transitions `state` from `Open -> Closing -> Closed` and executes `close(native_fd)`.
2. **Double-Close Elimination**: Subsequent calls to `.close()` encounter `Closed` and become atomic, safe no-ops.
3. **No Finalizer Hazard**: The GC Finalizer never guesses descriptors. If an object with an `Open` control block is swept, it logs an immediate diagnostic error (`[FATAL LEAK] Resource abandoned without deterministic close()`) and safely performs cleanup solely through the atomic control block.

---

## 3. Deterministic Resource Cleanup & Stack Unwinding

### 3.1 Canonical Syntax
Only the explicit block syntax is permitted for RAII scopes:

```raven
using res = Resource.open(...) {
    // Critical resource lifetime
}
```

### 3.2 Cleanup Stack Execution Matrix
Each execution frame maintains a LIFO `CleanupStack` of active `using` bindings.

| Scenario | Execution Sequence | Error / Suppressed Exception Policy |
|:---|:---|:---|
| **Normal Completion** | Pop handler, invoke `res.close()`. | If `res.close()` succeeds, proceed normally. If it fails, error becomes Primary Exception. |
| **Early `return <val>`** | 1. Root `<val>` in `ReturnSlot` (GC protected).<br>2. Pop and run each handler.<br>3. Return `<val>`. | If any `res.close()` fails, `<val>` is discarded and the cleanup error becomes the active exception. |
| **`break` / `continue`** | Pop and run all handlers up to the loop boundary; then execute the jump. | If cleanup fails, jump is cancelled and cleanup error is thrown. |
| **Exception In-Flight ($E_{active}$)** | Pop and run **every** cleanup handler in the unwinding chain. **Never abort subsequent cleanups on failure.** | If `res.close()` throws $E_{cleanup}$, $E_{cleanup}$ is appended to $E_{active}.suppressed$. Unwinding continues with $E_{active}$. |
| **Multiple Cleanup Failures** | All remaining cleanups on the stack MUST still execute. | Every encountered cleanup failure is recorded and preserved in the suppressed chain. |

### 3.3 GC Rooting of In-Flight Artifacts During Unwinding
During stack unwinding, cleanup functions may allocate memory or trigger a GC cycle. The runtime guarantees:
1. **Active Exception ($E_{active}$)** and its suppressed list are registered as dynamic GC roots.
2. **Pending Return Value (`ReturnSlot`)** is registered as a dynamic GC root until the calling frame takes ownership.

---

## 4. Single Memory Manager: Connecting `Arc<T>` with GC

To eliminate competing allocators and double-free hazards, Raven enforces a **Single Memory Manager** architecture:

```
┌───────────────────────────────────────────────────────────────┐
│                      Arc<T> Control Block                     │
│  • Atomic Strong RefCount                                     │
│  • Registered in Global ArcRootRegistry (GC Root Table)       │
└──────────────────────────────┬────────────────────────────────┘
                               │ Strong Count > 0 keeps Root Active
                               ▼
┌───────────────────────────────────────────────────────────────┐
│                  Garbage Collector Managed Heap               │
│  • Tracing Generational GC is the SOLE memory reclaimer       │
│  • When Strong Count reaches 0, Root is unregistered          │
│  • Object reclaimed naturally on next GC cycle                │
└───────────────────────────────────────────────────────────────┘
```

1. **`Arc<GCObject>`**:
   - `Arc` is **not** an independent memory deallocator for GC objects.
   - It acts as a **Cross-Thread Strong Root Provider**.
   - While `strong_count > 0`, the global `ArcRootRegistry` exposes the underlying pointer to the GC's root scanner.
   - When `strong_count == 0`, the pointer is removed from `ArcRootRegistry`. The GC frees the object during standard collection passes.
2. **`Arc<NativeResource>`**:
   - For unmanaged native handles, the `ResourceControlBlock` transitions to `Closed` when `strong_count == 0` and invokes native deallocation.

---

## 5. Precise GC Root Tracking & Safepoint Protocol

### 5.1 The Safepoint Protocol
No GC cycle may scan or modify the heap while mutator threads are actively executing bytecode:
1. **Safepoint Triggers**: Loop back-edges and function call sites.
2. **Stop-The-World Coordination**:
   - GC initiates a cycle by setting the global atomic `safepoint_requested = true`.
   - Mutator threads poll this flag at safepoint locations and suspend on a coordination barrier (`safepoint_ack`).
   - No heap allocations or pointer mutations occur during root scanning.

### 5.2 Precise Stack Maps (Corvus VM)
The bytecode compiler generates a static `StackMapTable` for every function chunk:
```cpp
struct StackMap {
    uint32_t bytecode_offset;
    std::vector<bool> slot_is_reference; // True if stack slot holds a GC pointer
};
```
During collection at a safepoint, the Corvus VM inspects the `StackMap` matching current `ip` to precisely identify references without conservative guessing.

### 5.3 Root Scopes (Tree-Walk Interpreter & Extensions)
For the tree-walk interpreter and native extensions, all temporary evaluation values must be guarded by RAII root handles:
```cpp
class RootScope {
public:
    explicit RootScope(Interpreter& interp);
    ~RootScope();
    void track(const Value& val);
};
```

---

## 6. Generational & Incremental GC Write Barriers

Raven separates write barrier semantics into two distinct mechanisms:

### 6.1 Generational Card-Marking Barrier (Minor GC)
- Partition heap into **Young Generation** (Eden + Survivor) and **Old Generation**.
- **Card Table**: 1 byte represents a 512-byte heap page.
- Whenever a reference field in an Old-generation object is updated:
  ```cpp
  inline void write_barrier_generational(void* old_obj, void* new_val) {
      if (is_old_gen(old_obj) && is_young_gen(new_val)) {
          card_table[card_index(old_obj)] = DIRTY_CARD;
      }
  }
  ```

### 6.2 Incremental Marking Barrier (Major GC)
To prevent the classic *lost object* bug where a mutator swings a pointer from a White object to a Black object without going through a Grey object:
- Raven adopts the **Incremental Update Barrier**:
  ```cpp
  inline void write_barrier_marking(GCObject* source, GCObject* target) {
      if (gc_phase == GCPhase::Marking && source->is_black() && target->is_white()) {
          target->set_grey();
          gc_mark_stack.push(target);
      }
  }
  ```

---

## 7. Escape Analysis & Stack Allocation Constraints

Stack allocation of objects is permitted **only** when proven safe by conservative escape analysis:
1. **Disqualification Criteria** (forcing heap allocation):
   - Escapes via `return` or function argument passed to an unknown/dynamic call.
   - Captured by a closure that outlives the current stack frame.
   - Stored into any field of a heap-allocated object (`Array`, `Map`, `ClassInstance`).
   - Spawned into or referenced by a concurrent thread (`spawn`).
2. **Default Conservative Rule**: If the compiler cannot prove total lexical non-escape across all possible control-flow branches, the object is heap allocated.

---

## 8. Mutex & MutexGuard Concurrency Contract

1. **Non-Recursive Semantics**: Raven mutexes are strictly non-recursive. Attempting to lock a mutex held by the current thread causes a `DeadlockError`.
2. **Borrow Lifetime Enforcement**:
   - `mutex.lock()` returns a unique `MutexGuard`.
   - Accessing data guarded by `MutexGuard` returns values by value or strictly scoped borrow references.
   - Borrow references derived from `MutexGuard` **cannot** be returned or stored outside the guard's `using` scope.
3. **Panic & Poisoning Policy**:
   - If a thread panics or encounters an unhandled runtime error while holding a `MutexGuard`, the Mutex enters the **Poisoned** state.
   - Subsequent `.lock()` calls receive a `PoisonedMutexError` unless explicitly cleared.

---

## 9. Comprehensive Verification & Stress Matrix

| Test Suite | Purpose & Verification Standard | Required Gate |
|:---|:---|:---|
| `tests/memory/safepoint_test` | Trigger GC at VM safepoints; verify zero pointer corruption with precise stack maps. | Pass under ASan |
| `tests/memory/unwind_suppressed_test` | Nested `using` failures; verify all cleanups execute and exceptions are chained. | 100% Pass |
| `tests/memory/return_root_test` | GC triggered during cleanup of function returning large array; verify array survives. | Pass under ASan |
| `tests/concurrency/arc_gc_test` | 16 threads sharing `Arc<Map>` under high allocation pressure; verify zero leaks. | Pass under TSan |
| `tests/concurrency/mutex_guard_test` | Mutex poisoning on panic; borrow lifetime enforcement at compile time. | Pass |
| `tests/memory/move_alias_test` | Verify compiler rejects copy of `File` and rejects use after `move`. | Pass |

---

## 10. Implementation Status Tracker

| Milestone | Component | Target Version | Status |
|:---|:---|:---|:---|
| **M1** | Striped ContainerLock & `Environment` shared mutex | v1.3.1 | **COMPLETED** |
| **M2** | Dynamic RootScanner & `GCRegistry` auto-tracking | v1.3.1 | **COMPLETED** |
| **M3** | Hardened ADR 0008.1 Specification | v1.4.0 | **COMPLETED** |
| **M4** | Atomic `ResourceControlBlock` & Double-close prevention | v1.4.0 | **ACTIVE** |
| **M5** | `UsingStmt` AST, Parser & CleanupStack unwinding with Suppressed Exceptions | v1.4.0 | **ACTIVE** |
| **M6** | In-flight Return Slot & Active Exception GC Rooting | v1.4.0 | **NEXT** |
| **M7** | Compiler Move-Only Enforcement & `move` operator | v1.4.0 | **PLANNED** |
| **M8** | Single Memory Manager: `Arc` as Dynamic GC Root Provider | v1.4.0 | **PLANNED** |
| **M9** | Corvus VM Precise Stack Maps & Safepoint Protocol | v1.5.0 | **PLANNED** |
| **M10** | Incremental Marking Write Barrier & Card Table | v1.5.0 | **PLANNED** |
