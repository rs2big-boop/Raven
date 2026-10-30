#include "raven/runtime/memory/cycle_collector.hpp"

namespace raven::gc {

// Compilation unit anchor for cycle collector singleton
size_t trigger_cycle_collection() {
    return GCRegistry::instance().collect();
}

} // namespace raven::gc
