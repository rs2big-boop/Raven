#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace raven {

enum class GCPhase : uint8_t {
    Idle,
    InitialMark,
    IncrementalMarking,
    FinalRemark,
    Sweep
};

/**
 * @brief Global Thread-Safe Striped Registry tracking all active Arc<T> references
 * as GC root providers (ADR 0008.2 §3).
 * Uses 32 striped mutex partitions to eliminate cross-thread cache-line contention.
 */
class ArcRootRegistry {
public:
    static constexpr size_t kNumStripes = 32;

    static ArcRootRegistry& instance() {
        static ArcRootRegistry reg;
        return reg;
    }

    void set_gc_phase(GCPhase phase) noexcept {
        gc_phase_.store(phase, std::memory_order_release);
        if (phase == GCPhase::Idle) {
            // When GC cycle completes, release all epoch-pinned grey root lifetime locks
            std::lock_guard<std::mutex> lock(grey_mtx_);
            pinned_grey_roots_.clear();
            deferred_reclaim_roots_.clear();
        }
    }

    GCPhase gc_phase() const noexcept {
        return gc_phase_.load(std::memory_order_acquire);
    }

    // Invoked when an Arc is created or cloned
    void register_root(void* gc_ptr) {
        if (!gc_ptr) return;
        size_t idx = stripe_index(gc_ptr);
        {
            std::unique_lock<std::shared_mutex> lock(stripes_[idx].mtx);
            stripes_[idx].active_roots[gc_ptr]++;
        }
        // Root Write Barrier: if in IncrementalMarking phase, shade grey and pin lifetime
        if (gc_phase() == GCPhase::IncrementalMarking) {
            write_barrier_root(gc_ptr);
        }
    }

    /**
     * @brief Invoked when an Arc strong count drops.
     * If the root count reaches 0 during an active GC marking pass, its lifetime
     * is pinned in deferred_reclaim_roots_ until the current cycle's Sweep completes,
     * guaranteeing that the GC scanner never encounters dangling grey roots.
     */
    void unregister_root(void* gc_ptr) {
        if (!gc_ptr) return;
        size_t idx = stripe_index(gc_ptr);
        std::unique_lock<std::shared_mutex> lock(stripes_[idx].mtx);
        auto it = stripes_[idx].active_roots.find(gc_ptr);
        if (it != stripes_[idx].active_roots.end()) {
            if (--(it->second) == 0) {
                stripes_[idx].active_roots.erase(it);

                // If GC is active, retain lifetime pin until Idle phase
                if (gc_phase() != GCPhase::Idle) {
                    std::lock_guard<std::mutex> glock(grey_mtx_);
                    if (pinned_grey_roots_.find(gc_ptr) != pinned_grey_roots_.end()) {
                        deferred_reclaim_roots_.insert(gc_ptr);
                    }
                }
            }
        }
    }

    // Invoked by the GC coordinator during STW Initial Mark
    std::vector<void*> snapshot_roots() {
        std::vector<void*> roots;
        for (size_t i = 0; i < kNumStripes; ++i) {
            std::shared_lock<std::shared_mutex> lock(stripes_[i].mtx);
            if (stripes_[i].active_roots.empty()) continue;
            roots.reserve(roots.size() + stripes_[i].active_roots.size());
            for (const auto& [ptr, count] : stripes_[i].active_roots) {
                if (count > 0 && ptr != nullptr) {
                    roots.push_back(ptr);
                }
            }
        }
        return roots;
    }

    size_t root_count() const {
        size_t total = 0;
        for (size_t i = 0; i < kNumStripes; ++i) {
            std::shared_lock<std::shared_mutex> lock(stripes_[i].mtx);
            total += stripes_[i].active_roots.size();
        }
        return total;
    }

    size_t strong_count(void* gc_ptr) const {
        if (!gc_ptr) return 0;
        size_t idx = stripe_index(gc_ptr);
        std::shared_lock<std::shared_mutex> lock(stripes_[idx].mtx);
        auto it = stripes_[idx].active_roots.find(gc_ptr);
        return (it != stripes_[idx].active_roots.end()) ? it->second : 0;
    }

