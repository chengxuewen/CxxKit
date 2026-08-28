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

#if !defined(CXXKIT_OS_WIN)

#    include <pthread.h>
#    include <sched.h> // SCHED_IDLE
#    include <unistd.h>
#    include <sys/types.h>
#    include <sys/syscall.h>
#    ifdef __GLIBCXX__
#        include <cxxabi.h>
#    endif
#    if defined(CXXKIT_OS_LINUX)
#        include <sys/prctl.h> // PR_SET_NAME
#        if !defined(CXXKIT_LINUXBASE)
#            include <sys/prctl.h>
#        endif
#        if !defined(SCHED_IDLE)
#            define SCHED_IDLE 5 // from linux/sched.h
#        endif
#    elif defined(CXXKIT_OS_MAC)
#        include <sys/sysctl.h> // sysctlbyname
#        if !defined(CXXKIT_OS_IOS)
#            include <CoreServices/CoreServices.h>
#        endif // !defined(CXXKIT_OS_IOS)
#    elif defined(CXXKIT_OS_BSD4)
#        include <sys/sysctl.h>
#    elif defined(CXXKIT_OS_HPUX)
#        include <sys/pstat.h>
#    elif defined(CXXKIT_OS_VXWORKS)
#        if (_WRS_VXWORKS_MAJOR > 6) || ((_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR >= 6))
#            include <vxCpuLib.h>
#            include <cpuset.h>
#            define CXXKIT_VXWORKS_HAS_CPUSET
#        endif
#    endif

#    if defined(CXXKIT_OS_DARWIN) || !defined(CXXKIT_OS_ANDROID) && !defined(CXXKIT_OS_OPENBSD) &&                     \
                                         defined(_POSIX_THREAD_PRIORITY_SCHEDULING) &&                                 \
                                         (_POSIX_THREAD_PRIORITY_SCHEDULING - 0 >= 0)
#        define CXXKIT_HAS_THREAD_PRIORITY_SCHEDULING 1
#    else
#        define CXXKIT_HAS_THREAD_PRIORITY_SCHEDULING 0
#    endif

CXXKIT_BEGIN_NAMESPACE

