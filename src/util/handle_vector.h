#pragma once

#include <MemoryMgr.h>
#include <type_traits>
#include <utility>
#include <string.h>

namespace Executor
{

template<typename T, typename TypedHandle = GUEST<GUEST<T>*>*, size_t offset = 0>
class handle_vector
{
    static_assert(std::is_trivially_copyable_v<T>);
    Handle handle_ = nullptr;
    size_t size_ = 0;
    size_t capacity_ = 0;

    static constexpr size_t elem_size = sizeof(GUEST<T>);
public:
    handle_vector() noexcept = default;
    explicit handle_vector(TypedHandle h) noexcept
        : handle_((Handle) h)
        , size_((GetHandleSize(handle_) - offset) / elem_size)
    {
    }
    ~handle_vector()
    {
        if(handle_)
            DisposeHandle(handle_);
    }

    friend void swap(handle_vector& a, handle_vector& b) noexcept
    {
        std::swap(a.handle_, b.handle_);
        std::swap(a.size_, b.size_);
        std::swap(a.capacity_, b.capacity_);
    }

    handle_vector(const handle_vector& other)
    {
        if(other.handle_)
        {
            handle_ = NewHandle(offset + elem_size * other.size_);
            size_ = other.size_;
            capacity_ = other.size_;
            memcpy(*handle_, *other.handle_, offset + other.size_ * elem_size);
        }
    }

    handle_vector(handle_vector&& other) noexcept
    {
        swap(*this, other);
    }

    handle_vector& operator=(const handle_vector& other)
    {
        return (*this = handle_vector(other));
    }
    handle_vector& operator=(handle_vector&& other)
    {
        handle_vector moved(std::move(other));
        swap(*this, moved);
        return *this;
    }

    TypedHandle get() const noexcept
    {
        return (TypedHandle)handle_;
    }

    TypedHandle release() noexcept
    {
        if(!handle_)
            return (TypedHandle) NewHandleClear(offset);
        
        if(capacity_ != size_)
            SetHandleSize(handle_, offset + size_ * elem_size);
        TypedHandle h = (TypedHandle) handle_;
        handle_ = nullptr;
        size_ = capacity_ = 0;
        return h;
    }

    size_t size() const noexcept { return size_; }
    size_t capacity() const noexcept { return capacity_; }
    
    void reserve(size_t cap)
    {
        if(cap > capacity_)
        {
            capacity_ = cap;
            if(handle_)
                SetHandleSize(handle_, offset + capacity_ * elem_size);
            else
            {
                handle_ = NewHandle(offset + capacity_ * elem_size);
                if(offset)
                    memset(*handle_, 0, offset);
            }
        }
    }

    using pointer = GUEST<T>*;

    pointer data() const noexcept { return handle_ ? reinterpret_cast<pointer>(*handle_ + offset) : nullptr; }
    pointer begin() const noexcept { return data(); }
    pointer end() const noexcept { return data() + size_; }
    GUEST<T>& operator[] (size_t idx) const noexcept { return *begin()[idx]; }

    void push_back(T x)
    {
        if(size_ == capacity_)
            reserve(capacity_ + 32 + capacity_/10);
        new(data() + size_) GUEST<T>(x);
        ++size_;
    }
};

}
