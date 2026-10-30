#include "raven/value.hpp"

namespace raven::runtime {

std::shared_ptr<std::unordered_map<std::string, Value>> make_map() {
    return std::make_shared<std::unordered_map<std::string, Value>>();
}

bool map_contains_key(const Value& map_val, const std::string& key) {
    if (!map_val.is_map()) return false;
    return map_val.as_map()->find(key) != map_val.as_map()->end();
}

Value map_lookup(const Value& map_val, const std::string& key, const Value& default_val) {
    if (!map_val.is_map()) return default_val;
    auto it = map_val.as_map()->find(key);
    if (it != map_val.as_map()->end()) {
        return it->second;
    }
    return default_val;
}

void map_insert(Value& map_val, const std::string& key, const Value& val) {
    if (map_val.is_map()) {
        (*map_val.as_map())[key] = val;
    }
}

bool map_remove(Value& map_val, const std::string& key) {
    if (map_val.is_map()) {
        return map_val.as_map()->erase(key) > 0;
    }
    return false;
}

} // namespace raven::runtime
