#pragma once

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "raven/value.hpp"

namespace raven {
namespace concurrency {

struct NativeChannel {
    std::mutex mtx;
    std::condition_variable cv_recv;
    std::condition_variable cv_send;
    std::deque<Value> queue;
    size_t capacity = 0;
    bool closed = false;
};

class ChannelManager {
public:
    static ChannelManager& instance() {
        static ChannelManager mgr;
        return mgr;
    }

    int64_t create_channel(size_t capacity);
    std::shared_ptr<NativeChannel> get_channel(int64_t id);
    bool close_channel(int64_t id);

private:
    ChannelManager() = default;
    std::mutex mutex_;
    std::unordered_map<int64_t, std::shared_ptr<NativeChannel>> channels_;
    int64_t next_id_ = 1;
};

} // namespace concurrency
} // namespace raven
