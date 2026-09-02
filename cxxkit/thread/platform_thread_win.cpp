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

#include <cxxkit/thread/detail/platform_thread_p.hpp>
#include <cxxkit/tools/exception.hpp>
#include <cxxkit/tools/logging.hpp>
#include <cxxkit/tools/assert.hpp>

#if defined(CXXKIT_OS_WIN)

#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    include <windows.h>
#    ifndef CXXKIT_OS_WINCE
#        ifndef _MT
#            define _MT
#        endif
#        include <process.h>
#    else
#    endif

CXXKIT_BEGIN_NAMESPACE

namespace detail
{
/* tls */
namespace tls
{
static DWORD currentThreadDataTLSIndex = TLS_OUT_OF_INDEXES;
void create_tls()
{
    if (TLS_OUT_OF_INDEXES == currentThreadDataTLSIndex)
    {
        static std::mutex mutex;
        std::lock_guard<std::mutex> lock(mutex);
        if (TLS_OUT_OF_INDEXES == currentThreadDataTLSIndex)
        {
            currentThreadDataTLSIndex = tls_alloc();
        }
    }
}
static void free_tls()
{
    if (TLS_OUT_OF_INDEXES != currentThreadDataTLSIndex)
    {
        tls_free(currentThreadDataTLSIndex);
        currentThreadDataTLSIndex = TLS_OUT_OF_INDEXES;
    }
}
CXXKIT_DESTRUCTOR_FUNCTION(free_tls)
} // namespace tls
// Utility functions for getting, setting and clearing thread specific data.
static PlatformThreadData *get_thread_data()
{
    return reinterpret_cast<PlatformThreadData *>(tls_get_value(tls::currentThreadDataTLSIndex));
}
static void set_thread_data(PlatformThreadData *data)
{
    tls_set_value(tls::currentThreadDataTLSIndex, data);
}
static void clear_thread_data()
{
    tls_set_value(tls::currentThreadDataTLSIndex, 0);
}

namespace thread
{
static void finish(void *arg, bool lockAnyway = true) noexcept
{
    auto threadPrivate = reinterpret_cast<PlatformThreadPrivate *>(arg);
    auto thread = PlatformThreadPrivate::get(threadPrivate);
    auto threadData = threadPrivate->mData;

    PlatformThreadPrivate::ThreadMutex::UniqueLock lock(threadPrivate->mMutex);
    if (!lockAnyway)
    {
        lock.unlock();
    }
    threadPrivate->mInFinish = true;
    threadPrivate->mPriority = PlatformThread::Priority::kInherit;
    // void **tls_data = reinterpret_cast<void **>(&d->data->tls);
    if (lockAnyway)
    {
        lock.unlock();
    }
    threadPrivate->on_finished();
    //QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    //QThreadStorageData::finish(tls_data);
    if (lockAnyway)
    {
        lock.lock();
    }

    // QAbstractEventDispatcher *eventDispatcher = d->data->eventDispatcher.loadRelaxed();
    // if (eventDispatcher) {
    //     d->data->eventDispatcher = 0;
    //     locker.unlock();
    //     eventDispatcher->closingDown();
    //     delete eventDispatcher;
    //     locker.relock();
    // }

    threadPrivate->mRunning = false;
    threadPrivate->mFinished = true;
    threadPrivate->mInterruptionRequested = false;

    threadData->thread_id.store(0);
    threadPrivate->mThreadHandle = 0;

    threadPrivate->mInFinish = false;
    threadPrivate->mDoneCondition.notify_all();
}
static unsigned int __stdcall start(void *arg) noexcept
{
    auto threadPrivate = reinterpret_cast<PlatformThreadPrivate *>(arg);
    auto thread = PlatformThreadPrivate::get(threadPrivate);
    auto threadData = threadPrivate->mData;

    tls::create_tls();
    set_thread_data(threadData);
    threadData->thread_id.store(PlatformThread::current_thread_id());

    PlatformThreadPrivate::set_termination_enabled(false);
    {
        {
            PlatformThreadPrivate::ThreadMutex::UniqueLock lock(threadPrivate->mMutex);
            threadData->quitNow = threadPrivate->mExited;
        }
        // data->ensureEventDispatcher();
        PlatformThread::set_current_thread_name(threadPrivate->mName.c_str());
        threadPrivate->on_started();
    }
    PlatformThreadPrivate::set_termination_enabled(true);

    threadPrivate->run();
    finish(arg);
    return 0;
}

static std::vector<PlatformThread *> adoptedPlatformThreads;
static std::vector<HANDLE> adoptedThreadHandles;
static std::mutex adoptedThreadWatcherMutex;
static DWORD adoptedThreadWatcherId = 0;
static HANDLE adoptedThreadWakeup = 0;
/*
    This function loops and waits for native adopted threads to finish.
    When this happens it derefs the QThreadData for the adopted thread
    to make sure it gets cleaned up properly.
*/
DWORD WINAPI adopted_thread_watcher_function(LPVOID)
{
    CXXKIT_FOREVER
    {
        std::unique_lock<std::mutex> lock(adoptedThreadWatcherMutex);
        if (adoptedThreadHandles.size() == 1)
        {
            adoptedThreadWatcherId = 0;
            break;
        }

        std::vector<HANDLE> handlesCopy = adoptedThreadHandles;
        lock.unlock();

        DWORD ret = WAIT_TIMEOUT;
        int count;
        int offset;
        int loops = handlesCopy.size() / MAXIMUM_WAIT_OBJECTS;
        if (handlesCopy.size() % MAXIMUM_WAIT_OBJECTS)
        {
            ++loops;
        }
        if (loops == 1)
        {
            // no need to loop, no timeout
            offset = 0;
            count = handlesCopy.size();
#    ifndef CXXKIT_OS_WINRT
            ret = wait_for_multiple_objects(handlesCopy.size(), handlesCopy.data(), false, INFINITE);
#    else
            ret = wait_for_multiple_objects_ex(handlesCopy.size(), handlesCopy.data(), false, INFINITE, false);
#    endif
        }
        else
        {
            int loop = 0;
            do
            {
                offset = loop * MAXIMUM_WAIT_OBJECTS;
                count = std::min((int)handlesCopy.size() - offset, MAXIMUM_WAIT_OBJECTS);
#    ifndef CXXKIT_OS_WINRT
                ret = wait_for_multiple_objects(count, handlesCopy.data() + offset, false, 100);
#    else
                ret = wait_for_multiple_objects_ex(count, handlesCopy.data() + offset, false, 100, false);
#    endif
                loop = (loop + 1) % loops;
            } while (ret == WAIT_TIMEOUT);
        }

        if (ret == WAIT_FAILED || ret >= WAIT_OBJECT_0 + uint_t(count))
        {
            CXXKIT_WARNING("PlatformThread internal error while waiting for adopted threads: %d",
                           int(get_last_error()));
            continue;
        }

        const int handleIndex = offset + ret - WAIT_OBJECT_0;
        if (handleIndex == 0) // New handle to watch was added.
        {
            continue;
        }
        const int platformThreadIndex = handleIndex - 1;

        lock.lock();
        PlatformThreadData *data = PlatformThreadData::current(adoptedPlatformThreads.at(platformThreadIndex));
        lock.unlock();
        if (data->is_adopted)
        {
            PlatformThread *thread = data->thread.load();
            CXXKIT_ASSERT(thread);
            auto threadPrivate = PlatformThreadPrivate::get(thread);
            CXXKIT_ASSERT(!threadPrivate->mFinished);
            CXXKIT_UNUSED(threadPrivate)
            finish(threadPrivate);
        }
        data->deref();

        lock.lock();
        close_handle(adoptedThreadHandles.at(handleIndex));
        adoptedThreadHandles.erase(adoptedThreadHandles.begin() + handleIndex);
        adoptedPlatformThreads.erase(adoptedPlatformThreads.begin() + platformThreadIndex);
    }

    PlatformThreadData *threadData = get_thread_data();
    if (threadData)
    {
        threadData->deref();
    }

    return 0;
}
/**
 * Adds an adopted thread to the list of threads that Qt watches to make sure the thread data is properly cleaned up.
 * This function starts the watcher thread if necessary.
*/
static void watch_adopted(const HANDLE adoptedThreadHandle, PlatformThread *platformThread)
{
    std::lock_guard<std::mutex> lock(adoptedThreadWatcherMutex);
    if (get_current_thread_id() == adoptedThreadWatcherId)
    {
        close_handle(adoptedThreadHandle);
        return;
    }

    adoptedThreadHandles.push_back(adoptedThreadHandle);
    adoptedPlatformThreads.push_back(platformThread);

    // Start watcher thread if it is not already running.
    if (adoptedThreadWatcherId == 0)
    {
        if (adoptedThreadWakeup == 0)
        {
#    ifndef CXXKIT_OS_WINRT
            adoptedThreadWakeup = create_event(0, false, false, 0);
#    else
            adoptedThreadWakeup = create_event_ex(0, NULL, 0, EVENT_ALL_ACCESS);
#    endif
            adoptedThreadHandles.insert(adoptedThreadHandles.begin(), adoptedThreadWakeup);
        }

        close_handle(create_thread(0, 0, adopted_thread_watcher_function, 0, 0, &adoptedThreadWatcherId));
    }
    else
    {
        set_event(adoptedThreadWakeup);
    }
}
} // namespace thread
} // namespace detail

