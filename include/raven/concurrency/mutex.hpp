#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace raven {
namespace concurrency {

struct NativeMutex {
    std::recursive_mutex mtx;
    std::atomic<bool> locked{false};
};

class MutexManager {
public:
    static MutexManager& instance() {
        static MutexManager mgr;
        return mgr;
    }

    int64_t create();
    std::shared_ptr<NativeMutex> get(int64_t id);
    bool lock(int64_t id);
    bool unlock(int64_t id);
    bool try_lock(int64_t id);

private:
    MutexManager() = default;
    std::mutex mutex_;
    std::unordered_map<int64_t, std::shared_ptr<NativeMutex>> mutexes_;
    int64_t next_id_ = 1;
};

} // namespace concurrency
} // namespace raven
