#pragma once

#include <cstdint>
#include <cstring>

namespace raven {
namespace runtime {

/**
 * @brief 64-bit NaN-Boxed Value representation (IEEE 754 Negative Quiet NaN tagging).
 *
 * Provides ultra-compact 8-byte representation for all Raven values:
 * - Double precision floats stored directly as native IEEE-754 numbers.
 * - Non-numbers tagged within the quiet NaN unallocated payload space.
 *
 * Memory layout:
 * [1111 1111 1111 1000] [Tag: 16] [Payload: 32 / 48]
 */
class NanBoxedValue {
public:
    static constexpr uint64_t TAG_PREFIX     = 0xFFF8000000000000ULL;
    static constexpr uint64_t CANON_NAN      = 0x7FF8000000000000ULL;

    static constexpr uint64_t VAL_NIL        = TAG_PREFIX | 1ULL;
    static constexpr uint64_t VAL_FALSE      = TAG_PREFIX | 2ULL;
    static constexpr uint64_t VAL_TRUE       = TAG_PREFIX | 3ULL;
    static constexpr uint64_t VAL_MOVED      = TAG_PREFIX | 4ULL;

    static constexpr uint64_t PTR_PREFIX     = 0xFFF9000000000000ULL;
    static constexpr uint64_t PTR_MASK       = 0x0000FFFFFFFFFFFFULL;

    constexpr NanBoxedValue() noexcept : bits_(VAL_NIL) {}

    explicit NanBoxedValue(double num) noexcept {
        std::memcpy(&bits_, &num, sizeof(double));
        // Normalize any negative quiet NaN to positive canonical quiet NaN
        if ((bits_ & TAG_PREFIX) == TAG_PREFIX) {
            bits_ = CANON_NAN;
        }
    }

    static NanBoxedValue from_bool(bool b) noexcept {
        NanBoxedValue v;
        v.bits_ = b ? VAL_TRUE : VAL_FALSE;
        return v;
    }

    static NanBoxedValue from_nil() noexcept {
        NanBoxedValue v;
        v.bits_ = VAL_NIL;
        return v;
    }

    static NanBoxedValue from_pointer(void* ptr) noexcept {
        NanBoxedValue v;
        v.bits_ = PTR_PREFIX | (reinterpret_cast<uint64_t>(ptr) & PTR_MASK);
        return v;
    }

    bool is_number() const noexcept {
        return (bits_ & TAG_PREFIX) != TAG_PREFIX;
    }

    bool is_nil() const noexcept {
        return bits_ == VAL_NIL;
    }

    bool is_bool() const noexcept {
        return bits_ == VAL_TRUE || bits_ == VAL_FALSE;
    }

    bool is_pointer() const noexcept {
        return (bits_ & 0xFFFF000000000000ULL) == PTR_PREFIX;
    }

    double as_number() const noexcept {
        double d;
        std::memcpy(&d, &bits_, sizeof(double));
        return d;
    }

    bool as_bool() const noexcept {
        return bits_ == VAL_TRUE;
    }

    void* as_pointer() const noexcept {
        return reinterpret_cast<void*>(bits_ & PTR_MASK);
    }

    uint64_t raw_bits() const noexcept {
        return bits_;
    }

    bool operator==(const NanBoxedValue& other) const noexcept {
        if (is_number() && other.is_number()) {
            return as_number() == other.as_number();
        }
        return bits_ == other.bits_;
    }

private:
    uint64_t bits_;
};

static_assert(sizeof(NanBoxedValue) == 8, "NanBoxedValue must be exactly 8 bytes");

} // namespace runtime
} // namespace raven
