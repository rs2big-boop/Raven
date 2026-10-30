#include "raven/value.hpp"

namespace raven::runtime {

bool is_invocable(const Value& val) {
    return val.is_callable();
}

std::string callable_name(const Value& val) {
    if (!val.is_callable()) return "";
    return val.to_string();
}

} // namespace raven::runtime