namespace detail
{
namespace thread
{
static void finish(void *arg);
} // namespace thread

// static_assert(sizeof(pthread_t) <= sizeof(PlatformThread::Handle), "PlatformThread::Handle size error");
CXXKIT_STATIC_CONSTANT_NUMBER(kThreadPriorityResetFlag, 0x80000000)

/* tls */
namespace tls
{
static thread_local PlatformThreadData *currentThreadData = nullptr;
static pthread_once_t currentThreadDataOnce = PTHREAD_ONCE_INIT;
static pthread_key_t currentThreadDataKey;
static void destroy_current_thread_data(void *p)
{
    auto data = static_cast<PlatformThreadData *>(p);
    // thread_local variables are set to zero before calling this destructor function,
    // if they are internally using pthread-specific data management,
    // so we need to set it back to the right value...
    currentThreadData = data;
    if (data->is_adopted)
    {
        auto thread = data->thread.load();
        CXXKIT_ASSERT(thread);
        auto threadPrivate = static_cast<PlatformThreadPrivate *>(PlatformThreadPrivate::get(thread));
        CXXKIT_ASSERT(!threadPrivate->mFinished);
        thread::finish(threadPrivate);
    }
    data->deref();

    // ... but we must reset it to zero before returning so we aren't leaving a dangling pointer.
    currentThreadData = nullptr;
}
static void create_current_thread_data_key()
{
    // create key
    pthread_key_create(&currentThreadDataKey, destroy_current_thread_data);
}
static void destroy_current_thread_data_key()
{
    pthread_once(&currentThreadDataOnce, create_current_thread_data_key);
    pthread_key_delete(currentThreadDataKey);

    // reset currentThreadDataOnce in case we end up recreating the thread-data in the rare case of
    // PlatformThreadData construction after destroying the PlatformThreadData.
    pthread_once_t pthreadOnceInit = PTHREAD_ONCE_INIT;
    currentThreadDataOnce = pthreadOnceInit;
}
CXXKIT_DESTRUCTOR_FUNCTION(destroy_current_thread_data_key)
} // namespace tls
// Utility functions for getting, setting and clearing thread specific data.
static PlatformThreadData *get_thread_data()
{
    // CXXKIT_DEBUG("get_thread_data():%p", tls::currentThreadData);
    return tls::currentThreadData;
}
static void set_thread_data(PlatformThreadData *data)
{
    tls::currentThreadData = data;
    pthread_once(&tls::currentThreadDataOnce, tls::create_current_thread_data_key);
    pthread_setspecific(tls::currentThreadDataKey, data);
}
static void clear_thread_data()
{
    tls::currentThreadData = nullptr;
    pthread_setspecific(tls::currentThreadDataKey, nullptr);
}

/* thread */
namespace thread
{
#    if CXXKIT_HAS_THREAD_PRIORITY_SCHEDULING
#        if defined(CXXKIT_OS_QNX)
static bool calculate_priority(PlatformThread::Priority priority, int *sched_policy, int *sched_priority)
{
    // On QNX, NormalPriority is mapped to 10.  A QNX system could use a value different
    // than 10 for the "normal" priority but it's difficult to achieve this so we'll
    // assume that no one has ever created such a system.  This makes the mapping from
    // Qt priorities to QNX priorities lopsided.   There's usually more space available
    // to map into above the "normal" priority than below it.  QNX also has a privileged
    // priority range (for threads that assist the kernel).  We'll assume that no Qt
    // thread needs to use priorities in that range.
    int priority_norm = 10;
    // _sched_info::priority_priv isn't documented.  You'd think that it's the start of the
    // privileged priority range but it's actually the end of the unpriviledged range.
    struct _sched_info info;
    if (SchedInfo_r(0, *sched_policy, &info) != EOK)
    {
        return false;
    }

    if (priority == PlatformThread::Priority::kIdle)
    {
        *sched_priority = info.priority_min;
        return true;
    }

    if (priority_norm < info.priority_min)
    {
        priority_norm = info.priority_min;
    }
    if (priority_norm > info.priority_priv)
    {
        priority_norm = info.priority_priv;
    }

    int to_min, to_max;
    int from_min, from_max;
    int prio;
    if (priority < PlatformThread::Priority::kNormal)
    {
        to_min = info.priority_min;
        to_max = priority_norm;
        from_min = QThread::LowestPriority;
        from_max = PlatformThread::Priority::kNormal;
    }
    else
    {
        to_min = priority_norm;
        to_max = info.priority_priv;
        from_min = PlatformThread::Priority::kNormal;
        from_max = QThread::TimeCriticalPriority;
    }

    prio = ((priority - from_min) * (to_max - to_min)) / (from_max - from_min) + to_min;
    prio = q_bound(to_min, prio, to_max);

    *sched_priority = prio;
    return true;
}
#        else
// Does some magic and calculate the Unix scheduler priorities sched_policy is IN/OUT:
// it must be set to a valid policy before calling this function sched_priority is OUT only
static bool calculate_priority(PlatformThread::Priority priority, int *sched_policy, int *sched_priority)
{
#            ifdef SCHED_IDLE
    if (priority == PlatformThread::Priority::kIdle)
    {
        *sched_policy = SCHED_IDLE;
        *sched_priority = 0;
        return true;
    }
    const auto lowestPriority = PlatformThread::Priority::kLowest;
#            else
    const auto lowestPriority = PlatformThread::Priority::kIdle;
#            endif // SCHED_IDLE
    const auto highestPriority = PlatformThread::Priority::kTimeCritical;

    int prio_min;
    int prio_max;
#            if defined(CXXKIT_OS_VXWORKS) && defined(VXWORKS_DKM)
    // for other scheduling policies than SCHED_RR or SCHED_FIFO
    prio_min = SCHED_FIFO_LOW_PRI;
    prio_max = SCHED_FIFO_HIGH_PRI;

    if ((*sched_policy == SCHED_RR) || (*sched_policy == SCHED_FIFO))
#            endif // defined(CXXKIT_OS_VXWORKS) && defined(VXWORKS_DKM)
    {
        prio_min = sched_get_priority_min(*sched_policy);
        prio_max = sched_get_priority_max(*sched_policy);
    }

    if (prio_min == -1 || prio_max == -1)
    {
        return false;
    }

    int prio;
    // crudely scale our priority enum values to the prio_min/prio_max
    prio = (((int)priority - (int)lowestPriority) * (prio_max - prio_min) / (int)highestPriority) + prio_min;
    prio = std::max(prio_min, std::min(prio_max, prio));

    *sched_priority = prio;
    return true;
}
#        endif     // defined(CXXKIT_OS_QNX)
#    endif         // CXXKIT_HAS_THREAD_PRIORITY_SCHEDULING
#    if (defined(CXXKIT_OS_LINUX) || defined(CXXKIT_OS_MAC) || defined(CXXKIT_OS_QNX))
static void set_current_name(const char *name)
{
#        if defined(CXXKIT_OS_LINUX) && !defined(CXXKIT_LINUXBASE)
    prctl(PR_SET_NAME, (unsigned long)name, 0, 0, 0);
#        elif defined(CXXKIT_OS_MAC)
    pthread_setname_np(name);
#        elif defined(CXXKIT_OS_QNX)
    pthread_setname_np(pthread_self(), name);
#        endif
}
#    endif // (defined(CXXKIT_OS_LINUX) || defined(CXXKIT_OS_MAC) || defined(CXXKIT_OS_QNX))
static void finish(void *arg)
{
    CXXKIT_TRY
    {
        auto threadPrivate = reinterpret_cast<PlatformThreadPrivate *>(arg);
        auto thread = PlatformThreadPrivate::get(threadPrivate);
        auto threadData = threadPrivate->mData;
        PlatformThreadPrivate::ThreadMutex::UniqueLock lock(threadPrivate->mMutex);
        threadPrivate->mInFinish = true;
        threadPrivate->mPriority = PlatformThread::Priority::kInherit;
        void *data = &threadPrivate->mData->tls;
        lock.unlock();
        threadPrivate->on_finished();
        // Application::sendPostedEvents(nullptr, Event::DeferredDelete);
        // ThreadStorageData::finish((void **)data);
        lock.lock();

        // AbstractEventDispatcher *eventDispatcher = d->data->eventDispatcher.loadRelaxed();
        // if (eventDispatcher)
        // {
        //     d->data->eventDispatcher = nullptr;
        //     lock.unlock();
        //     eventDispatcher->closingDown();
        //     delete eventDispatcher;
        //     lock.relock();
        // }

        threadPrivate->mRunning = false;
        threadPrivate->mFinished = true;
        threadPrivate->mInterruptionRequested = false;

        threadData->thread_id.store(0);
        threadPrivate->mThreadHandle = 0;

        threadPrivate->mInFinish = false;
        threadPrivate->mDoneCondition.notify_all();
    }
#    if CXXKIT_HAS_EXCEPTIONS
#        ifdef __GLIBCXX__
    // POSIX thread cancellation under glibc is implemented by throwing an exception
    // of this type. Do what libstdc++ is doing and handle it specially in order not to
    // abort the application if user's code calls a cancellation function.
    CXXKIT_CATCH(const abi::__forced_unwind &)
    {
        throw;
    }
#        endif // __GLIBCXX__
    CXXKIT_CATCH(...)
    {
        CXXKIT_ASSERT(false);
    }
#    endif // CXXKIT_HAS_EXCEPTIONS
}
static void *start(void *arg)
{
#    if !defined(CXXKIT_OS_ANDROID)
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
#    endif // !defined(CXXKIT_OS_ANDROID)
    pthread_cleanup_push(thread::finish, arg);

    CXXKIT_TRY
    {
        auto threadPrivate = reinterpret_cast<PlatformThreadPrivate *>(arg);
        auto threadData = PlatformThreadData::current(threadPrivate);
        //        auto thread = PlatformThreadPrivate::get(threadPrivate);
        {
            PlatformThreadPrivate::ThreadMutex::UniqueLock lock(threadPrivate->mMutex);

            // do we need to reset the thread priority?
            if (int(threadPrivate->mPriority) & kThreadPriorityResetFlag)
            {
                threadPrivate->set_priority(
                    PlatformThread::Priority((int)threadPrivate->mPriority & ~kThreadPriorityResetFlag));
            }
            threadData->thread_id.store(PlatformThread::current_thread_id());
            threadPrivate->mThreadHandle = pthread_self();
            set_thread_data(threadData);

            threadData->ref();
        }

        // data->ensureEventDispatcher();

#    if (defined(CXXKIT_OS_LINUX) || defined(CXXKIT_OS_MAC) || defined(CXXKIT_OS_QNX))
        {
            // Sets the name of the current thread. We can only do this when the thread is starting, as we don't
            // have a cross platform way of setting the name of an arbitrary thread.
            if (threadPrivate->mName.empty())
            {
                //set_current_name(threadPrivate->mName.c_str());
                PlatformThread::set_current_thread_name(threadPrivate->mName.c_str());
            }
        }
#    endif
        threadPrivate->on_started();
#    if !defined(CXXKIT_OS_ANDROID)
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
        pthread_testcancel();
#    endif
        threadPrivate->run();
    }
#    if CXXKIT_HAS_EXCEPTIONS
#        ifdef __GLIBCXX__
    // POSIX thread cancellation under glibc is implemented by throwing an exception
    // of this type. Do what libstdc++ is doing and handle it specially in order not to
    // abort the application if user's code calls a cancellation function.
    CXXKIT_CATCH(const abi::__forced_unwind &)
    {
        throw;
    }
#        endif // __GLIBCXX__
    CXXKIT_CATCH(...)
    {
        CXXKIT_ASSERT(false);
    }
#    endif // CXXKIT_HAS_EXCEPTIONS

    // This pop runs finish() below. It's outside the try/catch (and has its own try/catch) to prevent finish()
    // to be run in case an exception is thrown.
    pthread_cleanup_pop(1);
    return nullptr;
}
} // namespace thread
} // namespace detail