PlatformThreadData *PlatformThreadData::current(bool createIfNecessary)
{
    detail::tls::create_tls();
    auto *threadData = detail::get_thread_data();
    if (!threadData && createIfNecessary)
    {
        threadData = new PlatformThreadData;
        detail::set_thread_data(threadData);
        CXXKIT_TRY
        {
            threadData->thread = new AdoptedPlatformThread(threadData);
        }
        CXXKIT_CATCH(...)
        {
            detail::clear_thread_data();
            threadData->deref();
            threadData = nullptr;
            CXXKIT_RETHROW;
        }
        threadData->is_adopted = true;
        threadData->thread_id.store(PlatformThread::current_thread_id());
        // if (!CoreApplicationPrivate::theMainThread.load_acquire())
        // CoreApplicationPrivate::theMainThread.storeRelease(data->thread.loadRelaxed());

        HANDLE realHandle = INVALID_HANDLE_VALUE;
        duplicate_handle(get_current_process(),
                         get_current_thread(),
                         get_current_process(),
                         &realHandle,
                         0,
                         FALSE,
                         DUPLICATE_SAME_ACCESS);
        detail::thread::watch_adopted(realHandle, threadData->thread.load());
    }
    return threadData;
}

void PlatformThreadData::clear_current()
{
    detail::clear_thread_data();
}

