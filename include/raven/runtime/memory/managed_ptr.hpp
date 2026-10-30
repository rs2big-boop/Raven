#pragma once
#include "managed_object.hpp"
#include <utility>
#include <type_traits>

namespace raven::runtime::memory {

// RAII smart pointer for ManagedObject subclasses.
// Maintains reference counts automatically.
// Similar to shared_ptr but integrates with Raven's GC.
template<typename T>
class ManagedPtr {
    static_assert(std::is_base_of_v<ManagedObject, T>,
                  "T must derive from ManagedObject");
public:
    ManagedPtr() noexcept : ptr_(nullptr) {}

    explicit ManagedPtr(T* raw) noexcept : ptr_(raw) {
        // raw is assumed to have ref_count == 1 (from allocator)
    }

    ManagedPtr(const ManagedPtr& other) noexcept : ptr_(other.ptr_) {
        if (ptr_) ptr_->inc_ref();
    }

    ManagedPtr(ManagedPtr&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }

    ManagedPtr& operator=(const ManagedPtr& other) noexcept {
        if (this != &other) {
            reset();
            ptr_ = other.ptr_;
            if (ptr_) ptr_->inc_ref();
        }
        return *this;
    }

    ManagedPtr& operator=(ManagedPtr&& other) noexcept {
        if (this != &other) {
            reset();
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    ~ManagedPtr() { reset(); }

    void reset() noexcept {
        if (ptr_) {
            if (ptr_->dec_ref() == 0) {
                delete ptr_;
            }
            ptr_ = nullptr;
        }
    }

    T* get() const noexcept { return ptr_; }
    T& operator*() const noexcept { return *ptr_; }
    T* operator->() const noexcept { return ptr_; }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    bool operator==(const ManagedPtr& other) const noexcept { return ptr_ == other.ptr_; }
    bool operator!=(const ManagedPtr& other) const noexcept { return ptr_ != other.ptr_; }

private:
    T* ptr_;
};

// Convenience factory
template<typename T, typename... Args>
ManagedPtr<T> make_managed(Args&&... args) {
    return ManagedPtr<T>(new T(std::forward<Args>(args)...));
}

} // namespace raven::runtime::memory
