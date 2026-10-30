// value.cpp — Value type helper implementations
#include "raven/value.hpp"

namespace raven::runtime {

std::string value_type_to_string(const Value& val) {
    return val.type_name();
}

} // namespace raven::runtime
