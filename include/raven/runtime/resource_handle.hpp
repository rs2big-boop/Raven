#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>

namespace raven {

enum class ResourceState : uint8_t {
    Open,
    Closing,
    Closed
};

/**
 * @brief Thread-safe Atomic Control Block governing a physical system resource.
 *
 * Implements:
 * 1. Concurrency protection via Operation Leases (active_operations).
 * 2. Deterministic close draining in-flight leases before invoking native close.
 * 3. Atomic no-op on duplicate close calls.
 */
class ResourceControlBlock {
public:
    using NativeCloseFn = std::function<void(int)>;

    explicit ResourceControlBlock(int native_fd = -1,
                                 std::string descriptor_name = "",
                                 NativeCloseFn close_fn = nullptr)
        : native_fd_(native_fd),
          descriptor_name_(std::move(descriptor_name)),
          close_fn_(std::move(close_fn)),
          state_(ResourceState::Open) {}

    ~ResourceControlBlock() {
        close();
    }

    ResourceControlBlock(const ResourceControlBlock&) = delete;
    ResourceControlBlock& operator=(const ResourceControlBlock&) = delete;

    ResourceState state() const noexcept {
        return state_.load(std::memory_order_acquire);
    }

    bool is_open() const noexcept {
        return state() == ResourceState::Open;
    }

    bool is_closed() const noexcept {
        ResourceState s = state();
        return s == ResourceState::Closed || s == ResourceState::Closing;
    }

    /**
     * @brief Returns the native descriptor.
     * @note Accessing the native descriptor should be done via a valid ResourceLease
     * to guarantee that the descriptor remains open and valid for the duration of the operation.
     */
    [[nodiscard]] int native_fd() const noexcept {
        return native_fd_.load(std::memory_order_acquire);
    }

    const std::string& descriptor_name() const noexcept {
        return descriptor_name_;
    }

    uint32_t active_operations() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return active_operations_;
    }

    const std::string& last_close_error() const noexcept {
        std::lock_guard<std::mutex> lock(mtx_);
        return last_close_error_;
    }

    bool had_close_error() const noexcept {
        std::lock_guard<std::mutex> lock(mtx_);
        return !last_close_error_.empty();
    }

    /**
     * @brief Atomically initiates close. Drains all active operation leases
     * before calling close_fn_ exactly once.
     * @return true if this call performed the close; false if already closing/closed.
     */
    bool close() {
        std::unique_lock<std::mutex> lock(mtx_);
        if (state_.load(std::memory_order_acquire) != ResourceState::Open) {
            return false;
        }

        // 1. Transition to Closing: rejects any NEW leases immediately
        state_.store(ResourceState::Closing, std::memory_order_release);

        // 2. Wait for all in-flight operation leases to complete
        cv_.wait(lock, [this]() {
            return active_operations_ == 0;
        });

        // 3. Atomically exchange descriptor to -1 and invoke native closer exactly once
        int fd = native_fd_.exchange(-1, std::memory_order_acq_rel);
        if (close_fn_ && fd >= 0) {
            try {
                close_fn_(fd);
            } catch (const std::exception& e) {
                last_close_error_ = e.what();
            } catch (...) {
                last_close_error_ = "Unknown native exception during close";
            }
        }

        state_.store(ResourceState::Closed, std::memory_order_release);
        cv_.notify_all();
        return true;
    }

private:
    friend class ResourceLease;

    std::atomic<int> native_fd_{-1};
    std::string descriptor_name_;
    NativeCloseFn close_fn_;
    std::atomic<ResourceState> state_{ResourceState::Open};
    mutable std::mutex mtx_;
    mutable std::condition_variable cv_;
    uint32_t active_operations_{0};
    std::string last_close_error_;
};

/**
 * @brief Ephemeral RAII lease guarding an active operation (e.g. read, write) on a resource.
 * Prevents close() from closing the underlying descriptor while an operation is running.
 */
class ResourceLease {
public:
    ResourceLease() : block_(nullptr) {}

    explicit ResourceLease(std::shared_ptr<ResourceControlBlock> block)
        : block_(std::move(block)) {
        if (!block_) {
            throw std::runtime_error("Invalid resource handle: block is null.");
        }
        std::unique_lock<std::mutex> lock(block_->mtx_);
        if (block_->state_.load(std::memory_order_acquire) != ResourceState::Open) {
            throw std::runtime_error("Cannot operate on closed resource '" + block_->descriptor_name_ + "'.");
        }
        block_->active_operations_++;
    }

    ~ResourceLease() {
        reset();
    }

    ResourceLease(const ResourceLease&) = delete;
    ResourceLease& operator=(const ResourceLease&) = delete;

    ResourceLease(ResourceLease&& other) noexcept
        : block_(std::move(other.block_)) {
        other.block_ = nullptr;
    }

    ResourceLease& operator=(ResourceLease&& other) noexcept {
        if (this != &other) {
            reset();
            block_ = std::move(other.block_);
            other.block_ = nullptr;
        }
        return *this;
    }

    void reset() {
        if (block_) {
            std::lock_guard<std::mutex> lock(block_->mtx_);
            block_->active_operations_--;
            if (block_->active_operations_ == 0 &&
                block_->state_.load(std::memory_order_acquire) == ResourceState::Closing) {
                block_->cv_.notify_all();
            }
            block_ = nullptr;
        }
    }

