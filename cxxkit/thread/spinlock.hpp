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
#include <thread>

CXXKIT_BEGIN_NAMESPACE

class SpinLock
{
public:
    class Locker
    {
    public:
        Locker(SpinLock &lock)
            : spin_lock(lock)
        {
            spin_lock.lock();
        }
        virtual ~Locker() { spin_lock.unlock(); }
        virtual void relock() { spin_lock.lock(); }
        virtual void unlock() { spin_lock.unlock(); }
        virtual bool is_locked() { return spin_lock.is_locked(); }

    private:
        SpinLock &spin_lock;
        CXXKIT_DISABLE_COPY_MOVE(Locker)
    };

    SpinLock() { }
    virtual ~SpinLock() { }

    void lock()
    {
        int spinCount = 100;
        while (--spinCount > 0)
        {
            if (!mFlag.load(std::memory_order_relaxed)) // check
            {
                if (!mFlag.exchange(true, std::memory_order_acquire)) // try lock
                {
                    return;
                }
            }
        }

        while (mFlag.exchange(true, std::memory_order_acquire)) // spin lock
        {
#if CXXKIT_CC_CPP20_OR_GREATER
            mFlag.wait(true, std::memory_order_acquire);
#else
            std::this_thread::yield();
#endif
        }
    }
    void unlock()
    {
        mFlag.store(false, std::memory_order_release);
#if CXXKIT_CC_CPP20_OR_GREATER
        mFlag.notify_one();
#endif
    }
    bool is_locked() const { return mFlag.load(std::memory_order_relaxed); }

private:
    std::atomic<bool> mFlag{false};
    CXXKIT_DISABLE_COPY_MOVE(SpinLock)
};

CXXKIT_END_NAMESPACE
