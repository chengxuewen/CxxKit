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

#include "cxxkit/tools/status.hpp"
#include "cxxkit/thread/mutex.hpp"
#include "cxxkit/base/core_config.hpp"

#include <thread>
#include <future>     // for std::async
#include <functional> // for std::invoke; no guard needed as it's a C++98 header

/**
 * @addtogroup core
 * @{
 * @addtogroup PlatformThread
 * @brief
 * @{
 * @details
 */

#if defined(__cpp_lib_invoke) && __cpp_lib_invoke >= 201411 && defined(__cpp_init_captures) &&                         \
    __cpp_init_captures >= 201304 && defined(__cpp_generic_lambdas) && __cpp_generic_lambdas >= 201304
#    define CXXKIT_PLATFORM_THREAD_HAS_VARIADIC_CREATE 1
#else
#    define CXXKIT_PLATFORM_THREAD_HAS_VARIADIC_CREATE 0
#endif

#if defined(__cpp_init_captures) && __cpp_init_captures >= 201304
#    define CXXKIT_PLATFORM_THREAD_HAS_INIT_CAPTURES 1
#else
#    define CXXKIT_PLATFORM_THREAD_HAS_INIT_CAPTURES 0
#endif

#if defined(CXXKIT_OS_WIN)
#    define CXXKIT_PLATFORM_THREAD_HANDLE_T void *;
#else
#    define CXXKIT_PLATFORM_THREAD_HANDLE_T pthread_t;
#endif // defined(CXXKIT_OS_WIN)

CXXKIT_BEGIN_NAMESPACE

namespace detail
{
template <typename Function>
struct Callable
{
    explicit Callable(Function &&f)
        : mFunction(std::forward<Function>(f))
    {
    }

    // Apply the same semantics of a lambda closure type w.r.t. the special
    // member functions, if possible: delete the copy assignment operator,
    // bring back all the others as per the RO5 (cf. §8.1.5.1/11 [expr.prim.lambda.closure])
    ~Callable() = default;
    Callable(const Callable &) = default;
    Callable(Callable &&) = default;
    Callable &operator=(const Callable &) = delete;
    Callable &operator=(Callable &&) = default;

    void operator()() { (void)mFunction(); }

    typename std::decay<Function>::type mFunction;
};
} // namespace detail

class PlatformThreadPrivate;
class CXXKIT_CORE_API PlatformThread
{
    using ThreadMutex = RecursiveMutex;

public:
    using SharedPtr = std::shared_ptr<PlatformThread>;
    using UniquePtr = std::unique_ptr<PlatformThread>;

    using Id = size_t;

    enum class Priority : int
    {
        kIdle,
        kLowest,
        kLow,
        kNormal,
        kHigh,
        kHighest,
        kTimeCritical,
        kInherit,
    };

    PlatformThread();
    PlatformThread(PlatformThreadPrivate *d);
    virtual ~PlatformThread();

    bool isInterruptionRequested() const;
    Status requestInterruption();

    std::string name() const;
    Status setName(const StringView name, const void *obj = nullptr);

    /**
     * If the thread is not running, this function returns \c InheritPriority.
     * \sa Priority, setPriority(), start()
     * @return Returns the priority for a running thread.
     */
    Priority priority() const;
    /**
     * This function sets the \a priority for a running thread.
     * If the thread is not running, this function does nothing and returns immediately.
     * Use start() to start a thread with a specific priority.
     *
     * The \a priority argument can be any value in the \c Priority enum except for \c InheritPriority.
     * The effect of the \a priority parameter is dependent on the operating system's scheduling policy.
     * In particular, the \a priority will be ignored on systems that do not support thread priorities
     * (such as on Linux, see http://linux.die.net/man/2/sched_setscheduler for more details).
     *
     * \sa Priority, priority(), start()
     * @param priority
     * @return
     */
    Status setPriority(Priority priority);

    /**
     * @return Returns the maximum stack size for the thread (if set with setStackSize()); otherwise returns zero.
     */
    uint_t stackSize() const;
    /**
     * Sets the maximum stack size for the thread to \a stackSize.
     * If \a stackSize is greater than zero, the maximum stack size is set to \a stackSize bytes,
     * otherwise the maximum stack size is automatically determined by the operating system.
     *
     * \warning Most operating systems place minimum and maximum limits on thread stack sizes.
     * The thread will fail to start if the stack size is outside these limits.
     *
     * \sa stackSize()
     * @param stackSize
     * @return
     */
    Status setStackSize(uint_t stackSize);

    bool isFinished() const;
    bool isRunning() const;
    bool isAdopted() const;

    Id threadId() const;
    int retval() const;

    Status start(Priority = Priority::kInherit);
    Status terminate();

    CXXKIT_STATIC_CONSTANT_NUMBER(kWaitForeverMSecs, std::numeric_limits<unsigned int>::max())
    bool wait(unsigned int msecs = kWaitForeverMSecs);

    static void setCurrentThreadName(const StringView name);
    static int idealConcurrencyThreadCount() noexcept;

    static PlatformThread *currentThread() noexcept;
    static Id currentThreadId() noexcept;

    static void usleep(unsigned long usecs);
    static void msleep(unsigned long msecs);
    static void sleep(unsigned long secs);
    static void yield();

    static UniquePtr create(std::future<void> &&future);
#if CXXKIT_PLATFORM_THREAD_HAS_VARIADIC_CREATE
    template <typename Func, typename... Args>
    static UniquePtr create(Func &&f, Args &&...args)
    {
        using DecayedFunction = typename std::decay<Func>::type;
        auto threadFunction = [f = static_cast<DecayedFunction>(std::forward<Func>(f))](auto &&...largs) mutable -> void
        { (void)std::invoke(std::move(f), std::forward<decltype(largs)>(largs)...); };

        return create(std::async(std::launch::deferred, std::move(threadFunction), std::forward<Args>(args)...));
    }
#elif CXXKIT_PLATFORM_THREAD_HAS_INIT_CAPTURES
    template <typename Func>
    static UniquePtr create(Func &&func)
    {
        using DecayedFunc = typename std::decay<Func>::type;
        auto threadFunc = [func = static_cast<DecayedFunc>(std::forward<Func>(func))]() mutable -> void
        { (void)func(); };

        return create(std::async(std::launch::deferred, std::move(threadFunc)));
    }
#else
    template <typename Func>
    static UniquePtr create(Func &&func)
    {
        return create(std::async(std::launch::deferred, detail::Callable<Func>(std::forward<Func>(func))));
    }
#endif

protected:
    static void setTerminationEnabled(bool enabled = true);

    virtual void onFinished() { }
    virtual void onStarted() { }
    virtual void run() { }

protected:
    CXXKIT_DEFINE_DPTR(PlatformThread)
    CXXKIT_DECLARE_PRIVATE(PlatformThread)
    CXXKIT_DISABLE_COPY_MOVE(PlatformThread)
};

CXXKIT_END_NAMESPACE

/**
 * @}
 * @}
 */
