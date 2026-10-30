#include "raven/runtime/memory/object_header.hpp"
#include <string>

namespace raven::runtime::memory {

std::string object_type_name(ObjectType type) {
    switch (type) {
        case ObjectType::String:   return "String";
        case ObjectType::Array:    return "Array";
        case ObjectType::Map:      return "Map";
        case ObjectType::Function: return "Function";
        case ObjectType::Closure:  return "Closure";
        case ObjectType::Native:   return "Native";
        default:                   return "Unknown";
    }
}

std::string gc_color_name(GCColor color) {
    switch (color) {
        case GCColor::Black:  return "Black";
        case GCColor::Gray:   return "Gray";
        case GCColor::White:  return "White";
        case GCColor::Purple: return "Purple";
        default:              return "Unknown";
    }
}

} // namespace raven::runtime::memory
