# ADR 0008: Hybrid Memory & Resource Architecture (Historical Context & Principles)

- **Status**: SUPERSEDED
- **Date**: 2026-11-10
- **Authors**: Raven Language Architecture Group
- **Superceded by**: [ADR 0008.1](0008-1-ownership-root-tracking-cleanup-spec.md) and [ADR 0008.2](0008-2-leases-arc-sync-safepoint-lifecycle.md)

---

> [!IMPORTANT]
> **Notice of Architectural Specification Migration**
> All technical implementation contracts, state machine definitions, and concurrency specifications previously drafted in this ADR have been superseded and formalized in:
> - **[ADR 0008.1: Ownership, Root Tracking & Cleanup Semantics](0008-1-ownership-root-tracking-cleanup-spec.md)**
> - **[ADR 0008.2: Resource Leases, Arc/GC Synchronization & Safepoint Lifecycle](0008-2-leases-arc-sync-safepoint-lifecycle.md)**
>
> This document remains solely to record the original motivation, high-level rationale, and design philosophy.

---

## 1. Context & Motivation
Traditional programming languages generally force developers into one of two paradigms:
1. **Pure Garbage Collection (Python, Java, Go)**: Excellent developer velocity for standard algorithms, but non-deterministic for operating system handles (sockets, file descriptors, locks). Relying on GC passes for resource deallocation causes descriptor exhaustion and latency spikes.
2. **Exhaustive Compile-Time Borrow Checking (Rust)**: Deterministic lifetime and resource cleanup with zero runtime overhead, but imposes steep cognitive overhead that hinders rapid prototyping and application-level scripting.

Raven was conceived to provide the best of both worlds: **ergonomic dynamic scripting with systems-grade resource determinism**.

---

## 2. Fundamental Decision & Architectural Principles
Raven formally adopts a **Hybrid Memory & Resource Architecture**:
1. **Memory Management (Heap Allocation)**: Automated through a Tracing Generational Garbage Collector. Everyday data types (`String`, `Array`, `Map`, `ClassInstance`) require zero manual deallocation.
2. **Resource Management (OS Descriptors & Locks)**: Deterministic through lexical RAII scopes (`using`) backed by an unwinding `CleanupStack`.
3. **Concurrency Safety**: Enforces the invariant that **Shared Ownership does not equal Shared Mutability**. Shared data across threads must be explicitly synchronized.

For exact binding implementation specifications, please refer directly to **ADR 0008.1** and **ADR 0008.2**.