PlatformThreadData *PlatformThreadData::current(bool createIfNecessary)
{
    auto *threadData = detail::get_thread_data();
    if (!threadData && createIfNecessary)
    {
        threadData = new PlatformThreadData;
        CXXKIT_TRY
        {
            detail::set_thread_data(threadData);
            threadData->thread = new AdoptedPlatformThread(threadData);
            auto threadPrivate = PlatformThreadPrivate::get(threadData->thread);
            threadPrivate->mThreadHandle = pthread_self();
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
    }
    return threadData;
}

void PlatformThreadData::clear_current()
{
    detail::clear_thread_data();
}

void PlatformThreadPrivate::set_priority(Priority priority)
{
    mPriority = priority;
    // copied from start() with a few modifications:

#    if CXXKIT_HAS_THREAD_PRIORITY_SCHEDULING
    int sched_policy;
    sched_param param;

    if (pthread_getschedparam(mThreadHandle, &sched_policy, &param) != 0)
    {
        // failed to get the scheduling policy, don't bother setting the priority
        CXXKIT_WARNING("PlatformThread::set_priority: Cannot get scheduler parameters");
        return;
    }

    int prio;
    if (!detail::thread::calculate_priority(priority, &sched_policy, &prio))
    {
        // failed to get the scheduling parameters, don't bother setting the priority
        CXXKIT_WARNING("PlatformThread::set_priority: Cannot determine scheduler priority range");
        return;
    }

    param.sched_priority = prio;
    int status = pthread_setschedparam(mThreadHandle, sched_policy, &param);

#        ifdef SCHED_IDLE
    // were we trying to set to idle priority and failed?
    if (status == -1 && sched_policy == SCHED_IDLE && errno == EINVAL)
    {
        // reset to lowest priority possible
        pthread_getschedparam(mThreadHandle, &sched_policy, &param);
        param.sched_priority = sched_get_priority_min(sched_policy);
        pthread_setschedparam(mThreadHandle, sched_policy, &param);
    }
#        else
    CXXKIT_UNUSED(status);
#        endif // SCHED_IDLE
#    endif
}

bool PlatformThreadPrivate::start(Priority priority)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

#    if CXXKIT_HAS_THREAD_PRIORITY_SCHEDULING
    switch (priority)
    {
        case Priority::kInherit:
        {
            pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED);
            break;
        }
        default:
        {
            int sched_policy;
            if (pthread_attr_getschedpolicy(&attr, &sched_policy) != 0)
            {
                // failed to get the scheduling policy, don't bother setting the priority
                CXXKIT_WARNING("PlatformThread::start: Cannot determine default scheduler policy");
                break;
            }
            int prio;
            if (!detail::thread::calculate_priority(priority, &sched_policy, &prio))
            {
                // failed to get the scheduling parameters, don't bother setting the priority
                CXXKIT_WARNING("PlatformThread::start: Cannot determine scheduler priority range");
                break;
            }

            sched_param sp;
            sp.sched_priority = prio;
            if (pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) != 0 ||
                pthread_attr_setschedpolicy(&attr, sched_policy) != 0 || pthread_attr_setschedparam(&attr, &sp) != 0)
            {
                // could not set scheduling hints, fallback to inheriting them we'll try again from inside the thread
                pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED);
                mPriority = Priority((int)priority | detail::kThreadPriorityResetFlag);
            }
            break;
        }
    }
