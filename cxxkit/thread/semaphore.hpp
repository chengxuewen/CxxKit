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

#pragma once

#include "cxxkit/tools/assert.hpp"
#include "cxxkit/tools/utility.hpp"

#include <thread>
#include <condition_variable>

CXXKIT_BEGIN_NAMESPACE

template <std::ptrdiff_t LeastMaxValue = std::numeric_limits<std::ptrdiff_t>::max()>
class CountingSemaphore
{
    // using Mutex = std::recursive_mutex;
    using Mutex = std::mutex;
    using Lock = std::lock_guard<Mutex>;
    using UniqueLock = std::unique_lock<Mutex>;
    // using Condition = std::condition_variable_any;
    using Condition = std::condition_variable;

public:
    class Releaser
    {
    public:
        Releaser() = default;
        explicit Releaser(CountingSemaphore &sem, int n = 1) noexcept
            : mSem(&sem)
            , mCount(n)
        {
        }
        explicit Releaser(CountingSemaphore *sem, int n = 1) noexcept
            : mSem(sem)
            , mCount(n)
        {
        }
        Releaser(Releaser &&other) noexcept
            : mSem(other.cancel())
            , mCount(other.mCount)
        {
        }
        Releaser &operator=(Releaser &&other) noexcept
        {
            Releaser moved(std::move(other));
            swap(moved);
            return *this;
        }

        ~Releaser()
        {
            if (mSem)
            {
                mSem->release(mCount);
            }
        }

        void swap(Releaser &other) noexcept
        {
            std::swap(mSem, other.mSem);
            std::swap(mCount, other.mCount);
        }

        CountingSemaphore *semaphore() const noexcept { return mSem; }

        CountingSemaphore *cancel() noexcept { return utils::exchange(mSem, nullptr); }

    private:
        CountingSemaphore *mSem = nullptr;
        int mCount;
    };

    explicit CountingSemaphore(std::ptrdiff_t desired = 0)
        : mCount(desired)
    {
        CXXKIT_ASSERT(desired >= 0 && desired <= max());
    }
    virtual ~CountingSemaphore() = default;

    static constexpr std::ptrdiff_t max() noexcept
    {
        static_assert(LeastMaxValue >= 0, "LeastMaxValue shall be non-negative");
        return LeastMaxValue;
    }

    void acquire(std::ptrdiff_t n = 1)
    {
        UniqueLock lock{mMutex};
        mCondition.wait(lock, [&]() { return mCount >= n; });
        mCount -= n;
    }

    bool tryAcquire(std::ptrdiff_t n = 1)
    {
        Lock lock{mMutex};
        if (mCount < n)
        {
            return false;
        }
        mCount -= n;
        return true;
    }
    CXXKIT_STATIC_CONSTANT_NUMBER(kWaitForeverMSecs, std::numeric_limits<unsigned int>::max())
    bool tryAcquire(std::ptrdiff_t n, unsigned int msecs)
    {
        return this->tryAcquireWait(n, std::chrono::steady_clock::now() + std::chrono::milliseconds(msecs));
    }

    template <typename Rep, typename Period>
    bool tryAcquireFor(std::ptrdiff_t n, const std::chrono::duration<Rep, Period> &relTime)
    {
        const auto absTime = std::chrono::steady_clock::now() + relTime;
        return this->tryAcquireWait(n, absTime);
    }

    template <typename Clock, typename Duration>
    bool tryAcquireUntil(std::ptrdiff_t n, const std::chrono::time_point<Clock, Duration> &absTime)
    {
        return this->tryAcquireWait(n, absTime);
    }

    void release(std::ptrdiff_t update = 1)
    {
        Lock lock{mMutex};
        CXXKIT_ASSERT(update >= 0 && update <= this->max() - mCount);
        mCount += update;
        mCondition.notify_all();
    }

    std::ptrdiff_t available() const
    {
        Lock lock{mMutex};
        return mCount;
    }


protected:
    template <typename Clock, typename Duration>
    bool tryAcquireWait(std::ptrdiff_t n, const std::chrono::time_point<Clock, Duration> &timeoutTime)
    {
        UniqueLock lock{mMutex};
        if (!mCondition.wait_until(lock, timeoutTime, [&]() { return mCount >= n; }))
        {
            return false;
        }
        mCount -= n;
        return true;
    }

private:
    mutable Mutex mMutex;
    Condition mCondition;
    std::ptrdiff_t mCount{0};
    CXXKIT_DISABLE_COPY_MOVE(CountingSemaphore)
};
using BinarySemaphore = CountingSemaphore<1>;
using Semaphore = CountingSemaphore<>;

CXXKIT_END_NAMESPACE
