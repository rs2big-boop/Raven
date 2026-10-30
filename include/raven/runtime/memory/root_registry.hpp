#pragma once
#include "managed_object.hpp"
#include <unordered_set>
#include <mutex>
#include <vector>

namespace raven::runtime::memory {

// Tracks all GC roots (objects directly referenced by the interpreter stack,
// globals, and call frames). The cycle collector starts traversal from roots.
class RootRegistry {
public:
    static RootRegistry& instance();

    // Register/unregister a root object
    void add_root(ManagedObject* obj);
    void remove_root(ManagedObject* obj);

    // Visit all roots — used by the cycle collector
    void visit_roots(void (*visitor)(ManagedObject*)) const;

    // Snapshot of current roots count (for diagnostics)
    size_t root_count() const;

    // Clear all roots (for shutdown/reset)
    void clear();

private:
    RootRegistry() = default;
    mutable std::mutex mutex_;
    std::unordered_set<ManagedObject*> roots_;
};

// RAII helper: register an object as a root for the lifetime of this guard
class RootGuard {
public:
    explicit RootGuard(ManagedObject* obj) : obj_(obj) {
        if (obj_) RootRegistry::instance().add_root(obj_);
    }
    ~RootGuard() {
        if (obj_) RootRegistry::instance().remove_root(obj_);
    }
    RootGuard(const RootGuard&) = delete;
    RootGuard& operator=(const RootGuard&) = delete;
private:
    ManagedObject* obj_;
};

} // namespace raven::runtime::memory