#    endif // CXXKIT_HAS_THREAD_PRIORITY_SCHEDULING
    if (mStackSize > 0)
    {
#    if defined(_POSIX_THREAD_ATTR_STACKSIZE) && (_POSIX_THREAD_ATTR_STACKSIZE - 0 > 0)
        int code = pthread_attr_setstacksize(&attr, mStackSize);
#    else
        int code = ENOSYS; // stack size not supported, automatically fail
#    endif // _POSIX_THREAD_ATTR_STACKSIZE
        if (code)
        {
            CXXKIT_WARNING("PlatformThread::start: Thread stack size error");
            // we failed to set the stacksize, and as the documentation states, the thread will fail to run...
            mRunning = false;
            mFinished = false;
            return false;
        }
    }
    mPriority = priority;

    int code = pthread_create(&mThreadHandle, &attr, detail::thread::start, this);
    if (code == EPERM)
    {
        // caller does not have permission to set the scheduling parameters/policy
#    if CXXKIT_HAS_THREAD_PRIORITY_SCHEDULING
        pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED);
#    endif // CXXKIT_HAS_THREAD_PRIORITY_SCHEDULING
        code = pthread_create(&mThreadHandle, &attr, detail::thread::start, this);
    }
    pthread_attr_destroy(&attr);
    return 0 == code;
}

