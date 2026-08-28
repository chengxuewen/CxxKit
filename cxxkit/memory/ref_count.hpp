/***********************************************************************************************************************
**
** Library: CxxKit
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

#pragma once

#include <cxxkit/base/global.hpp>

#include <atomic>

CXXKIT_BEGIN_NAMESPACE
// Refcounted objects should implement the following informal interface:
//
// void addRef() const ;
// RefCountReleaseStatus Release() const;
//
// You may access members of a reference-counted object, including the addRef()
// and Release() methods, only if you already own a reference to it, or if
// you're borrowing someone else's reference. (A newly created object is a
// special case: the reference count is zero on construction, and the code that
// creates the object should immediately call addRef(), bringing the reference
// count from zero to one, e.g., by constructing an SharedRefPtr).
//
// addRef() creates a new reference to the object.
//
// Release() releases a reference to the object; the caller now has one less
// reference than before the call. Returns kDroppedLastRef if the number of
// references dropped to zero because of this (in which case the object destroys
// itself). Otherwise, returns kOtherRefsRemained, to signal that at the precise
// time the caller's reference was dropped, other references still remained (but
// if other threads own references, this may of course have changed by the time
// Release() returns).
//
// The caller of Release() must treat it in the same way as a delete operation:
// Regardless of the return value from Release(), the caller mustn't access the
// object. The object might still be alive, due to references held by other
// users of the object, but the object can go away at any time, e.g., as the
// result of another thread calling Release().
//
// Calling addRef() and Release() manually is discouraged. It's recommended to
// use SharedRefPtr to manage all pointers to reference counted objects.
// Note that SharedRefPtr depends on compile-time duck-typing; formally
// implementing the below RefCountInterface is not required.

enum class RefCountReleaseStatus
{
    kDroppedLastRef,
    kOtherRefsRemained
};

// Interfaces where refcounting is part of the public api should
// inherit this abstract interface. The implementation of these
// methods is usually provided by the RefCountedObject template class,
// applied as a leaf in the inheritance tree.
class RefCountInterface
{
public:
    virtual void addRef() const = 0;
    virtual RefCountReleaseStatus Release() const = 0;

    // Non-public destructor, because Release() has exclusive responsibility for
    // destroying the object.
protected:
    virtual ~RefCountInterface() { }
};

namespace detail
{
class RefCounter
{
public:
    explicit RefCounter(int ref_count)
        : mRefCount(ref_count)
    {
    }
    RefCounter() = delete;

    void incRef()
    {
        // Relaxed memory order: The current thread is allowed to act on the
        // resource protected by the reference counter both before and after the
        // atomic op, so this function doesn't prevent memory access reordering.
        mRefCount.fetch_add(1, std::memory_order_relaxed);
    }

    // Returns kDroppedLastRef if this call dropped the last reference; the caller
    // should therefore free the resource protected by the reference counter.
    // Otherwise, returns kOtherRefsRemained (note that in case of multithreading,
    // some other caller may have dropped the last reference by the time this call
    // returns; all we know is that we didn't do it).
    RefCountReleaseStatus DecRef()
    {
        // Use release-acquire barrier to ensure all actions on the protected
        // resource are finished before the resource can be freed.
        // When ref_count_after_subtract > 0, this function require
        // std::memory_order_release part of the barrier.
        // When ref_count_after_subtract == 0, this function require
        // std::memory_order_acquire part of the barrier.
        // In addition std::memory_order_release is used for synchronization with
        // the HasOneRef function to make sure all actions on the protected resource
        // are finished before the resource is assumed to have exclusive access.
        int ref_count_after_subtract = mRefCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        return ref_count_after_subtract == 0 ? RefCountReleaseStatus::kDroppedLastRef
                                             : RefCountReleaseStatus::kOtherRefsRemained;
    }

    // Return whether the reference count is one. If the reference count is used
    // in the conventional way, a reference count of 1 implies that the current
    // thread owns the reference and no other thread shares it. This call performs
    // the test for a reference count of one, and performs the memory barrier
    // needed for the owning thread to act on the resource protected by the
    // reference counter, knowing that it has exclusive access.
    bool HasOneRef() const
    {
        // To ensure resource protected by the reference counter has exclusive
        // access, all changes to the resource before it was released by other
        // threads must be visible by current thread. That is provided by release
        // (in DecRef) and acquire (in this function) ordering.
        return mRefCount.load(std::memory_order_acquire) == 1;
    }

private:
    std::atomic<int> mRefCount;
};
} // namespace detail

class RefCountedBase
{
public:
    RefCountedBase() = default;

    RefCountedBase(const RefCountedBase &) = delete;
    RefCountedBase &operator=(const RefCountedBase &) = delete;

    void addRef() const { mRefCount.incRef(); }
    RefCountReleaseStatus Release() const
    {
        const auto status = mRefCount.DecRef();
        if (status == RefCountReleaseStatus::kDroppedLastRef)
        {
            delete this;
        }
        return status;
    }

protected:
    // Provided for internal webrtc subclasses for corner cases where it's
    // necessary to know whether or not a reference is exclusively held.
    bool HasOneRef() const { return mRefCount.HasOneRef(); }

    virtual ~RefCountedBase() = default;

private:
    mutable detail::RefCounter mRefCount{0};
};

// Template based version of `RefCountedBase` for simple implementations that do
// not need (or want) destruction via virtual destructor or the overhead of a
// vtable.
//
// To use:
//   struct MyInt : public rtc::RefCountedNonVirtual<MyInt>  {
//     int foo_ = 0;
//   };
//
//   SharedRefPtr<MyInt> my_int(new MyInt());
//
// sizeof(MyInt) on a 32 bit system would then be 8, int + refcount and no
// vtable generated.
template <typename T>
class RefCountedNonVirtual
{
public:
    RefCountedNonVirtual() = default;

    RefCountedNonVirtual(const RefCountedNonVirtual &) = delete;
    RefCountedNonVirtual &operator=(const RefCountedNonVirtual &) = delete;

    void addRef() const { mRefCount.incRef(); }
    RefCountReleaseStatus Release() const
    {
        // If you run into this assert, T has virtual methods. There are two
        // options:
        // 1) The class doesn't actually need virtual methods, the type is complete
        //    so the virtual attribute(s) can be removed.
        // 2) The virtual methods are a part of the design of the class. In this
        //    case you can consider using `RefCountedBase` instead or alternatively
        //    use `rtc::RefCountedObject`.
        static_assert(!std::is_polymorphic<T>::value, "T has virtual methods. RefCountedBase is a better fit.");
        const auto status = mRefCount.DecRef();
        if (status == RefCountReleaseStatus::kDroppedLastRef)
        {
            delete static_cast<const T *>(this);
        }
        return status;
    }

protected:
    // Provided for internal webrtc subclasses for corner cases where it's
    // necessary to know whether or not a reference is exclusively held.
    bool HasOneRef() const { return mRefCount.HasOneRef(); }

    ~RefCountedNonVirtual() = default;

private:
    mutable detail::RefCounter mRefCount{0};
};

CXXKIT_END_NAMESPACE

