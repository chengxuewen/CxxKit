// cxxkit::tools OnceFlag tests — coverage for once_flag.cpp (thread_local accessor + states).
#include <cxxkit/tools/once_flag.hpp>

#include <gtest/gtest.h>

#include <thread>

using namespace cxxkit;

TEST(OnceFlag, LocalAccessorSameThreadStable)
{
    // thread_local singleton: same pointer within a thread.
    EXPECT_EQ(OnceFlag::localOnceFlag(), OnceFlag::localOnceFlag());
}

TEST(OnceFlag, LocalAccessorPerThread)
{
    void *mainPtr = OnceFlag::localOnceFlag();
    void *otherPtr = nullptr;
    std::thread t([&otherPtr]() { otherPtr = OnceFlag::localOnceFlag(); });
    t.join();
    EXPECT_NE(mainPtr, otherPtr); // different thread => different thread_local instance
}

TEST(OnceFlag, StateTransitions)
{
    OnceFlag flag;
    EXPECT_TRUE(flag.isNeverCalled());
    EXPECT_FALSE(flag.isDone());
    EXPECT_FALSE(flag.isInProcess());

    EXPECT_TRUE(flag.enter());
    EXPECT_TRUE(flag.isInProcess());
    EXPECT_FALSE(flag.isDone());

    flag.leave();
    EXPECT_TRUE(flag.isDone());
    EXPECT_FALSE(flag.enter()); // cannot re-enter after done
}

TEST(OnceFlag, CallOnce)
{
    OnceFlag flag;
    int calls = 0;
    auto fn = [&calls]() { ++calls; };
    EXPECT_TRUE(flag.call(fn));
    EXPECT_EQ(calls, 1);
    EXPECT_FALSE(flag.call(fn)); // second call skipped
    EXPECT_EQ(calls, 1);
}