    bool is_valid() const noexcept {
        return block_ != nullptr;
    }

    /**
     * @brief Accesses the native descriptor guarded by this lease.
     * Guaranteed not to be closed or invalidated while this lease is active.
     */
    [[nodiscard]] int native_fd() const noexcept {
        return block_ ? block_->native_fd() : -1;
    }

private:
    std::shared_ptr<ResourceControlBlock> block_;
};

/**
 * @brief Represents ownership of a resource at the variable binding level.
 * Handles move semantics: when moved, the handle is invalidated (moved = true)
 * while the underlying ResourceControlBlock remains Open for the new owner.
 */
class ResourceOwnerHandle {
public:
    ResourceOwnerHandle() : control_(nullptr), moved_(false) {}

    explicit ResourceOwnerHandle(std::shared_ptr<ResourceControlBlock> control)
        : control_(std::move(control)), moved_(false) {}

    ~ResourceOwnerHandle() = default;

    ResourceOwnerHandle(const ResourceOwnerHandle&) = delete;
    ResourceOwnerHandle& operator=(const ResourceOwnerHandle&) = delete;

    ResourceOwnerHandle(ResourceOwnerHandle&& other) noexcept
        : control_(std::move(other.control_)), moved_(other.moved_) {
        other.moved_ = true;
    }

    ResourceOwnerHandle& operator=(ResourceOwnerHandle&& other) noexcept {
        if (this != &other) {
            control_ = std::move(other.control_);
            moved_ = other.moved_;
            other.moved_ = true;
        }
        return *this;
    }

    bool is_valid() const noexcept {
        return !moved_ && control_ && control_->is_open();
    }

    bool is_moved() const noexcept {
        return moved_;
    }

    void mark_moved() noexcept {
        moved_ = true;
    }

    std::shared_ptr<ResourceControlBlock> control() const noexcept {
        return control_;
    }

    ResourceLease acquire_lease() const {
        if (moved_) {
            throw std::runtime_error("Cannot access resource: handle has been moved.");
        }
        return ResourceLease(control_);
    }

    bool close() {
        if (moved_ || !control_) {
            return false;
        }
        return control_->close();
    }

private:
    std::shared_ptr<ResourceControlBlock> control_;
    bool moved_{false};
};

/**
 * @brief Thread-safe Reaper Queue for asynchronous reclamation of abandoned resources.
 * Decouples GC finalization from blocking native I/O.
 */
class ResourceReaperQueue {
public:
    static ResourceReaperQueue& instance() {
        static ResourceReaperQueue q;
        return q;
    }

    void enqueue(std::shared_ptr<ResourceControlBlock> block) {
        if (!block) return;
        std::lock_guard<std::mutex> lock(mtx_);
        queue_.push(std::move(block));
    }

    size_t drain() {
        std::vector<std::shared_ptr<ResourceControlBlock>> to_close;
        {
            std::lock_guard<std::mutex> lock(mtx_);
            while (!queue_.empty()) {
                to_close.push_back(std::move(queue_.front()));
                queue_.pop();
            }
        }
        size_t count = 0;
        for (auto& item : to_close) {
            if (item && item->close()) {
                count++;
            }
        }
        return count;
    }

    size_t pending_count() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.size();
    }

private:
    ResourceReaperQueue() = default;
    mutable std::mutex mtx_;
    std::queue<std::shared_ptr<ResourceControlBlock>> queue_;
};

/**
 * @brief Global table mapping active system descriptors (sockets, pipes, handles)
 * to their governing ResourceControlBlocks. Enables thread-safe ResourceLease acquisition
 * across concurrent networking and I/O operations.
 */
class ResourceTable {
public:
    static ResourceTable& instance() {
        static ResourceTable inst;
        return inst;
    }

    void register_resource(int fd, const std::string& name, std::function<void(int)> close_fn) {
        if (fd < 0) return;
        std::lock_guard<std::mutex> lk(mtx_);
        table_[fd] = std::make_shared<ResourceControlBlock>(fd, name, std::move(close_fn));
    }

    std::shared_ptr<ResourceControlBlock> get(int fd) {
        std::lock_guard<std::mutex> lk(mtx_);
        auto it = table_.find(fd);
        return it != table_.end() ? it->second : nullptr;
    }

    bool close_resource(int fd) {
        std::shared_ptr<ResourceControlBlock> rcb;
        {
            std::lock_guard<std::mutex> lk(mtx_);
            auto it = table_.find(fd);
            if (it != table_.end()) {
                rcb = it->second;
                table_.erase(it);
            }
        }
        if (rcb) {
            return rcb->close();
        }
        return false;
    }

    void clear() {
        std::unordered_map<int, std::shared_ptr<ResourceControlBlock>> old;
        {
            std::lock_guard<std::mutex> lk(mtx_);
            old = std::move(table_);
        }
        for (auto& [_, rcb] : old) {
            if (rcb) rcb->close();
        }
    }

private:
    ResourceTable() = default;
    mutable std::mutex mtx_;
    std::unordered_map<int, std::shared_ptr<ResourceControlBlock>> table_;
};

} // namespace raven
