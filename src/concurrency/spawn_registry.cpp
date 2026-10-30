#include "raven/concurrency/spawn_registry.hpp"

namespace raven {
namespace concurrency {

void SpawnRegistry::add(std::thread t) {
    std::lock_guard<std::mutex> lk(mtx_);
    threads_.push_back(std::move(t));
}

void SpawnRegistry::join_all(int64_t /*timeout_ms*/) {
    std::lock_guard<std::mutex> lk(mtx_);
    for (auto& t : threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
    threads_.clear();
}

SpawnRegistry::~SpawnRegistry() {
    std::lock_guard<std::mutex> lk(mtx_);
    for (auto& t : threads_) {
        if (t.joinable()) {
            t.detach();
        }
    }
}

} // namespace concurrency
} // namespace raven
