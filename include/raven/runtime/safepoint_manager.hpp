#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <vector>

namespace raven {

enum class MutatorState : uint8_t {
    Running,
    AtSafepoint,
    InNativeBlocked
};

/**
 * @brief Coordinates Stop-The-World (STW) safepoints and Native Thread Parking
 * without deadlocks (ADR 0008.2 §4).
 */
class SafepointManager {
public:
    static SafepointManager& instance() {
        static SafepointManager mgr;
        return mgr;
    }

    void register_thread(std::thread::id tid = std::this_thread::get_id()) {
        if (tid != std::this_thread::get_id()) {
            throw std::invalid_argument("SafepointManager: cannot register non-current thread.");
        }
        std::unique_lock<std::mutex> lock(mtx_);
        if (safepoint_requested_.load(std::memory_order_acquire)) {
            thread_states_[tid] = MutatorState::AtSafepoint;
            cv_gc_.notify_all();
            cv_mutator_.wait(lock, [this]() {
                return !safepoint_requested_.load(std::memory_order_acquire);
            });
        }
        thread_states_[tid] = MutatorState::Running;
    }

    void unregister_thread(std::thread::id tid = std::this_thread::get_id()) {
        if (tid != std::this_thread::get_id()) {
            throw std::invalid_argument("SafepointManager: cannot unregister non-current thread.");
        }
        std::lock_guard<std::mutex> lock(mtx_);
        thread_states_.erase(tid);
        cv_gc_.notify_all();
    }

    /**
     * @brief Called by mutator thread before entering blocking native I/O / sleep.
     * Transitions state to InNativeBlocked; GC treats this thread's roots as frozen
     * and proceeds with STW without waiting.
     */
    void enter_native(std::thread::id tid = std::this_thread::get_id()) {
        if (tid != std::this_thread::get_id()) {
            throw std::invalid_argument("SafepointManager: cannot enter native on non-current thread.");
        }
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = thread_states_.find(tid);
        if (it != thread_states_.end()) {
            it->second = MutatorState::InNativeBlocked;
            if (safepoint_requested_.load(std::memory_order_relaxed)) {
                cv_gc_.notify_all();
            }
        }
    }

    /**
     * @brief Called by mutator thread upon returning from blocking native I/O.
     * If a safepoint is currently active, the thread parks until GC resumes.
     */
    void exit_native(std::thread::id tid = std::this_thread::get_id()) {
        if (tid != std::this_thread::get_id()) {
            throw std::invalid_argument("SafepointManager: cannot exit native on non-current thread.");
        }
        std::unique_lock<std::mutex> lock(mtx_);
        auto it = thread_states_.find(tid);
        if (it == thread_states_.end()) return;

        if (safepoint_requested_.load(std::memory_order_acquire)) {
            it->second = MutatorState::AtSafepoint;
            cv_gc_.notify_all();
            cv_mutator_.wait(lock, [this]() {
                return !safepoint_requested_.load(std::memory_order_acquire);
            });
        }
        it->second = MutatorState::Running;
    }

    /**
     * @brief Polled at loop back-edges and function call sites.
     * Extremely fast path: single atomic load when no safepoint is requested.
     */
    void poll_safepoint(std::thread::id tid = std::this_thread::get_id()) {
        if (tid != std::this_thread::get_id()) {
            throw std::invalid_argument("SafepointManager: cannot poll safepoint for non-current thread.");
        }
        if (!safepoint_requested_.load(std::memory_order_acquire)) {
            return;
        }

        std::unique_lock<std::mutex> lock(mtx_);
        auto it = thread_states_.find(tid);
        if (it == thread_states_.end()) return;

        it->second = MutatorState::AtSafepoint;
        cv_gc_.notify_all();

        cv_mutator_.wait(lock, [this]() {
            return !safepoint_requested_.load(std::memory_order_acquire);
        });

        it->second = MutatorState::Running;
    }

