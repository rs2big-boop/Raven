#include "raven/runtime/memory/cycle_collector.hpp"
#include "raven/gc_hook.hpp"

namespace raven::gc {

void track_array_auto(const ArrayPtr& arr) {
    if (arr) {
        GCRegistry::instance().track_array(arr);
    }
}

void track_map_auto(const MapPtr& map) {
    if (map) {
        GCRegistry::instance().track_map(map);
    }
}

// Compilation unit anchor for cycle collector singleton
size_t trigger_cycle_collection() {
    return GCRegistry::instance().collect();
}

} // namespace raven::gc
