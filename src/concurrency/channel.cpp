#include "raven/concurrency/channel.hpp"

namespace raven {
namespace concurrency {

int64_t ChannelManager::create_channel(size_t capacity) {
    auto ch = std::make_shared<NativeChannel>();
    ch->capacity = capacity;
    std::lock_guard<std::mutex> lk(mutex_);
    int64_t id = next_id_++;
    channels_[id] = ch;
    return id;
}

std::shared_ptr<NativeChannel> ChannelManager::get_channel(int64_t id) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = channels_.find(id);
    if (it != channels_.end()) {
        return it->second;
    }
    return nullptr;
}

bool ChannelManager::close_channel(int64_t id) {
    std::shared_ptr<NativeChannel> ch;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        auto it = channels_.find(id);
        if (it == channels_.end()) return false;
        ch = it->second;
    }
    std::lock_guard<std::mutex> lk(ch->mtx);
    ch->closed = true;
    ch->cv_recv.notify_all();
    ch->cv_send.notify_all();
    return true;
}

} // namespace concurrency
} // namespace raven
