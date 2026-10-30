#pragma once
#include <atomic>
#include <cstdint>
#include <functional>

namespace raven::runtime::memory {

class ManagedObject; // forward declaration

// Helpers for atomic reference counting operations.
// These are free functions to avoid virtual dispatch overhead.
struct ReferenceCounter {
    // Increment ref count atomically. Returns the new value.
    static uint32_t inc(std::atomic<uint32_t>& counter) noexcept {
        return counter.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    // Decrement ref count atomically. Returns the new value.
    // If return value is 0, the caller must release the object.
    static uint32_t dec(std::atomic<uint32_t>& counter) noexcept {
        return counter.fetch_sub(1, std::memory_order_acq_rel) - 1;
    }

    // Atomically load the current count.
    static uint32_t load(const std::atomic<uint32_t>& counter) noexcept {
        return counter.load(std::memory_order_relaxed);
    }

    // Compare-and-swap: set counter to desired if it equals expected.
    static bool compare_exchange(std::atomic<uint32_t>& counter,
                                  uint32_t expected, uint32_t desired) noexcept {
        return counter.compare_exchange_strong(expected, desired,
                                               std::memory_order_acq_rel,
                                               std::memory_order_relaxed);
    }
};

} // namespace raven::runtime::memory
