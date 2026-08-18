/***********************************************************************************************************************
**
** Library: cxxkit
**
** Copyright (C) 2025~Present ChengXueWen.
**
** License: MIT License
**
** Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
** documentation files (the "Software"), to deal in the Software without restriction, including without limitation
** the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
** and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all copies or substantial portions
** of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
** TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
** THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
** CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
**
***********************************************************************************************************************/

#ifndef _CXXKIT_REF_COUNTED_OBJECT_HPP
#define _CXXKIT_REF_COUNTED_OBJECT_HPP

#include <cxxkit/memory/shared_ref_ptr.hpp>
#include <cxxkit/memory/nullability.hpp>
#include <cxxkit/memory/ref_count.hpp>

CXXKIT_BEGIN_NAMESPACE

template <class T>
class RefCountedObject : public T
{
public:
    RefCountedObject() { }

    RefCountedObject(const RefCountedObject &) = delete;
    RefCountedObject &operator=(const RefCountedObject &) = delete;

    template <class P0>
    explicit RefCountedObject(P0 &&p0)
        : T(std::forward<P0>(p0))
    {
    }

    template <class P0, class P1, class... Args>
    RefCountedObject(P0 &&p0, P1 &&p1, Args &&...args)
        : T(std::forward<P0>(p0), std::forward<P1>(p1), std::forward<Args>(args)...)
    {
    }

    void addRef() const override { mRefCount.incRef(); }

    RefCountReleaseStatus Release() const override
    {
        const auto status = mRefCount.DecRef();
        if (status == RefCountReleaseStatus::kDroppedLastRef)
        {
            delete this;
        }
        return status;
    }

    // Return whether the reference count is one. If the reference count is used
    // in the conventional way, a reference count of 1 implies that the current
    // thread owns the reference and no other thread shares it. This call
    // performs the test for a reference count of one, and performs the memory
    // barrier needed for the owning thread to act on the object, knowing that it
    // has exclusive access to the object.
    virtual bool HasOneRef() const { return mRefCount.HasOneRef(); }

protected:
    ~RefCountedObject() override { }

    mutable detail::RefCounter mRefCount{0};
};

template <class T>
class FinalRefCountedObject final : public T
{
public:
    using T::T;
    // Above using declaration propagates a default move constructor
    // FinalRefCountedObject(FinalRefCountedObject&& other), but we also need
    // move construction from T.
    explicit FinalRefCountedObject(T &&other)
        : T(std::move(other))
    {
    }
    FinalRefCountedObject(const FinalRefCountedObject &) = delete;
    FinalRefCountedObject &operator=(const FinalRefCountedObject &) = delete;

    void addRef() const { ref_count_.incRef(); }
    RefCountReleaseStatus Release() const
    {
        const auto status = ref_count_.DecRef();
        if (status == RefCountReleaseStatus::kDroppedLastRef)
        {
            delete this;
        }
        return status;
    }
    bool HasOneRef() const { return ref_count_.HasOneRef(); }

private:
    ~FinalRefCountedObject() = default;

    mutable detail::RefCounter ref_count_{0};
};

namespace utils
{

namespace detail
{
// Determines if the given class has addRef and Release methods.
template <typename T>
class HasaddRefAndRelease
{
private:
    template <typename C,
              decltype(std::declval<C>().addRef()) * = nullptr,
              decltype(std::declval<C>().Release()) * = nullptr>
    static int Test(int);
    template <typename>
    static char Test(...);

public:
    static constexpr bool value = std::is_same<decltype(Test<T>(0)), int>::value;
};
} // namespace detail

// General utilities for constructing a reference counted class and the
// appropriate reference count implementation for that class.
//
// These utilities select either the `RefCountedObject` implementation or
// `FinalRefCountedObject` depending on whether the to-be-shared class is
// derived from the RefCountInterface interface or not (respectively).

// `makeRefCounted`:
//
// Use this when you want to construct a reference counted object of type T and
// get a `SharedRefPtr<>` back. Example:
//
//   auto p = makeRefCounted<Foo>("bar", 123);
//
// For a class that inherits from RefCountInterface, this is equivalent to:
//
//   auto p = SharedRefPtr<Foo>(new RefCountedObject<Foo>("bar", 123));
//
// If the class does not inherit from RefCountInterface, but does have
// addRef/Release methods (so a T* is convertible to rtc::SharedRefPtr), this
// is equivalent to just
//
//   auto p = SharedRefPtr<Foo>(new Foo("bar", 123));
//
// Otherwise, the example is equivalent to:
//
//   auto p = SharedRefPtr<FinalRefCountedObject<Foo>>(
//       new FinalRefCountedObject<Foo>("bar", 123));
//
// In these cases, `makeRefCounted` reduces the amount of boilerplate code but
// also helps with the most commonly intended usage of RefCountedObject whereby
// methods for reference counting, are virtual and designed to satisfy the need
// of an interface. When such a need does not exist, it is more efficient to use
// the `FinalRefCountedObject` template, which does not add the vtable overhead.
//
// Note that in some cases, using RefCountedObject directly may still be what's
// needed.

// `makeRefCounted` for abstract classes that are convertible to
// RefCountInterface. The is_abstract requirement rejects classes that inherit
// both RefCountInterface and RefCounted object, which is a a discouraged
// pattern, and would result in double inheritance of RefCountedObject if this
// template was applied.
template <typename T,
          typename... Args,
          typename std::enable_if<std::is_convertible<T *, RefCountInterface *>::value && std::is_abstract<T>::value,
                                  T>::type * = nullptr>
Nonnull<SharedRefPtr<T>> makeRefCounted(Args &&...args)
{
    return SharedRefPtr<T>(new RefCountedObject<T>(std::forward<Args>(args)...));
}

// `makeRefCounted` for complete classes that are not convertible to
// RefCountInterface and already carry a ref count.
template <typename T,
          typename... Args,
          typename std::enable_if<!std::is_convertible<T *, RefCountInterface *>::value &&
                                      detail::HasaddRefAndRelease<T>::value,
                                  T>::type * = nullptr>
Nonnull<SharedRefPtr<T>> makeRefCounted(Args &&...args)
{
    return SharedRefPtr<T>(new T(std::forward<Args>(args)...));
}

// `makeRefCounted` for complete classes that are not convertible to
// RefCountInterface and have no ref count of their own.
template <typename T,
          typename... Args,
          typename std::enable_if<!std::is_convertible<T *, RefCountInterface *>::value &&
                                      !detail::HasaddRefAndRelease<T>::value,
                                  T>::type * = nullptr>
Nonnull<SharedRefPtr<FinalRefCountedObject<T>>> makeRefCounted(Args &&...args)
{
    return SharedRefPtr<FinalRefCountedObject<T>>(new FinalRefCountedObject<T>(std::forward<Args>(args)...));
}
} // namespace utils

CXXKIT_END_NAMESPACE

#endif // _CXXKIT_REF_COUNTED_OBJECT_HPP
