#include "raven/concurrency/mutex.hpp"

namespace raven {
namespace concurrency {

int64_t MutexManager::create() {
    auto m = std::make_shared<NativeMutex>();
    std::lock_guard<std::mutex> lk(mutex_);
    int64_t id = next_id_++;
    mutexes_[id] = m;
    return id;
}

std::shared_ptr<NativeMutex> MutexManager::get(int64_t id) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = mutexes_.find(id);
    if (it != mutexes_.end()) {
        return it->second;
    }
    return nullptr;
}

bool MutexManager::lock(int64_t id) {
    auto m = get(id);
    if (!m) return false;
    m->mtx.lock();
    m->locked = true;
    return true;
}

bool MutexManager::unlock(int64_t id) {
    auto m = get(id);
    if (!m) return false;
    m->locked = false;
    m->mtx.unlock();
    return true;
}

bool MutexManager::try_lock(int64_t id) {
    auto m = get(id);
    if (!m) return false;
    if (m->mtx.try_lock()) {
        m->locked = true;
        return true;
    }
    return false;
}

} // namespace concurrency
} // namespace raven
