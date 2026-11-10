#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <mutex>
#include <algorithm>
#include "object_header.hpp"

namespace raven::runtime::memory {

enum class Generation : uint8_t {
    Young = 0,
    Old   = 1
};

static constexpr size_t CARD_SHIFT = 9; // 2^9 = 512 bytes per card
static constexpr size_t CARD_SIZE  = 1 << CARD_SHIFT;
static constexpr uint8_t CARD_CLEAN = 0;
static constexpr uint8_t CARD_DIRTY = 1;

/**
 * @brief Generational Card Table tracking Old -> Young cross-generational pointers (ADR 0008.1 §6.1).
 */
class CardTable {
public:
    static CardTable& instance() {
        static CardTable ct;
        return ct;
    }

    void set_heap_bounds(uintptr_t start, uintptr_t end) {
        std::lock_guard<std::mutex> lock(mtx_);
        heap_start_ = start;
        heap_end_   = end;
        size_t num_cards = (end > start) ? ((end - start + CARD_SIZE - 1) >> CARD_SHIFT) : 0;
        cards_.assign(num_cards, CARD_CLEAN);
    }

    bool is_in_heap(const void* ptr) const noexcept {
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        return addr >= heap_start_ && addr < heap_end_;
    }

    size_t card_index(const void* ptr) const noexcept {
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        if (addr < heap_start_ || addr >= heap_end_) return static_cast<size_t>(-1);
        return (addr - heap_start_) >> CARD_SHIFT;
    }

    void mark_dirty(const void* old_obj) {
        size_t idx = card_index(old_obj);
        if (idx < cards_.size()) {
            cards_[idx] = CARD_DIRTY;
        }
    }

    bool is_dirty(size_t card_idx) const noexcept {
        return card_idx < cards_.size() && cards_[card_idx] == CARD_DIRTY;
    }

    void clear_card(size_t card_idx) noexcept {
        if (card_idx < cards_.size()) {
            cards_[card_idx] = CARD_CLEAN;
        }
    }

    void clear_all_cards() noexcept {
        std::fill(cards_.begin(), cards_.end(), CARD_CLEAN);
    }

    std::vector<size_t> get_dirty_cards() const {
        std::vector<size_t> dirty;
        for (size_t i = 0; i < cards_.size(); ++i) {
            if (cards_[i] == CARD_DIRTY) {
                dirty.push_back(i);
            }
        }
        return dirty;
    }

    size_t card_count() const noexcept { return cards_.size(); }

private:
    CardTable() : heap_start_(0), heap_end_(0) {}

    mutable std::mutex mtx_;
    uintptr_t heap_start_;
    uintptr_t heap_end_;
    std::vector<uint8_t> cards_;
};

/**
 * @brief Generational Write Barrier (ADR 0008.1 §6.1).
 * Marks the card dirty if an Old-generation object points to a Young-generation value.
 */
inline void write_barrier_generational(Generation source_gen, const void* source_obj,
                                       Generation target_gen, const void* target_obj) {
    (void)target_obj;
    if (source_gen == Generation::Old && target_gen == Generation::Young && source_obj != nullptr) {
        CardTable::instance().mark_dirty(source_obj);
    }
}

/**
 * @brief Incremental Marking Barrier (ADR 0008.1 §6.2).
 * Preserves the strong tri-color invariant during incremental marking passes.
 */
inline bool write_barrier_marking(GCColor source_color, GCColor& target_color,
                                  std::vector<void*>& mark_stack, void* target_obj) {
    if (source_color == GCColor::Black && target_color == GCColor::White) {
        target_color = GCColor::Gray;
        if (target_obj) {
            mark_stack.push_back(target_obj);
        }
        return true;
    }
    return false;
}

} // namespace raven::runtime::memory
