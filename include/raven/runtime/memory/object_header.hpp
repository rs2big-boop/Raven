#pragma once
#include <cstdint>
#include <cstddef>
#include "managed_object.hpp"

namespace raven::runtime::memory {

// Sentinel value placed at start of every heap allocation
static constexpr uint32_t OBJECT_HEADER_MAGIC = 0xCAFEB00Bu;

struct ObjectHeader {
    uint32_t   magic;               // Must equal OBJECT_HEADER_MAGIC
    uint32_t   alloc_size;          // Total bytes allocated (including header)
    ObjectType type_tag;            // Object type tag
    GCColor    gc_color;            // Current GC color
    uint8_t    _padding[2];
    uint32_t   ref_count_snapshot;  // Last known ref count (updated on dec_ref)

    static void init(ObjectHeader* hdr, size_t alloc_size, ObjectType type) noexcept {
        hdr->magic              = OBJECT_HEADER_MAGIC;
        hdr->alloc_size         = static_cast<uint32_t>(alloc_size);
        hdr->type_tag           = type;
        hdr->gc_color           = GCColor::Black;
        hdr->_padding[0]        = 0;
        hdr->_padding[1]        = 0;
        hdr->ref_count_snapshot = 1;
    }

    static bool is_valid(const ObjectHeader* hdr) noexcept {
        return hdr != nullptr && hdr->magic == OBJECT_HEADER_MAGIC;
    }
};

} // namespace raven::runtime::memory
