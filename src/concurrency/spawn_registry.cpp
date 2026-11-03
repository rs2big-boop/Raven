#include "raven/concurrency/spawn_registry.hpp"

#include <chrono>
#include <ctime>

#if defined(__linux__)
#include <pthread.h>
#endif

namespace raven {
namespace concurrency {

void SpawnRegistry::add(std::thread t) {
    std::lock_guard<std::mutex> lk(mtx_);
    threads_.push_back(std::move(t));
}

void SpawnRegistry::join_all(int64_t timeout_ms) {
    std::lock_guard<std::mutex> lk(mtx_);

    if (timeout_ms < 0) {
        for (auto& t : threads_) {
            if (t.joinable()) {
                t.join();
            }
        }
        threads_.clear();
        return;
    }

    auto start = std::chrono::steady_clock::now();
    for (auto& t : threads_) {
        if (!t.joinable()) continue;

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        int64_t remaining_ms = timeout_ms - elapsed;

        if (remaining_ms <= 0) {
            t.detach();
            continue;
        }

#if defined(__linux__)
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += remaining_ms / 1000;
        ts.tv_nsec += (remaining_ms % 1000) * 1000000;
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000;
        }
        int rc = pthread_timedjoin_np(t.native_handle(), nullptr, &ts);
        if (rc != 0) {
            t.detach();
        }
#else
        t.join();
#endif
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
