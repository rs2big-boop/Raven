#include "raven/concurrency/wait_group.hpp"

namespace raven {
namespace concurrency {

int64_t WaitGroupManager::create() {
    auto wg = std::make_shared<NativeWaitGroup>();
    std::lock_guard<std::mutex> lk(mutex_);
    int64_t id = next_id_++;
    wait_groups_[id] = wg;
    return id;
}

std::shared_ptr<NativeWaitGroup> WaitGroupManager::get(int64_t id) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = wait_groups_.find(id);
    if (it != wait_groups_.end()) {
        return it->second;
    }
    return nullptr;
}

bool WaitGroupManager::add(int64_t id, int64_t delta) {
    auto wg = get(id);
    if (!wg) return false;
    std::lock_guard<std::mutex> lk(wg->mtx);
    wg->counter += delta;
    return true;
}

bool WaitGroupManager::done(int64_t id) {
    auto wg = get(id);
    if (!wg) return false;
    std::lock_guard<std::mutex> lk(wg->mtx);
    wg->counter--;
    if (wg->counter <= 0) {
        wg->counter = 0;
        wg->cv.notify_all();
    }
    return true;
}

bool WaitGroupManager::wait(int64_t id) {
    auto wg = get(id);
    if (!wg) return false;
    std::unique_lock<std::mutex> lk(wg->mtx);
    wg->cv.wait(lk, [&]() { return wg->counter <= 0; });
    return true;
}

} // namespace concurrency
} // namespace raven
