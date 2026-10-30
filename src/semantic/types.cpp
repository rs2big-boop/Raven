// types.cpp — Type representations for Raven semantic analysis
#include <string>

namespace raven::semantic {

enum class TypeKind {
    Unknown,
    Nil,
    Boolean,
    Number,
    String,
    Array,
    Map,
    Function,
    Any,
};

std::string type_kind_name(TypeKind kind) {
    switch (kind) {
        case TypeKind::Unknown:  return "unknown";
        case TypeKind::Nil:      return "nil";
        case TypeKind::Boolean:  return "bool";
        case TypeKind::Number:   return "number";
        case TypeKind::String:   return "string";
        case TypeKind::Array:    return "array";
        case TypeKind::Map:      return "map";
        case TypeKind::Function: return "function";
        case TypeKind::Any:      return "any";
        default:                 return "unknown";
    }
}

bool is_numeric(TypeKind kind) {
    return kind == TypeKind::Number;
}

bool is_comparable(TypeKind a, TypeKind b) {
    if (a == TypeKind::Any || b == TypeKind::Any) return true;
    return a == b;
}

} // namespace raven::semantic
