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
#include <cxxkit/tools/assert.hpp>

#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <pthread.h>

CXXKIT_BEGIN_NAMESPACE

class Mutex : public std::mutex
{
public:
    using Base = std::mutex;
    using Lock = std::lock_guard<Base>;
    using UniqueLock = std::unique_lock<Base>;
    using Condition = std::condition_variable;

    using Base::Base;
    Mutex() = default;
    ~Mutex() = default;
};

class RecursiveMutex : public std::recursive_mutex
{
public:
    using Base = std::recursive_mutex;
    using Lock = std::lock_guard<Base>;
    using UniqueLock = std::unique_lock<Base>;
    using Condition = std::condition_variable_any;

    using Base::Base;
    RecursiveMutex() = default;
    ~RecursiveMutex() = default;
};

/// @brief RAII lock guard for Mutex (constructs lock, destructs unlock).
class MutexLock
{
public:
    explicit MutexLock(Mutex &mutex)
        : mMutex(mutex)
    {
        mMutex.lock();
    }
    ~MutexLock() { mMutex.unlock(); }

    MutexLock(const MutexLock &) = delete;
    MutexLock &operator=(const MutexLock &) = delete;

private:
    Mutex &mMutex;
};

/// @brief Reader-writer lock using pthread_rwlock.
class RWLock
{
public:
    RWLock() { pthread_rwlock_init(&mRwlock, nullptr); }
    ~RWLock() { pthread_rwlock_destroy(&mRwlock); }

    RWLock(const RWLock &) = delete;
    RWLock &operator=(const RWLock &) = delete;

    void read_lock() { pthread_rwlock_rdlock(&mRwlock); }
    void write_lock() { pthread_rwlock_wrlock(&mRwlock); }
    void unlock() { pthread_rwlock_unlock(&mRwlock); }

private:
    pthread_rwlock_t mRwlock;
};

/// @brief RAII read lock guard for RWLock.
class ReadLock
{
public:
    explicit ReadLock(RWLock &rw)
        : mRw(rw)
    {
        mRw.read_lock();
    }
    ~ReadLock() { mRw.unlock(); }

    ReadLock(const ReadLock &) = delete;
    ReadLock &operator=(const ReadLock &) = delete;

private:
    RWLock &mRw;
};

/// @brief RAII write lock guard for RWLock.
class WriteLock
{
public:
    explicit WriteLock(RWLock &rw)
        : mRw(rw)
    {
        mRw.write_lock();
    }
    ~WriteLock() { mRw.unlock(); }

    WriteLock(const WriteLock &) = delete;
    WriteLock &operator=(const WriteLock &) = delete;

private:
    RWLock &mRw;
};

CXXKIT_END_NAMESPACE
