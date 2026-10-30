// scheduler.cpp — Worker-pool task scheduler for Raven
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <vector>
#include <atomic>

namespace raven::concurrency {

using WorkItem = std::function<void()>;

class TaskScheduler {
public:
    static TaskScheduler& instance() {
        static TaskScheduler scheduler(std::max(2u, std::thread::hardware_concurrency()));
        return scheduler;
    }

    explicit TaskScheduler(size_t workers) {
        for (size_t i = 0; i < workers; i++) {
            workers_.emplace_back([this]() { worker_loop(); });
        }
    }

    ~TaskScheduler() {
        shutdown();
    }

    void submit(WorkItem task) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(task));
            active_tasks_++;
        }
        cv_.notify_one();
    }

    void wait_all() {
        std::unique_lock<std::mutex> lock(mutex_);
        idle_cv_.wait(lock, [this]() { return active_tasks_ == 0 && queue_.empty(); });
    }

    void shutdown() {
        running_ = false;
        cv_.notify_all();
        for (auto& w : workers_) {
            if (w.joinable()) w.join();
        }
        workers_.clear();
    }

    size_t worker_count() const { return workers_.size(); }

private:
    void worker_loop() {
        while (running_) {
            WorkItem task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this]() { return !queue_.empty() || !running_; });
                if (!running_ && queue_.empty()) break;
                if (queue_.empty()) continue;
                task = std::move(queue_.front());
                queue_.pop();
            }
            task();
            int remaining = --active_tasks_;
            if (remaining == 0) idle_cv_.notify_all();
        }
    }

    std::vector<std::thread> workers_;
    std::queue<WorkItem> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::condition_variable idle_cv_;
    std::atomic<bool> running_{true};
    std::atomic<int> active_tasks_{0};
};

} // namespace raven::concurrency
