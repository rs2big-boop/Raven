#pragma once

#include <cstdint>
#include <vector>
#include <optional>

namespace raven {

/**
 * @brief Bitmaps identifying exact GC reference roots at a specific safepoint (ADR 0008.1 §5.2).
 */
struct StackMap {
    uint32_t bytecode_offset;
    std::vector<bool> operand_stack_bitmap; // true if operand stack slot holds GC pointer
    std::vector<bool> local_slots_bitmap;   // true if local slot holds GC pointer
};

/**
 * @brief Table of StackMaps generated per bytecode chunk by the compiler.
 */
class StackMapTable {
public:
    void add_map(StackMap map) {
        maps_.push_back(std::move(map));
    }

    const std::vector<StackMap>& maps() const noexcept {
        return maps_;
    }

    const StackMap* find_map(uint32_t bytecode_offset) const noexcept {
        for (const auto& map : maps_) {
            if (map.bytecode_offset == bytecode_offset) {
                return &map;
            }
        }
        return nullptr;
    }

    bool empty() const noexcept {
        return maps_.empty();
    }

    size_t size() const noexcept {
        return maps_.size();
    }

    void clear() {
        maps_.clear();
    }

private:
    std::vector<StackMap> maps_;
};

} // namespace raven
