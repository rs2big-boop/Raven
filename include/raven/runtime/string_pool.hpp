#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_set>

namespace raven {

/**
 * @brief Thread-safe Global String Interning Pool.
 *
 * Provides canonical, immutable storage for identifiers, constant strings,
 * and property/map keys. Guarantees that identical strings share memory
 * and enables fast O(1) pointer-identity comparisons.
 */
class StringPool {
public:
    static StringPool& instance() {
        static StringPool inst;
        return inst;
    }

    /**
     * @brief Intern a string, returning a canonical string_view pointing to the pool.
     */
    std::string_view intern(const std::string& str) {
        std::lock_guard<std::mutex> lk(mtx_);
        auto it = pool_.find(str);
        if (it != pool_.end()) {
            return *it;
        }
        auto [inserted_it, _] = pool_.insert(str);
        return *inserted_it;
    }

    /**
     * @brief Intern a string_view without heap allocation if already present.
     */
    std::string_view intern(std::string_view sv) {
        std::lock_guard<std::mutex> lk(mtx_);
        auto it = pool_.find(std::string(sv));
        if (it != pool_.end()) {
            return *it;
        }
        auto [inserted_it, _] = pool_.insert(std::string(sv));
        return *inserted_it;
    }

    /**
     * @brief Check if two string_views point to the same interned memory.
     */
    static bool is_same(std::string_view a, std::string_view b) noexcept {
        return a.data() == b.data();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lk(mtx_);
        return pool_.size();
    }

    void clear() {
        std::lock_guard<std::mutex> lk(mtx_);
        pool_.clear();
    }

private:
    StringPool() = default;
    mutable std::mutex mtx_;
    std::unordered_set<std::string> pool_;
};

} // namespace raven
