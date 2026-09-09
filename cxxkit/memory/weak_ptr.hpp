/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2026~Present ChengXueWen.
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

#include <memory>
#include <atomic>

CXXKIT_BEGIN_NAMESPACE

/**
 * @brief Non-intrusive weak reference to an owner whose lifetime is governed by a WeakPtrFactory.
 *
 * A WeakPtr does not keep its owner alive. get() returns the owner's pointer while the
 * WeakPtrFactory that issued this WeakPtr is alive, and nullptr afterwards.
 *
 * @warning Thread-safe invalidation checks only. The owner pointer itself must not be
 *          dereferenced concurrently with owner destruction (same contract as Chromium WeakPtr).
 */
template <class T>
class WeakPtr
{
public:
    /** @brief Constructs an empty WeakPtr (get() == nullptr). */
    WeakPtr()
        : mOwner(nullptr)
    {
    }

    /** @brief Copy constructor — shares the invalidation flag with @p other. */
    WeakPtr(const WeakPtr &other)
        : mOwner(other.mOwner)
        , mReference(other.mReference)
    {
    }

    /** @brief Move constructor — takes the reference from @p other, leaves it empty. */
    WeakPtr(WeakPtr &&other)
        : mOwner(other.mOwner)
        , mReference(std::move(other.mReference))
    {
        other.mOwner = nullptr;
    }

    /** @brief Copy assignment. */
    WeakPtr &operator=(const WeakPtr &other)
    {
        if (this != &other)
        {
            mOwner = other.mOwner;
            mReference = other.mReference;
        }
        return *this;
    }

    /** @brief Move assignment. */
    WeakPtr &operator=(WeakPtr &&other)
    {
        if (this != &other)
        {
            mOwner = other.mOwner;
            mReference = std::move(other.mReference);
            other.mOwner = nullptr;
        }
        return *this;
    }

    /**
     * @brief Returns the owner pointer while its factory is alive, nullptr otherwise.
     *
     * Order matters: weak_ptr::lock() first (keeps the flag block alive), then the
     * atomic flag read.
     */
    T *get() const
    {
        std::shared_ptr<std::atomic<bool>> reference = mReference.lock();
        if (reference && !reference->load(std::memory_order_acquire))
        {
            return mOwner;
        }
        return nullptr;
    }

    /** @brief Clears this WeakPtr (get() == nullptr afterwards). */
    void reset()
    {
        mOwner = nullptr;
        mReference.reset();
    }

    /** @brief Returns true if get() would return a non-null pointer. */
    explicit operator bool() const { return get() != nullptr; }

private:
    template <class U>
    friend class WeakPtrFactory;

    explicit WeakPtr(T *owner, std::weak_ptr<std::atomic<bool>> reference)
        : mOwner(owner)
        , mReference(reference)
    {
    }

    T *mOwner;
    std::weak_ptr<std::atomic<bool>> mReference;
};

/**
 * @brief Creates and invalidates WeakPtr<T> references to an @p owner it does not own.
 *
 * @note Lifetime contract (Chromium shape): the factory must be a member of — or otherwise
 *       outlive — the owner. If the owner dies before the factory, get() dangles; destroying
 *       the factory first is what invalidates the WeakPtrs.
 *
 * Not copyable.
 */
template <class T>
class WeakPtrFactory
{
public:
    CXXKIT_DECLARE_DISABLE_COPY(WeakPtrFactory)

    /** @brief Constructs a factory bound to @p owner (not owned). */
    explicit WeakPtrFactory(T *owner)
        : mOwner(owner)
        , mReference(new std::atomic<bool>(false))
    {
    }

    /** @brief Destructor — invalidates every WeakPtr issued by this factory. */
    ~WeakPtrFactory() { mReference->store(true, std::memory_order_release); }

    /** @brief Returns a WeakPtr observing @p owner. */
    WeakPtr<T> get_weak_ptr() const { return WeakPtr<T>(mOwner, mReference); }

private:
    T *mOwner;
    std::shared_ptr<std::atomic<bool>> mReference;
};

CXXKIT_END_NAMESPACE
