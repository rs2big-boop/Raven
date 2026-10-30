#pragma once
#include <atomic>
#include <cstdint>
#include <string>

namespace raven::runtime::memory {

// GC colors for Bacon-Rajan cycle detection
enum class GCColor : uint8_t {
    Black  = 0,  // In use, ref count > 0
    Gray   = 1,  // Possible member of cycle
    White  = 2,  // Candidate for collection
    Purple = 3,  // Possible root of cycle
    Red    = 4,  // Currently in cycle list
    Orange = 5,  // In cycle list, awaiting epoch check
};

// Type tags for managed objects
enum class ObjectType : uint8_t {
    Unknown  = 0,
    String   = 1,
    Array    = 2,
    Map      = 3,
    Function = 4,
    Closure  = 5,
    Native   = 6,
};

// Base class for all GC-tracked objects.
// Uses Bacon-Rajan concurrent cycle detection.
class ManagedObject {
public:
    explicit ManagedObject(ObjectType type = ObjectType::Unknown)
        : ref_count_(1), gc_color_(GCColor::Black), type_(type),
          buffered_(false) {}

    virtual ~ManagedObject() = default;

    // Reference counting
    void inc_ref() noexcept { ref_count_.fetch_add(1, std::memory_order_relaxed); }
    uint32_t dec_ref() noexcept { return ref_count_.fetch_sub(1, std::memory_order_acq_rel) - 1; }
    uint32_t ref_count() const noexcept { return ref_count_.load(std::memory_order_relaxed); }

    // GC color
    GCColor color() const noexcept { return gc_color_; }
    void set_color(GCColor c) noexcept { gc_color_ = c; }

    // Type tag
    ObjectType type() const noexcept { return type_; }

    // Cycle detection: is this object in the purple buffer?
    bool buffered() const noexcept { return buffered_; }
    void set_buffered(bool b) noexcept { buffered_ = b; }

    // Subclasses override to visit all child references
    virtual void trace_children(void (*visitor)(ManagedObject*)) { (void)visitor; }

    // For debugging
    virtual std::string debug_name() const { return "ManagedObject"; }

protected:
    std::atomic<uint32_t> ref_count_;
    GCColor gc_color_;
    ObjectType type_;
    bool buffered_;
};

} // namespace raven::runtime::memory
