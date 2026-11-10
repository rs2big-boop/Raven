#pragma once
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

namespace raven {
class Value;
using ArrayPtr = std::shared_ptr<std::vector<Value>>;
using MapPtr = std::shared_ptr<std::unordered_map<std::string, Value>>;

namespace gc {
void track_array_auto(const ArrayPtr& arr);
void track_map_auto(const MapPtr& map);
}
}
