// thread.cpp — Thread abstraction for Raven
#include <thread>
#include <functional>
#include <string>
#include <atomic>

namespace raven::concurrency {

class ManagedThread {
public:
    using TaskFn = std::function<void()>;

    explicit ManagedThread(std::string name, TaskFn task)
        : name_(std::move(name)), task_(std::move(task)) {}

    void start() {
        if (running_) return;
        running_ = true;
        thread_ = std::thread([this]() {
            task_();
            running_ = false;
        });
    }

    void join() {
        if (thread_.joinable()) thread_.join();
    }

    bool is_running() const { return running_; }
    const std::string& name() const { return name_; }

    ~ManagedThread() {
        if (thread_.joinable()) thread_.detach();
    }

private:
    std::string name_;
    TaskFn task_;
    std::thread thread_;
    std::atomic<bool> running_{false};
};

} // namespace raven::concurrency
