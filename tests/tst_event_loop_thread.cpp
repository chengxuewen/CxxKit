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
#include <cxxkit/thread/event_loop_thread.hpp>
#include <cxxkit/kernel/abstract_event_dispatcher.hpp>
#include <cxxkit/kernel/default_dispatcher.hpp>
#include <cxxkit/kernel/object.hpp>
#include "fake_dispatcher.hpp"

#include <atomic>
#include <memory>
#include <pthread.h>
#include <thread>
#include <gtest/gtest.h>

#if CXXKIT_FEATURE_ENABLE_KERNEL

using cxxkit::EventLoop;
using cxxkit::EventLoopThread;
using cxxkit::FakeDispatcher;
using cxxkit::Object;

namespace
{
// Factory shaped exactly like cxxkit::make_default_dispatcher: returns a fresh
// heap dispatcher as unique_ptr<AbstractEventDispatcher>.
std::unique_ptr<cxxkit::AbstractEventDispatcher> make_fake_dispatcher()
{
    return std::unique_ptr<cxxkit::AbstractEventDispatcher>(new FakeDispatcher);
}

// Victim whose destructor records the deleting thread: delete_later must run
// ON the EventLoopThread worker, not on the posting (main) thread.
class DeleteProbe : public cxxkit::Object
{
public:
    ~DeleteProbe() override
    {
        deleted.store(true);
        deleting_thread.store(pthread_self());
    }
    static std::atomic<bool> deleted;
    static std::atomic<pthread_t> deleting_thread;
};
std::atomic<bool> DeleteProbe::deleted{false};
std::atomic<pthread_t> DeleteProbe::deleting_thread{};

// Bounded spin-wait so a broken liveness contract hangs the TEST, not the suite.
bool wait_until(const std::atomic<bool> &flag)
{
    for (int i = 0; i < 2000000 && !flag.load(); ++i)
    {
        std::this_thread::yield();
    }
    return flag.load();
}

// Bounded wait until a worker publishes a thread id (pthread_t{} == "not yet set").
bool wait_for_thread_id(const std::atomic<pthread_t> &id)
{
    for (int i = 0; i < 2000000 && id.load() == pthread_t{}; ++i)
    {
        std::this_thread::yield();
    }
    return id.load() != pthread_t{};
}
} // namespace

// D43.5 contract: the loop is born ON the worker thread inside start() — every test
// starts the thread before touching loop(); loop() outside the started window is fatal.
TEST(EventLoopThread, start_stop_roundtrip_runs_posted_task)
{
    // DispatcherFactory is a plain function pointer (make_default_dispatcher convention) —
    // the test uses the named factory; it is invoked by the WORKER after start().
    EventLoopThread elt(&make_fake_dispatcher);

    elt.start(); // the worker builds the loop, then exec()s it
    EXPECT_TRUE(elt.is_running());

    std::atomic<bool> ran{false};
    elt.loop().post([&ran]() { ran.store(true); }); // loop() returned: worker published it
    EXPECT_TRUE(wait_until(ran));                   // the task ran on the worker thread

    elt.stop();
    EXPECT_FALSE(elt.is_running());
}

TEST(EventLoopThread, delete_later_cross_thread_deletes_on_worker)
{
    DeleteProbe::deleted.store(false);
    EventLoopThread elt(&make_fake_dispatcher);
    elt.start();

    // Worker-first (D43.5): move_to_loop(elt) is gone (it needs a non-running target loop).
    // Affinity instead comes from constructing the victim inside the loop: the posted task
    // runs on the worker where EventLoop::current() is the worker loop, so the zero-affinity
    // DeleteProbe binds to it at construction.
    EventLoop &loop = elt.loop();
    std::atomic<DeleteProbe *> victim{nullptr};
    loop.post([&victim]() { victim.store(new DeleteProbe); });
    for (int i = 0; i < 2000000 && victim.load() == nullptr; ++i)
    {
        std::this_thread::yield();
    }
    ASSERT_NE(victim.load(), nullptr);

    victim.load()->delete_later(); // from the main thread: must wake the loop and delete on the worker

    const pthread_t main_thread = pthread_self();
    EXPECT_TRUE(wait_until(DeleteProbe::deleted));
    EXPECT_NE(DeleteProbe::deleting_thread.load(), main_thread); // NOT on the main thread

    elt.stop();
}

