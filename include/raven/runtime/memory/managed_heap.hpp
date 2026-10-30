#pragma once
#include "managed_object.hpp"
#include "object_header.hpp"
#include <cstddef>
#include <cstdlib>
#include <atomic>
#include <mutex>
#include <unordered_set>

namespace raven::runtime::memory {

struct HeapStats {
    size_t total_allocated   = 0;  // bytes currently allocated
    size_t total_objects     = 0;  // live object count
    size_t peak_allocated    = 0;  // max bytes ever allocated at once
    size_t total_alloc_ops   = 0;  // lifetime alloc count
    size_t total_free_ops    = 0;  // lifetime free count
    size_t gc_collections    = 0;  // number of GC cycles triggered
};

// Managed heap: allocates ManagedObject instances and tracks them.
// Works in tandem with RootRegistry and CycleCollector.
class ManagedHeap {
public:
    static ManagedHeap& instance();

    // Allocate a new object. The object is constructed with `args`
    // and tracked by the heap. Returns a raw pointer (ref count == 1).
    template<typename T, typename... Args>
    T* allocate(Args&&... args) {
        T* obj = new T(std::forward<Args>(args)...);
        track(obj, sizeof(T));
        return obj;
    }

    // Called when an object's ref count reaches 0 — removes from tracking
    void on_release(ManagedObject* obj, size_t size);

    // Visit all live objects (for GC mark phase)
    void visit_all(void (*visitor)(ManagedObject*)) const;

    // Statistics
    HeapStats stats() const;

    // Force-free all tracked objects (for shutdown)
    void force_shutdown();

    // GC threshold: trigger cycle collection when allocation exceeds this
    size_t gc_threshold() const { return gc_threshold_bytes_; }
    void set_gc_threshold(size_t bytes) { gc_threshold_bytes_ = bytes; }

private:
    ManagedHeap();
    void track(ManagedObject* obj, size_t size);
    bool should_collect() const;

    mutable std::mutex mutex_;
    std::unordered_set<ManagedObject*> live_objects_;
    std::atomic<size_t> allocated_bytes_{0};
    HeapStats stats_;
    size_t gc_threshold_bytes_ = 1024 * 1024; // 1 MB default
};

} // namespace raven::runtime::memory
