#pragma once

#include "raven/runtime/memory/cycle_collector.hpp"
#include "raven/runtime/memory/memory_statistics.hpp"

namespace raven {
namespace memory {

class MemoryManager {
public:
    static MemoryManager& instance() {
        static MemoryManager mgr;
        return mgr;
    }

    size_t collect_cycles() {
        return gc::GCRegistry::instance().collect();
    }

    MemoryStatistics statistics() const {
        auto gc_stats = gc::GCRegistry::instance().stats();
        MemoryStatistics stats;
        stats.total_collections = gc_stats.total_collections;
        stats.cycles_broken = gc_stats.cycles_broken;
        stats.objects_freed = gc_stats.objects_freed;
        stats.roots_traced = gc_stats.roots_traced;
        return stats;
    }

private:
    MemoryManager() = default;
};

} // namespace memory
} // namespace raven
