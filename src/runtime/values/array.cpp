#include "raven/value.hpp"
#include <algorithm>

namespace raven::runtime {

std::shared_ptr<std::vector<Value>> make_array(size_t capacity) {
    auto arr = std::make_shared<std::vector<Value>>();
    if (capacity > 0) {
        arr->reserve(capacity);
    }
    return arr;
}

void array_push(Value& arr_val, const Value& element) {
    if (arr_val.is_array()) {
        arr_val.as_array()->push_back(element);
    }
}

Value array_pop(Value& arr_val) {
    if (arr_val.is_array() && !arr_val.as_array()->empty()) {
        Value back = arr_val.as_array()->back();
        arr_val.as_array()->pop_back();
        return back;
    }
    return Value();
}

size_t array_length(const Value& arr_val) {
    if (arr_val.is_array()) {
        return arr_val.as_array()->size();
    }
    return 0;
}

} // namespace raven::runtime