Status PlatformThreadPrivate::terminate()
{
    if (!mThreadHandle)
    {
        return "threadHandle empty";
    }

    const int code = pthread_cancel(mThreadHandle);
    if (code)
    {
        return "pthread_cancel error";
    }
    return Status::ok;
}

void PlatformThread::set_current_thread_name(const StringView name)
{
#    if defined(CXXKIT_OS_LINUX) || defined(CXXKIT_LINUXBASE)
    prctl(PR_SET_NAME, reinterpret_cast<unsigned long>(name.data()), 0, 0, 0);
#    elif defined(CXXKIT_OS_MAC)
    pthread_setname_np(name.data());
#    elif defined(CXXKIT_OS_QNX)
    pthread_setname_np(pthread_self(), name.data());
#    endif
}

int PlatformThread::ideal_concurrency_thread_count() noexcept
{
    int cores = 1;
#    if defined(PTK_OS_MAC)
    // Mac OS X
#        if defined(__MAC_10_7)
    size_t size = sizeof(cores);
    sysctlbyname("hw.activecpu", &cores, &size, NULL, 0);
#        else
    cores = MPProcessorsScheduled();
#        endif
#    elif defined(PTK_OS_HPUX)
    // HP-UX
    struct pst_dynamic psd;
    if (pstat_getdynamic(&psd, sizeof(psd), 1, 0) == -1)
    {
        perror("pstat_getdynamic");
        cores = -1;
    }
    else
    {
        cores = (int)psd.psd_proc_cnt;
    }
#    elif defined(PTK_OS_BSD4)
    // FreeBSD, OpenBSD, NetBSD, BSD/OS
    ptk_size_t len = sizeof(cores);
    ptk_int_t mib[2];
    mib[0] = CTL_HW;
    mib[1] = HW_NCPU;
    if (sysctl(mib, 2, &cores, &len, NULL, 0) != 0)
    {
        perror("sysctl");
        cores = -1;
    }
#    elif defined(PTK_OS_IRIX)
    // IRIX
    cores = (int)sysconf(_SC_NPROC_ONLN);
#    elif defined(PTK_OS_INTEGRITY)
#        if (__INTEGRITY_MAJOR_VERSION >= 10)
    // Integrity V10+ does support multicore CPUs
    Value processorCount;
    if (get_processor_count(current_task(), &processorCount) == 0)
    {
        cores = processorCount;
    }
    else
#        endif
    {
        cores = 1;
    }
#    elif defined(PTK_OS_VXWORKS)
    // VxWorks
#        if defined(QT_VXWORKS_HAS_CPUSET)
    cpuset_t cpus = vx_cpu_enabled_get();
    cores = 0;

    // 128 cores should be enough for everyone ;)
    for (int i = 0; i < 128 && !CPUSET_ISZERO(cpus); ++i)
    {
        if (CPUSET_ISSET(cpus, i))
        {
            CPUSET_CLR(cpus, i);
            cores++;
        }
    }
#        else
    {
        // as of aug 2008 VxWorks < 6.6 only supports one single core CPU
        cores = 1;
    }
#        endif
#    elif defined(PTK_OS_WASM)
    cores = 1;
#    else
    // the rest: Linux, Solaris, AIX, Tru64
    cores = (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (cores <= 0)
    {
        cores = 1;
    }
#    endif
    return cores;
}

PlatformThread::Id PlatformThread::current_thread_id() noexcept
{
#    if defined(CXXKIT_OS_MAC) || defined(CXXKIT_OS_IOS)
    uint64_t tid{0};
    pthread_threadid_np(pthread_self(), &tid);
    return static_cast<Id>(tid);
#    elif defined(CXXKIT_OS_ANDROID)
    return static_cast<Id>(gettid());
#    elif defined(CXXKIT_OS_LINUX)
    return static_cast<Id>(::syscall(__NR_gettid));
#    else
    // Default implementation for nacl and solaris.
    return reinterpret_cast<Id>(pthread_self());
#    endif
}

void PlatformThread::set_termination_enabled(bool enabled)
{
    auto thread = PlatformThread::current_thread();
    CXXKIT_ASSERT_X(thread != nullptr,
                    "PlatformThread::set_termination_enabled()",
                    "Current thread was not started with PlatformThread.");
    CXXKIT_UNUSED(thread)

#    if defined(CXXKIT_OS_ANDROID)
    CXXKIT_UNUSED(enabled);
#    else
    pthread_setcancelstate(enabled ? PTHREAD_CANCEL_ENABLE : PTHREAD_CANCEL_DISABLE, nullptr);
    if (enabled)
    {
        pthread_testcancel();
    }
#    endif
}

void AdoptedPlatformThread::init()
{
}

CXXKIT_END_NAMESPACE

#endif // !defined(CXXKIT_OS_WIN)
