#include "raven/runtime/memory/managed_heap.hpp"
#include <algorithm>

namespace raven::runtime::memory {

ManagedHeap::ManagedHeap() = default;

ManagedHeap& ManagedHeap::instance() {
    static ManagedHeap instance;
    return instance;
}

void ManagedHeap::track(ManagedObject* obj, size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);
    live_objects_.insert(obj);
    allocated_bytes_ += size;
    stats_.total_allocated += size;
    stats_.total_objects++;
    stats_.total_alloc_ops++;
    if (stats_.total_allocated > stats_.peak_allocated) {
        stats_.peak_allocated = stats_.total_allocated;
    }
}

void ManagedHeap::on_release(ManagedObject* obj, size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);
    live_objects_.erase(obj);
    if (allocated_bytes_ >= size) allocated_bytes_ -= size;
    if (stats_.total_allocated >= size) stats_.total_allocated -= size;
    if (stats_.total_objects > 0) stats_.total_objects--;
    stats_.total_free_ops++;
}

void ManagedHeap::visit_all(void (*visitor)(ManagedObject*)) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto* obj : live_objects_) {
        visitor(obj);
    }
}

HeapStats ManagedHeap::stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    HeapStats s = stats_;
    s.total_allocated = allocated_bytes_.load(std::memory_order_relaxed);
    return s;
}

void ManagedHeap::force_shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    live_objects_.clear();
    allocated_bytes_ = 0;
}

bool ManagedHeap::should_collect() const {
    return allocated_bytes_.load(std::memory_order_relaxed) > gc_threshold_bytes_;
}

} // namespace raven::runtime::memory
