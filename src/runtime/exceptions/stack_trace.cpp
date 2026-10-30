// stack_trace.cpp — Call stack trace capture and formatting
#include <string>
#include <vector>
#include <sstream>

namespace raven::runtime {

struct StackFrameInfo {
    std::string function_name;
    std::string source_file;
    int line;

    std::string to_string() const {
        std::ostringstream oss;
        oss << "  at " << function_name;
        if (!source_file.empty()) {
            oss << " (" << source_file << ":" << line << ")";
        }
        return oss.str();
    }
};

class StackTraceRecorder {
public:
    void push(StackFrameInfo frame) { frames_.push_back(std::move(frame)); }
    void pop() { if (!frames_.empty()) frames_.pop_back(); }

    std::string format() const {
        std::ostringstream oss;
        oss << "Stack trace (most recent call last):\n";
        for (int i = static_cast<int>(frames_.size()) - 1; i >= 0; i--) {
            oss << frames_[i].to_string() << "\n";
        }
        return oss.str();
    }

    bool empty() const { return frames_.empty(); }
    size_t depth() const { return frames_.size(); }

private:
    std::vector<StackFrameInfo> frames_;
};

} // namespace raven::runtime
