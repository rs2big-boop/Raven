#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
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
     * Blocks until all registered mutators are either AtSafepoint or InNativeBlocked.
     */
    void request_safepoint() {
        std::unique_lock<std::mutex> lock(mtx_);
        safepoint_requested_.store(true, std::memory_order_release);

        cv_gc_.wait(lock, [this]() {
            for (const auto& [tid, state] : thread_states_) {
                if (state == MutatorState::Running) {
                    return false;
                }
            }
            return true;
        });
    }

    /**
     * @brief Initiated by GC coordinator to resume all mutators after STW pause.
     */
    void resume_mutators() {
        std::lock_guard<std::mutex> lock(mtx_);
        safepoint_requested_.store(false, std::memory_order_release);
        cv_mutator_.notify_all();
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
        thread_states_.clear();
    }

private:
    SafepointManager() : safepoint_requested_(false) {}

    mutable std::mutex mtx_;
    std::condition_variable cv_gc_;
    std::condition_variable cv_mutator_;
    std::atomic<bool> safepoint_requested_{false};
    std::unordered_map<std::thread::id, MutatorState> thread_states_;
};

} // namespace raven
