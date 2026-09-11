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

namespace
{
// Factory shaped exactly like cxxkit::make_uv_dispatcher: returns a fresh
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
} // namespace

TEST(EventLoopThread, start_stop_roundtrip_runs_posted_task)
{
    // Capture-by-value dispatcher with a function-pointer-compatible adapter kept OUT of
    // the ctor call: DispatcherFactory is a plain function pointer (make_uv_dispatcher
    // convention) — test uses the named factory instead.
    EventLoopThread elt(&make_fake_dispatcher);

    std::atomic<bool> ran{false};
    elt.loop().post([&ran]() { ran.store(true); });
    EXPECT_FALSE(elt.is_running());

    elt.start();
    EXPECT_TRUE(elt.is_running());
    EXPECT_TRUE(wait_until(ran)); // the task ran on the worker thread

    elt.stop();
    EXPECT_FALSE(elt.is_running());
    EXPECT_TRUE(elt.loop().is_running() == false); // exec has fully exited
}

TEST(EventLoopThread, delete_later_cross_thread_deletes_on_worker)
{
    DeleteProbe::deleted.store(false);
    EventLoopThread elt(&make_fake_dispatcher);

    // Affinity binding BEFORE start (the documented usage): the static-migration
    // contract (D35) holds because the loop is not running yet.
    DeleteProbe *victim = new DeleteProbe;
    victim->move_to_thread(&elt.loop());

    elt.start();
    victim->delete_later(); // cross-thread: must wake the loop and delete on the worker

    const pthread_t main_thread = pthread_self();
    EXPECT_TRUE(wait_until(DeleteProbe::deleted));
    EXPECT_NE(DeleteProbe::deleting_thread.load(), main_thread); // NOT on the main thread

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
    elt.stop(); // not started: no exit/join side effects, no hang
    EXPECT_FALSE(elt.is_running());
}

TEST(EventLoopThread, destructor_stops_running_thread)
{
    std::atomic<bool> ran{false};
    {
        EventLoopThread elt(&make_fake_dispatcher);
        elt.loop().post([&ran]() { ran.store(true); });
        elt.start();
        // No stop(): the destructor must join the worker implicitly.
    } // bounded: if dtor failed to stop, this scope would hang forever
    EXPECT_TRUE(ran.load());
}

TEST(EventLoopThread, loop_has_no_parent)
{
    EventLoopThread elt(&make_fake_dispatcher);
    // Momus F4: the loop is a plain member (parent = nullptr) — not a child object.
    EXPECT_EQ(elt.loop().parent(), nullptr);
}

#endif // #if CXXKIT_FEATURE_ENABLE_THREAD && CXXKIT_FEATURE_ENABLE_KERNEL
