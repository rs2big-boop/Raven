#pragma once

#include <cstddef>

namespace raven {
namespace memory {

struct MemoryStatistics {
    size_t total_allocated_bytes = 0;
    size_t active_objects = 0;
    size_t total_collections = 0;
    size_t cycles_broken = 0;
    size_t objects_freed = 0;
    size_t roots_traced = 0;
};

} // namespace memory
} // namespace raven