    /**
     * @brief Root Write Barrier shading an object into grey roots.
     * Pins the pointer into pinned_grey_roots_ to guarantee lifetime through FinalRemark.
     */
    void write_barrier_root(void* gc_ptr) {
        if (!gc_ptr) return;
        std::lock_guard<std::mutex> lock(grey_mtx_);
        grey_roots_.push_back(gc_ptr);
        pinned_grey_roots_.insert(gc_ptr);
    }

    std::vector<void*> drain_grey_roots() {
        std::lock_guard<std::mutex> lock(grey_mtx_);
        std::vector<void*> drained = std::move(grey_roots_);
        grey_roots_.clear();
        return drained;
    }

    bool is_root_pinned_for_gc(void* gc_ptr) const {
        if (!gc_ptr) return false;
        std::lock_guard<std::mutex> lock(grey_mtx_);
        return pinned_grey_roots_.find(gc_ptr) != pinned_grey_roots_.end();
    }

    void clear() {
        for (size_t i = 0; i < kNumStripes; ++i) {
            std::unique_lock<std::shared_mutex> lock(stripes_[i].mtx);
            stripes_[i].active_roots.clear();
        }
        std::lock_guard<std::mutex> glock(grey_mtx_);
        grey_roots_.clear();
        pinned_grey_roots_.clear();
        deferred_reclaim_roots_.clear();
        gc_phase_.store(GCPhase::Idle, std::memory_order_release);
    }

private:
    struct alignas(64) Stripe {
        mutable std::shared_mutex mtx;
        std::unordered_map<void*, size_t> active_roots;
    };

    ArcRootRegistry() : gc_phase_(GCPhase::Idle) {}

    static size_t stripe_index(const void* ptr) noexcept {
        uintptr_t v = reinterpret_cast<uintptr_t>(ptr);
        return ((v >> 4) * 2654435761u) & (kNumStripes - 1);
    }

    std::array<Stripe, kNumStripes> stripes_;
    std::atomic<GCPhase> gc_phase_{GCPhase::Idle};

    mutable std::mutex grey_mtx_;
    std::vector<void*> grey_roots_;
    std::unordered_set<void*> pinned_grey_roots_;
    std::unordered_set<void*> deferred_reclaim_roots_;
};

/**
 * @brief Thread-safe Atomically Reference-Counted pointer bridging native mutator
 * threads and the GC root scanner (ADR 0008.2 §3.1).
 */
template <typename T>
class Arc {
public:
    Arc() : ptr_(nullptr) {}

    explicit Arc(T* ptr) : ptr_(ptr) {
        if (ptr_) {
            ArcRootRegistry::instance().register_root(static_cast<void*>(ptr_));
        }
    }

    ~Arc() {
        if (ptr_) {
            ArcRootRegistry::instance().unregister_root(static_cast<void*>(ptr_));
            ptr_ = nullptr;
        }
    }

    Arc(const Arc& other) : ptr_(other.ptr_) {
        if (ptr_) {
            ArcRootRegistry::instance().register_root(static_cast<void*>(ptr_));
        }
    }

    Arc& operator=(const Arc& other) {
        if (this != &other) {
            if (ptr_) {
                ArcRootRegistry::instance().unregister_root(static_cast<void*>(ptr_));
            }
            ptr_ = other.ptr_;
            if (ptr_) {
                ArcRootRegistry::instance().register_root(static_cast<void*>(ptr_));
            }
        }
        return *this;
    }

    Arc(Arc&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }

    Arc& operator=(Arc&& other) noexcept {
        if (this != &other) {
            if (ptr_) {
                ArcRootRegistry::instance().unregister_root(static_cast<void*>(ptr_));
            }
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    T* get() const noexcept { return ptr_; }
    T& operator*() const noexcept { return *ptr_; }
    T* operator->() const noexcept { return ptr_; }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    size_t use_count() const {
        return ptr_ ? ArcRootRegistry::instance().strong_count(static_cast<void*>(ptr_)) : 0;
    }

private:
    T* ptr_;
};

} // namespace raven
