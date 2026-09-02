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

#include <condition_variable>
#include <cstddef>
#include <mutex>

CXXKIT_BEGIN_NAMESPACE

/**
 * @class Barrier
 * @brief N-thread synchronization barrier (C++11 implementation of C++20 std::barrier).
 *
 * Port from absl::Barrier. Threads call arrive_and_wait() to synchronize;
 * all N threads must arrive before any can proceed. Supports multiple phases.
 *
 * @code
 *   Barrier barrier(4);
 *   // In each thread:
 *   barrier.arrive_and_wait();  // blocks until all 4 threads arrive
 * @endcode
 */
class Barrier
{
public:
    /**
     * @brief Constructs a barrier for @p num_threads threads.
     * @param num_threads Number of threads that must arrive before the barrier opens.
     */
    explicit Barrier(size_t num_threads)
        : mNumThreads(num_threads)
        , mCount(num_threads)
        , mPhase(0)
    {
    }

    ~Barrier() = default;

    Barrier(const Barrier &) = delete;
    Barrier &operator=(const Barrier &) = delete;

    /**
     * @brief Arrive and block until all threads have arrived.
     *
     * Decrements the counter. If this is the last thread to arrive,
     * resets the counter and wakes all waiting threads. Otherwise,
     * blocks until the phase changes.
     */
    void arrive_and_wait()
    {
        std::unique_lock<std::mutex> lock(mMutex);
        size_t current_phase = mPhase;
        --mCount;
        if (mCount == 0)
        {
            // Last thread to arrive: reset counter, advance phase, wake all.
            mCount = mNumThreads;
            ++mPhase;
            mCv.notify_all();
        }
        else
        {
            // Wait until phase changes.
            mCv.wait(lock, [this, current_phase]() { return mPhase != current_phase; });
        }
    }

    /**
     * @brief Arrive without waiting (for threads that are exiting).
     *
     * Decrements both the expected thread count and the current counter.
     * If this is the last thread, wakes all waiting threads.
     */
    void arrive_and_drop()
    {
        std::unique_lock<std::mutex> lock(mMutex);
        --mNumThreads;
        --mCount;
        if (mCount == 0)
        {
            mCount = mNumThreads;
            ++mPhase;
            mCv.notify_all();
        }
    }

private:
    std::mutex mMutex;
    std::condition_variable mCv;
    size_t mNumThreads;
    size_t mCount;
    size_t mPhase;
};

CXXKIT_END_NAMESPACE
