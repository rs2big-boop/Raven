#pragma once

#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace raven {
namespace concurrency {

struct NativeWaitGroup {
    std::mutex mtx;
    std::condition_variable cv;
    int64_t counter = 0;
};

class WaitGroupManager {
public:
    static WaitGroupManager& instance() {
        static WaitGroupManager mgr;
        return mgr;
    }

    int64_t create();
    std::shared_ptr<NativeWaitGroup> get(int64_t id);
    bool add(int64_t id, int64_t delta);
    bool done(int64_t id);
    bool wait(int64_t id);

private:
    WaitGroupManager() = default;
    std::mutex mutex_;
    std::unordered_map<int64_t, std::shared_ptr<NativeWaitGroup>> wait_groups_;
    int64_t next_id_ = 1;
};

} // namespace concurrency
} // namespace raven