    /**
     * @brief Initiated by GC coordinator to begin an STW pause.
     * Serializes concurrent GC requests, enforces Single-GC-Owner semantics,
     * tracks generation epochs to prevent ABA hazards, and supports deadline timeouts.
     * @param timeout Maximum duration to wait for all mutator threads to park.
     */
    void request_safepoint(std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) {
        std::unique_lock<std::mutex> lock(mtx_);

        // 1. Serialize concurrent GC coordinator requests
        cv_coordinator_.wait(lock, [this]() {
            return !safepoint_requested_.load(std::memory_order_acquire);
        });

        // 2. Claim ownership of this safepoint session
        active_gc_owner_ = std::this_thread::get_id();
        safepoint_epoch_++;
        safepoint_requested_.store(true, std::memory_order_release);

        // 3. Wait with timeout for all registered mutators to park or block in native
        bool ok = cv_gc_.wait_for(lock, timeout, [this]() {
            for (const auto& [tid, state] : thread_states_) {
                if (state == MutatorState::Running) {
                    return false;
                }
            }
            return true;
        });

        if (!ok) {
            // Collect unparked threads for diagnostics
            std::string offenders;
            for (const auto& [tid, state] : thread_states_) {
                if (state == MutatorState::Running) {
                    offenders += "[tid " + std::to_string(std::hash<std::thread::id>{}(tid)) + "] ";
                }
            }
            // Teardown aborted session
            active_gc_owner_ = std::thread::id{};
            safepoint_requested_.store(false, std::memory_order_release);
            cv_mutator_.notify_all();
            cv_coordinator_.notify_one();
            throw std::runtime_error("SafepointManager: timeout waiting for mutator threads to park: " + offenders);
        }
    }

    /**
     * @brief Initiated by GC coordinator to resume all mutators after STW pause.
     * Validates that only the owning GC thread can resume mutators.
     */
    void resume_mutators() {
        std::lock_guard<std::mutex> lock(mtx_);
        if (!safepoint_requested_.load(std::memory_order_acquire)) {
            return;
        }

        // Validate Single-GC-Owner contract
        if (active_gc_owner_ != std::thread::id{} &&
            active_gc_owner_ != std::this_thread::get_id()) {
            throw std::runtime_error("SafepointManager: resume_mutators called by non-owner thread.");
        }

        active_gc_owner_ = std::thread::id{};
        safepoint_requested_.store(false, std::memory_order_release);
        cv_mutator_.notify_all();
        cv_coordinator_.notify_one(); // Wake up next queued coordinator if any
    }

    uint64_t safepoint_epoch() const noexcept {
        std::lock_guard<std::mutex> lock(mtx_);
        return safepoint_epoch_;
    }

    std::thread::id active_gc_owner() const noexcept {
        std::lock_guard<std::mutex> lock(mtx_);
        return active_gc_owner_;
    }

    bool is_safepoint_requested() const noexcept {
        return safepoint_requested_.load(std::memory_order_acquire);
    }

    size_t registered_thread_count() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return thread_states_.size();
    }

    size_t parked_thread_count() const {
        std::lock_guard<std::mutex> lock(mtx_);
        size_t count = 0;
        for (const auto& [tid, state] : thread_states_) {
            if (state == MutatorState::AtSafepoint || state == MutatorState::InNativeBlocked) {
                count++;
            }
        }
        return count;
    }

    MutatorState get_thread_state(std::thread::id tid) const {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = thread_states_.find(tid);
        return (it != thread_states_.end()) ? it->second : MutatorState::Running;
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mtx_);
        safepoint_requested_.store(false, std::memory_order_release);
        active_gc_owner_ = std::thread::id{};
        thread_states_.clear();
        cv_coordinator_.notify_all();
        cv_mutator_.notify_all();
    }

private:
    SafepointManager() : safepoint_requested_(false) {}

    mutable std::mutex mtx_;
    std::condition_variable cv_gc_;
    std::condition_variable cv_mutator_;
    std::condition_variable cv_coordinator_;
    std::atomic<bool> safepoint_requested_{false};
    std::thread::id active_gc_owner_{};
    uint64_t safepoint_epoch_{0};
    std::unordered_map<std::thread::id, MutatorState> thread_states_;
};

} // namespace raven