void PlatformThreadPrivate::set_priority(Priority priority)
{
    int prio;
    mPriority = priority;
    switch (priority)
    {
        case Priority::kIdle:
        {
            prio = THREAD_PRIORITY_IDLE;
            break;
        }
        case Priority::kLowest:
        {
            prio = THREAD_PRIORITY_LOWEST;
            break;
        }
        case Priority::kLow:
        {
            prio = THREAD_PRIORITY_BELOW_NORMAL;
            break;
        }
        case Priority::kNormal:
        {
            prio = THREAD_PRIORITY_NORMAL;
            break;
        }
        case Priority::kHigh:
        {
            prio = THREAD_PRIORITY_ABOVE_NORMAL;
            break;
        }
        case Priority::kHighest:
        {
            prio = THREAD_PRIORITY_HIGHEST;
            break;
        }
        case Priority::kTimeCritical:
        {
            prio = THREAD_PRIORITY_TIME_CRITICAL;
            break;
        }
        case Priority::kInherit:
        default:
        {
            prio = get_thread_priority(get_current_thread());
            break;
        }
    }

    if (!set_thread_priority(mThreadHandle, prio))
    {
        CXXKIT_WARNING("PlatformThread::set_priority: Failed to set thread priority");
    }
}

bool PlatformThreadPrivate::start(Priority priority)
{
    /*
      NOTE: we create the thread in the suspended state, set the priority and then resume the thread.

      since threads are created with normal priority by default, we
      could get into a case where a thread (with priority less than
      NormalPriority) tries to create a new thread (also with priority
      less than NormalPriority), but the newly created thread preempts
      its 'parent' and runs at normal priority.
    */
    unsigned int id = 0;
#    if defined(CXXKIT_CC_MSVC) && !defined(_DLL) // && !defined(CXXKIT_OS_WINRT)
#        ifdef CXXKIT_OS_WINRT
    // If you wish to accept the memory leaks, uncomment the part above.
    // See:
    //  https://support.microsoft.com/en-us/kb/104641
    //  https://msdn.microsoft.com/en-us/library/kdzttdcb.aspx
#            error "Microsoft documentation says this combination leaks memory every time a thread is started. " \
    "Please change your build back to -MD/-MDd or, if you understand this issue and want to continue, " \
    "edit this source file."
#        endif
    // MSVC -MT or -MTd build
    mThreadHandle = _beginthreadex(NULL, mStackSize, detail::thread::start, this, CREATE_SUSPENDED, &id);
#    else
    // MSVC -MD or -MDd or MinGW build
    mThreadHandle = create_thread(nullptr,
                                  mStackSize,
                                  reinterpret_cast<LPTHREAD_START_ROUTINE>(detail::thread::start),
                                  this,
                                  CREATE_SUSPENDED,
                                  reinterpret_cast<LPDWORD>(&id));
#    endif // CXXKIT_OS_WINRT
    mData->thread_id.store(id);

    if (!mThreadHandle)
    {
        CXXKIT_WARNING("PlatformThread::start: Failed to create thread");
        mRunning = false;
        mFinished = true;
        return false;
    }

    int prio;
    mPriority = priority;
    switch (priority)
    {
        case Priority::kIdle:
        {
            prio = THREAD_PRIORITY_IDLE;
            break;
        }
        case Priority::kLowest:
        {
            prio = THREAD_PRIORITY_LOWEST;
            break;
        }
        case Priority::kLow:
        {
            prio = THREAD_PRIORITY_BELOW_NORMAL;
            break;
        }
        case Priority::kNormal:
        {
            prio = THREAD_PRIORITY_NORMAL;
            break;
        }
        case Priority::kHigh:
        {
            prio = THREAD_PRIORITY_ABOVE_NORMAL;
            break;
        }
        case Priority::kHighest:
        {
            prio = THREAD_PRIORITY_HIGHEST;
            break;
        }
        case Priority::kTimeCritical:
        {
            prio = THREAD_PRIORITY_TIME_CRITICAL;
            break;
        }
        case Priority::kInherit:
        default:
        {
            prio = get_thread_priority(get_current_thread());
            break;
        }
    }
    if (!set_thread_priority(mThreadHandle, prio))
    {
        CXXKIT_WARNING("PlatformThread::start: Failed to set thread priority");
    }

    if (resume_thread(mThreadHandle) == (DWORD)-1)
    {
        CXXKIT_WARNING("PlatformThread::start: Failed to resume new thread");
    }
    return true;
}

