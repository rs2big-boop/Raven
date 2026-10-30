// cancellation.cpp — Cooperative cancellation tokens for Raven
#include <atomic>
#include <memory>
#include <vector>
#include <functional>
#include <mutex>

namespace raven::concurrency {

struct CancellationContext {
    std::atomic<bool> cancelled{false};
    std::mutex mutex;
    std::vector<std::function<void()>> callbacks;
};

class CancellationToken {
public:
    explicit CancellationToken(std::shared_ptr<CancellationContext> ctx)
        : ctx_(std::move(ctx)) {}

    bool is_cancelled() const {
        return ctx_ && ctx_->cancelled.load(std::memory_order_relaxed);
    }

    void register_callback(std::function<void()> cb) {
        if (!ctx_) return;
        std::lock_guard<std::mutex> lock(ctx_->mutex);
        if (ctx_->cancelled) {
            cb();
        } else {
            ctx_->callbacks.push_back(std::move(cb));
        }
    }

private:
    std::shared_ptr<CancellationContext> ctx_;
};

class CancellationSource {
public:
    CancellationSource() : ctx_(std::make_shared<CancellationContext>()) {}

    CancellationToken token() const { return CancellationToken(ctx_); }

    void cancel() {
        ctx_->cancelled.store(true, std::memory_order_release);
        std::lock_guard<std::mutex> lock(ctx_->mutex);
        for (auto& cb : ctx_->callbacks) cb();
        ctx_->callbacks.clear();
    }

    bool is_cancelled() const { return ctx_->cancelled.load(); }

private:
    std::shared_ptr<CancellationContext> ctx_;
};

} // namespace raven::concurrency
