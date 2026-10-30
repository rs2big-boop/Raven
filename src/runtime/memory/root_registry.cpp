#include "raven/runtime/memory/root_registry.hpp"
#include <algorithm>

namespace raven::runtime::memory {

RootRegistry& RootRegistry::instance() {
    static RootRegistry instance;
    return instance;
}

void RootRegistry::add_root(ManagedObject* obj) {
    if (!obj) return;
    std::lock_guard<std::mutex> lock(mutex_);
    roots_.insert(obj);
}

void RootRegistry::remove_root(ManagedObject* obj) {
    if (!obj) return;
    std::lock_guard<std::mutex> lock(mutex_);
    roots_.erase(obj);
}

void RootRegistry::visit_roots(void (*visitor)(ManagedObject*)) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto* obj : roots_) {
        visitor(obj);
    }
}

size_t RootRegistry::root_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return roots_.size();
}

void RootRegistry::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    roots_.clear();
}

} // namespace raven::runtime::memory
