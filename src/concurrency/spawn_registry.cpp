#include "raven/concurrency/spawn_registry.hpp"

#include <chrono>
#include <condition_variable>
#include <mutex>

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

        // Portable, standard C++ timed join using a scoped joiner thread.
        // Guarantees strict C++ thread lifecycle compliance, prevents double-join /
        // double-detach issues under AddressSanitizer and ThreadSanitizer, and works
        // uniformly across Linux, macOS, and Windows.
        auto t_ptr = std::make_shared<std::thread>(std::move(t));
        auto m = std::make_shared<std::mutex>();
        auto cv = std::make_shared<std::condition_variable>();
        auto joined = std::make_shared<bool>(false);

        std::thread joiner([t_ptr, m, cv, joined]() {
            if (t_ptr->joinable()) {
                t_ptr->join();
            }
            {
                std::lock_guard<std::mutex> lk(*m);
                *joined = true;
            }
            cv->notify_one();
        });

        std::unique_lock<std::mutex> lk(*m);
        bool ok = cv->wait_for(lk, std::chrono::milliseconds(remaining_ms), [&joined]() {
            return *joined;
        });

        if (ok) {
            if (joiner.joinable()) {
                joiner.join();
            }
        } else {
            // Timed out: detach joiner so it reaps t_ptr asynchronously without blocking or aborting
            if (joiner.joinable()) {
                joiner.detach();
            }
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
