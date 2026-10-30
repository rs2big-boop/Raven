// task.cpp — Asynchronous task unit for Raven
#include <functional>
#include <string>
#include <atomic>

namespace raven::concurrency {

enum class TaskStatus {
    Pending,
    Running,
    Completed,
    Failed,
};

class AsyncTask {
public:
    using Fn = std::function<void()>;

    explicit AsyncTask(std::string name, Fn fn)
        : name_(std::move(name)), fn_(std::move(fn)) {}

    void execute() {
        status_ = TaskStatus::Running;
        try {
            fn_();
            status_ = TaskStatus::Completed;
        } catch (...) {
            status_ = TaskStatus::Failed;
        }
    }

    TaskStatus status() const { return status_.load(); }
    const std::string& name() const { return name_; }

private:
    std::string name_;
    Fn fn_;
    std::atomic<TaskStatus> status_{TaskStatus::Pending};
};

} // namespace raven::concurrency
