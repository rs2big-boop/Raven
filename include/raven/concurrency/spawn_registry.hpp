#pragma once

#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

namespace raven {
namespace concurrency {

class SpawnRegistry {
public:
    static SpawnRegistry& instance() {
        static SpawnRegistry reg;
        return reg;
    }

    void add(std::thread t);
    void join_all(int64_t timeout_ms = 2000);

    ~SpawnRegistry();

private:
    SpawnRegistry() = default;
    std::mutex mtx_;
    std::vector<std::thread> threads_;
};

} // namespace concurrency
} // namespace raven