TEST(EventLoopThread, in_loop_construction_binds_elt_affinity)
{
    // The implicit-conversion route (obj.move_to_loop(elt)) is retired with worker-first
    // construction: it required the target loop to be non-running, which a started ELT
    // never is. The affinity assertion now pins in-loop construction: an Object created
    // inside a posted task must report the worker loop as its own.
    EventLoopThread elt(&make_fake_dispatcher);
    elt.start();
    EventLoop &loop = elt.loop(); // blocks on the main thread until the worker published the loop
    std::atomic<Object *> created{nullptr};
    loop.post([&created]() { created.store(new Object); });
    for (int i = 0; i < 2000000 && created.load() == nullptr; ++i)
    {
        std::this_thread::yield();
    }
    ASSERT_NE(created.load(), nullptr);
    EXPECT_EQ(created.load()->loop(), &loop);
    created.load()->delete_later(); // cleanup runs on the worker
    elt.stop();
}

TEST(EventLoopThread, double_start_fails)
{
    EventLoopThread elt(&make_fake_dispatcher);
    elt.start();
    EXPECT_DEATH(elt.start(), "");
    elt.stop();
}

TEST(EventLoopThread, stop_before_start_is_noop)
{
    EventLoopThread elt(&make_fake_dispatcher);
    elt.stop(); // not started: no worker exists, nothing to exit/join — no hang
    EXPECT_FALSE(elt.is_running());
}

TEST(EventLoopThread, loop_before_start_is_fatal)
{
    // Worker-first: between the ctor and start() no loop exists — wait_for_loop cannot wait
    // (the worker that would notify is not running) and returns null, which loop() turns into
    // a fatal with the start() hint.
    EventLoopThread elt(&make_fake_dispatcher);
    EXPECT_DEATH(elt.loop(), "");
}

TEST(EventLoopThread, destructor_stops_running_thread)
{
    std::atomic<bool> ran{false};
    {
        EventLoopThread elt(&make_fake_dispatcher);
        elt.start();
        elt.loop().post([&ran]() { ran.store(true); });
        EXPECT_TRUE(wait_until(ran)); // task ran on the worker
        // No stop(): the destructor must join the worker implicitly.
    } // bounded: if dtor failed to stop, this scope would hang forever
    EXPECT_TRUE(ran.load());
}

TEST(EventLoopThread, loop_has_no_parent)
{
    EventLoopThread elt(&make_fake_dispatcher);
    elt.start();
    // Momus F4: the loop is a plain member (parent = nullptr) — not a child object.
    EXPECT_EQ(elt.loop().parent(), nullptr);
    elt.stop();
}

#    if defined(CXXKIT_ENABLE_LOOP_BACKEND_UV)
namespace
{
// Factory thread pin: the real uv dispatcher records nothing, so this wrapper captures the
// thread that invoked the factory. Worker-first requires that thread to be the exec thread.
std::atomic<pthread_t> g_factory_thread{};
std::unique_ptr<cxxkit::AbstractEventDispatcher> recording_default_dispatcher()
{
    g_factory_thread.store(pthread_self());
    return cxxkit::make_default_dispatcher();
}
} // namespace

TEST(EventLoopThread, default_dispatcher_born_on_worker_thread)
{
    // The original D43 fatal: EventLoopThread(&make_default_dispatcher) built the uv
    // dispatcher in the ctor (main thread) and exec'd it on the worker — "called from
    // non-loop thread". Worker-first construction (D43.5 T1) invokes the factory on the
    // worker, so uv's loop-thread capture matches the exec thread by construction:
    // not-fatal + task-ran IS the regression pin.
    g_factory_thread.store(pthread_t{});
    EventLoopThread elt(&recording_default_dispatcher);
    elt.start();

    std::atomic<pthread_t> exec_thread{};
    elt.loop().post([&exec_thread]() { exec_thread.store(pthread_self()); });
    EXPECT_TRUE(wait_for_thread_id(exec_thread)); // the task ran: no affinity fatal anywhere
    EXPECT_TRUE(wait_for_thread_id(g_factory_thread));
    EXPECT_EQ(g_factory_thread.load(), exec_thread.load()); // factory thread == exec thread (the fix)
    EXPECT_NE(exec_thread.load(), pthread_self());          // and it is NOT the spawning thread

    elt.stop();
}
#    endif // defined(CXXKIT_ENABLE_LOOP_BACKEND_UV)

#endif // CXXKIT_FEATURE_ENABLE_KERNEL
