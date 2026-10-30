#pragma once

// Raven Garbage Collector — Phase 1: Cycle Collector Prototype
//
// Architecture (ADR-0007: Hybrid Memory Management):
//   Phase 1 (current): Reference Counting via std::shared_ptr (automatic).
//   Phase 2 (this file): Prototype cycle detector — identifies objects whose
//     shared_ptr ref-count is > 0 but are only reachable through cycles
//     (i.e., they are unreachable from any GC root).
//   Phase 3 (future): Custom allocator + generational tracing GC.
//
// How to use:
//   1. Register GC roots via GCRegistry::add_root() / remove_root().
//   2. Call GCRegistry::collect() periodically or on demand.
//   3. collect() traces reachable objects, identifies cycle-only objects,
//      and breaks cycles by clearing their internal references.
//
// NOTE: This is a research prototype. It does not replace std::shared_ptr
//   reference counting — it supplements it by breaking reference cycles
//   that would otherwise cause memory leaks.

#include <algorithm>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "raven/value.hpp"

namespace raven {
namespace gc {

// ----------------------------------------------------------------------------
// GCStats — diagnostic counters updated by each collect() pass
// ----------------------------------------------------------------------------
struct GCStats {
    size_t total_collections = 0;   // number of times collect() was called
    size_t cycles_broken    = 0;    // number of cyclic Value groups broken
    size_t objects_freed    = 0;    // approximate objects released (cycle members)
    size_t roots_traced     = 0;    // GC roots examined in last pass
};

// ----------------------------------------------------------------------------
// GCRegistry — singleton that tracks GC roots and performs cycle collection
// ----------------------------------------------------------------------------
class GCRegistry {
public:
    // Get the process-wide singleton registry.
    static GCRegistry& instance() {
        static GCRegistry inst;
        return inst;
    }

    // Add a root — a Value that is directly reachable (e.g. a global variable,
    // a stack frame's local). The root is identified by a stable pointer to the
    // Value object; the caller must call remove_root() before the Value is
    // destroyed.
    void add_root(const Value* root) {
        std::lock_guard<std::mutex> lk(mutex_);
        roots_.insert(root);
    }

    // Remove a previously registered root.
    void remove_root(const Value* root) {
        std::lock_guard<std::mutex> lk(mutex_);
        roots_.erase(root);
    }

    // Perform one cycle-detection pass.
    //
    // Algorithm (simplified Bacon-Rajan):
    //   1. Walk every registered root recursively.
    //   2. Record every unique ArrayPtr / MapPtr encountered.
    //   3. Among ALL known candidates (tracked via weak_ptr set), find those
    //      NOT reached from any root — those are cycle-only objects.
    //   4. Break cycles by clearing their contents.
    //
    // Returns the number of cycle groups broken.
    size_t collect() {
        std::lock_guard<std::mutex> lk(mutex_);

        stats_.total_collections++;
        stats_.roots_traced = roots_.size();

        // Step 1 & 2: traverse roots, collect reachable array/map raw pointers.
        std::unordered_set<const void*> reachable;
        for (const Value* root : roots_) {
            trace_value(*root, reachable);
        }

        // Step 3 & 4: find candidates not in reachable set and break them.
        size_t broken = 0;

        // Prune expired weak pointers and break unreachable cycles.
        {
            std::vector<std::weak_ptr<std::vector<Value>>*> dead_arr;
            for (auto& wp : tracked_arrays_) {
                auto sp = wp.lock();
                if (!sp) {
                    // already freed — nothing to do
                    continue;
                }
                const void* raw = sp.get();
                if (reachable.count(raw) == 0) {
                    // Only reachable through cycles — break the cycle.
                    sp->clear();
                    broken++;
                    stats_.objects_freed++;
                }
            }

            std::vector<std::weak_ptr<std::unordered_map<std::string, Value>>*> dead_map;
            for (auto& wp : tracked_maps_) {
                auto sp = wp.lock();
                if (!sp) {
                    continue;
                }
                const void* raw = sp.get();
                if (reachable.count(raw) == 0) {
                    sp->clear();
                    broken++;
                    stats_.objects_freed++;
                }
            }
        }

        // Prune expired weak pointers (housekeeping).
        tracked_arrays_.erase(
            std::remove_if(tracked_arrays_.begin(), tracked_arrays_.end(),
                [](const std::weak_ptr<std::vector<Value>>& wp) { return wp.expired(); }),
            tracked_arrays_.end());

        tracked_maps_.erase(
            std::remove_if(tracked_maps_.begin(), tracked_maps_.end(),
                [](const std::weak_ptr<std::unordered_map<std::string, Value>>& wp) { return wp.expired(); }),
            tracked_maps_.end());

        stats_.cycles_broken += broken;
        return broken;
    }

    // Track a newly allocated array so the GC can inspect it for cycles.
    void track_array(const ArrayPtr& arr) {
        std::lock_guard<std::mutex> lk(mutex_);
        tracked_arrays_.emplace_back(arr);
    }

    // Track a newly allocated map.
    void track_map(const MapPtr& map) {
        std::lock_guard<std::mutex> lk(mutex_);
        tracked_maps_.emplace_back(map);
    }

    // Return a copy of the current GC stats (thread-safe).
    GCStats stats() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return stats_;
    }

    // Reset stats counters (useful for testing).
    void reset_stats() {
        std::lock_guard<std::mutex> lk(mutex_);
        stats_ = {};
    }

private:
    GCRegistry() = default;

    // Recursively mark all arrays and maps reachable from a Value.
    void trace_value(const Value& val, std::unordered_set<const void*>& reachable) {
        if (val.is_array()) {
            const void* raw = val.as_array().get();
            if (reachable.insert(raw).second) {
                // newly seen — recurse into elements
                for (const Value& elem : *val.as_array()) {
                    trace_value(elem, reachable);
                }
            }
        } else if (val.is_map()) {
            const void* raw = val.as_map().get();
            if (reachable.insert(raw).second) {
                for (const auto& [k, v] : *val.as_map()) {
                    trace_value(v, reachable);
                }
            }
        }
        // Nil, bool, number, string, callable — no heap children to trace.
    }

    mutable std::mutex mutex_;
    std::unordered_set<const Value*> roots_;
    std::vector<std::weak_ptr<std::vector<Value>>>                          tracked_arrays_;
    std::vector<std::weak_ptr<std::unordered_map<std::string, Value>>>      tracked_maps_;
    GCStats stats_;
};

// ----------------------------------------------------------------------------
// RAII helper: ScopedRoot
// Registers a Value as a GC root for the duration of its lifetime.
// Use this for stack-local Values that may contain arrays/maps.
//
// Example:
//   Value arr = make_array(...);
//   gc::ScopedRoot root(arr);
//   // ... arr is now a GC root and won't be broken ...
// ----------------------------------------------------------------------------
class ScopedRoot {
public:
    explicit ScopedRoot(const Value& val) : val_(&val) {
        GCRegistry::instance().add_root(val_);
    }
    ~ScopedRoot() {
        GCRegistry::instance().remove_root(val_);
    }

    ScopedRoot(const ScopedRoot&) = delete;
    ScopedRoot& operator=(const ScopedRoot&) = delete;

private:
    const Value* val_;
};

} // namespace gc
} // namespace raven