Status PlatformThreadPrivate::terminate()
{
    if (!mTerminationEnabled)
    {
        mTerminatePending = true;
        return "Termination Disabled";
    }

    // Calling exit_thread() in set_termination_enabled is all we can do on WinRT
#    ifndef CXXKIT_OS_WINRT
    terminate_thread(mThreadHandle, 0);
#    endif
    detail::thread::finish(this, false);
    return Status::ok;
}

void PlatformThread::set_current_thread_name(const StringView name)
{
    struct
    {
        DWORD dwType;
        LPCSTR szName;
        DWORD dwThreadID;
        DWORD dwFlags;
    } info = {0x1000, name.data(), static_cast<DWORD>(-1), 0};

    __try
    {
        raise_exception(0x406D1388, 0, sizeof(info) / sizeof(DWORD), reinterpret_cast<const ULONG_PTR *>(&info));
    }
    __except (EXCEPTION_CONTINUE_EXECUTION)
    {
    }
}

int PlatformThread::ideal_concurrency_thread_count() noexcept
{
    SYSTEM_INFO sysinfo;
#    ifndef CXXKIT_OS_WINRT
    get_system_info(&sysinfo);
#    else
    get_native_system_info(&sysinfo);
#    endif
    return sysinfo.dwNumberOfProcessors;
}

PlatformThread::Id PlatformThread::current_thread_id() noexcept
{
    return get_current_thread_id();
}

void PlatformThread::set_termination_enabled(bool enabled)
{
    auto thread = PlatformThread::current_thread();
    CXXKIT_ASSERT_X(thread != nullptr,
                    "PlatformThread::set_termination_enabled()",
                    "Current thread was not started with PlatformThread.");
    CXXKIT_UNUSED(thread)

    auto d = thread->d_func();
    ThreadMutex::UniqueLock lock(d->mMutex);
    d->mTerminationEnabled = enabled;
    if (enabled && d->mTerminatePending)
    {
        detail::thread::finish(d, false);
        lock.unlock(); // don't leave the mutex locked!
#    ifndef CXXKIT_OS_WINRT
        _endthreadex(0);
#    else
        exit_thread(0);
#    endif
    }
}

void AdoptedPlatformThread::init()
{
    this->d_func()->mData->thread_id.store(get_current_thread_id());
    this->d_func()->mThreadHandle = get_current_thread();
}

CXXKIT_END_NAMESPACE

#endif // defined(CXXKIT_OS_WIN)