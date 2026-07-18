#pragma once

namespace GammaTest {

template <typename T>
class RefCountPtr {
public:
    explicit RefCountPtr(T* p = nullptr) : p_(p) {}
    ~RefCountPtr() { reset(); }
    RefCountPtr(const RefCountPtr&) = delete;
    RefCountPtr& operator=(const RefCountPtr&) = delete;
    RefCountPtr(RefCountPtr&& o) noexcept : p_(o.p_) { o.p_ = nullptr; }
    RefCountPtr& operator=(RefCountPtr&& o) noexcept
    {
        if (this != &o) {
            reset();
            p_ = o.p_;
            o.p_ = nullptr;
        }
        return *this;
    }

    T* get() const { return p_; }
    T* operator->() const { return p_; }
    explicit operator bool() const { return p_ != nullptr; }

    void reset(T* p = nullptr)
    {
        if (p_) {
            p_->Release();
        }
        p_ = p;
    }

private:
    T* p_;
};

} // namespace GammaTest
