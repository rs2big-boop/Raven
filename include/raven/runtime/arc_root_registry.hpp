#pragma once

#include <atomic>
#include <cstddef>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
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
 * @brief Global Thread-Safe Registry tracking all active Arc<T> references
 * as GC root providers (ADR 0008.2 §3).
 */
class ArcRootRegistry {
public:
    static ArcRootRegistry& instance() {
        static ArcRootRegistry reg;
        return reg;
    }

    void set_gc_phase(GCPhase phase) noexcept {
        gc_phase_.store(phase, std::memory_order_release);
    }

    GCPhase gc_phase() const noexcept {
        return gc_phase_.load(std::memory_order_acquire);
    }

    // Invoked when an Arc is created or cloned
    void register_root(void* gc_ptr) {
        if (!gc_ptr) return;
        {
            std::unique_lock<std::shared_mutex> lock(registry_mtx_);
            active_roots_[gc_ptr]++;
        }
        // Root Write Barrier: if in IncrementalMarking phase, shade grey
        if (gc_phase() == GCPhase::IncrementalMarking) {
            write_barrier_root(gc_ptr);
        }
    }

    // Invoked when an Arc strong count drops
    void unregister_root(void* gc_ptr) {
        if (!gc_ptr) return;
        std::unique_lock<std::shared_mutex> lock(registry_mtx_);
        auto it = active_roots_.find(gc_ptr);
        if (it != active_roots_.end()) {
            if (--(it->second) == 0) {
                active_roots_.erase(it);
            }
        }
    }

    // Invoked by the GC coordinator during STW Initial Mark
    std::vector<void*> snapshot_roots() {
        std::shared_lock<std::shared_mutex> lock(registry_mtx_);
        std::vector<void*> roots;
        roots.reserve(active_roots_.size());
        for (const auto& [ptr, count] : active_roots_) {
            if (count > 0 && ptr != nullptr) {
                roots.push_back(ptr);
            }
        }
        return roots;
    }

    size_t root_count() const {
        std::shared_lock<std::shared_mutex> lock(registry_mtx_);
        return active_roots_.size();
    }

    size_t strong_count(void* gc_ptr) const {
        if (!gc_ptr) return 0;
        std::shared_lock<std::shared_mutex> lock(registry_mtx_);
        auto it = active_roots_.find(gc_ptr);
        return (it != active_roots_.end()) ? it->second : 0;
    }

    void write_barrier_root(void* gc_ptr) {
        if (!gc_ptr) return;
        std::lock_guard<std::mutex> lock(grey_mtx_);
        grey_roots_.push_back(gc_ptr);
    }

    std::vector<void*> drain_grey_roots() {
        std::lock_guard<std::mutex> lock(grey_mtx_);
        std::vector<void*> drained = std::move(grey_roots_);
        grey_roots_.clear();
        return drained;
    }

    void clear() {
        std::unique_lock<std::shared_mutex> lock(registry_mtx_);
        active_roots_.clear();
        std::lock_guard<std::mutex> glock(grey_mtx_);
        grey_roots_.clear();
        gc_phase_.store(GCPhase::Idle, std::memory_order_release);
    }

private:
    ArcRootRegistry() : gc_phase_(GCPhase::Idle) {}

    mutable std::shared_mutex registry_mtx_;
    std::unordered_map<void*, size_t> active_roots_;
    std::atomic<GCPhase> gc_phase_{GCPhase::Idle};

    std::mutex grey_mtx_;
    std::vector<void*> grey_roots_;
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
