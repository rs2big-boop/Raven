// stack.cpp — Value stack management for the Corvus VM
#include "raven/value.hpp"
#include <vector>

namespace raven::vm {

class VMStack {
public:
    void push(Value v) { stack_.push_back(std::move(v)); }
    Value pop() {
        Value v = std::move(stack_.back());
        stack_.pop_back();
        return v;
    }
    Value& peek(size_t distance = 0) {
        return stack_[stack_.size() - 1 - distance];
    }
    size_t size() const { return stack_.size(); }
    bool empty() const { return stack_.empty(); }
    void clear() { stack_.clear(); }

private:
    std::vector<Value> stack_;
};

} // namespace raven::vm
