/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2026~Present ChengXueWen.
**
** License: MIT License
**
***********************************************************************************************************************/
#include <cxxkit/kernel/default_dispatcher.hpp>
#include <cxxkit/kernel/event_loop.hpp>

#include <cxxkit/network/tcp_server.hpp>
#include <cxxkit/network/tcp_socket.hpp>

#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#if GTEST_HAS_DEATH_TEST
#    include <gtest/gtest-death-test.h>
#endif

namespace
{
// Fault-injection coverage for the uv error branches (final-review I-1): bind/parse failures,
// cross-thread fatals, and the poll-channel status<0 drop path. The happy paths live in
// tst_tcp_socket.cpp / tst_tcp_server.cpp.

class TcpFaultTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mLoop = std::unique_ptr<cxxkit::EventLoop>(new cxxkit::EventLoop(cxxkit::make_default_dispatcher()));
    }

    std::unique_ptr<cxxkit::EventLoop> mLoop;
};

TEST_F(TcpFaultTest, ListenBadIpReturnsFalse)
{
    cxxkit::TcpServer server(*mLoop);
    EXPECT_FALSE(server.listen("999.999.999.999", 12345)); // uv_ip4_addr parse failure
    EXPECT_EQ(0u, server.bound_port());
}

TEST_F(TcpFaultTest, ListenBadPortReturnsFalse)
{
    cxxkit::TcpServer server(*mLoop);
    EXPECT_FALSE(server.listen("127.0.0.1", 0xFFFF + 10)); // uv_ip4_addr range failure
}

TEST_F(TcpFaultTest, ListenAfterListenFailureRetryOk)
{
    cxxkit::TcpServer server(*mLoop);
    EXPECT_FALSE(server.listen("999.999.999.999", 1));
    // Retry on a healthy address: the failed-listen teardown must leave a clean slate (I-2 guard).
    EXPECT_TRUE(server.listen("127.0.0.1", 0));
    EXPECT_NE(0u, server.bound_port());
}

TEST_F(TcpFaultTest, ListenInUseThenRetryThenDestroyNoAbort)
{
    // Real bind/listen failure (EADDRINUSE) — the path where the failed-listen teardown creates
    // a handle: retry must find a fresh backend and destruction must not abort (F1 regression).
    int probe = static_cast<int>(socket(AF_INET, SOCK_STREAM, 0));
    ASSERT_GE(probe, 0);
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    ASSERT_EQ(0, bind(probe, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)));
    socklen_t len = sizeof(addr);
    ASSERT_EQ(0, getsockname(probe, reinterpret_cast<sockaddr *>(&addr), &len));
    const uint16_t occupied = ntohs(addr.sin_port);
    ASSERT_EQ(0, listen(probe, 1));

    cxxkit::TcpServer server(*mLoop);
    EXPECT_FALSE(server.listen("127.0.0.1", occupied)); // real listen failure: handle created
    EXPECT_TRUE(server.listen("127.0.0.1", 0));         // retry succeeds on the same backend
    EXPECT_NE(0u, server.bound_port());
    // server destroyed here: pump must close the retry handle (previously aborted in ~backend)
    close(probe);
}

TEST_F(TcpFaultTest, AdoptFdBadFdFatal)
{
#if GTEST_HAS_DEATH_TEST
    // uv_tcp_open on a non-socket fd must trip the adopt fatal (error-branch coverage).
    EXPECT_DEATH(
        {
            cxxkit::TcpSocket doomed(*mLoop);
            doomed.adopt_fd(*mLoop, -1);
        },
        "");
#endif
}

TEST_F(TcpFaultTest, WriteBeforeConnectFatal)
{
#if GTEST_HAS_DEATH_TEST
    EXPECT_EXIT(
        {
            cxxkit::TcpSocket socket(*mLoop);
            const uint8_t data[] = {1};
            socket.write(data, 1, [](bool) { });
        },
        ::testing::KilledBySignal(SIGABRT),
        "");
#endif
}

TEST_F(TcpFaultTest, ReadStartBeforeConnectFatal)
{
#if GTEST_HAS_DEATH_TEST
    EXPECT_EXIT(
        {
            cxxkit::TcpSocket socket(*mLoop);
            socket.read_start([](const uint8_t *, ssize_t) { });
        },
        ::testing::KilledBySignal(SIGABRT),
        "");
#endif
}
} // namespace
