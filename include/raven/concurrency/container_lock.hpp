#pragma once
#include <mutex>
#include <cstdint>
#include <cstddef>

namespace raven::concurrency {

class ContainerLock {
public:
    static std::mutex& get_mutex(const void* ptr) {
        static constexpr size_t NUM_SHARDS = 64;
        static std::mutex shard_mutexes[NUM_SHARDS];
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        size_t idx = ((addr >> 4) ^ (addr >> 10)) % NUM_SHARDS;
        return shard_mutexes[idx];
    }
};

} // namespace raven::concurrency
