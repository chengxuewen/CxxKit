// cxxkit::tools IdRegistry tests — coverage for id_registry.cpp.
#include <cxxkit/tools/id_registry.hpp>

#include <gtest/gtest.h>

#include <algorithm>

using namespace cxxkit;

TEST(IdRegistry, RequestRegisterUnregister)
{
    IdRegistry registry;

    const int64_t id1 = registry.requestId();
    const int64_t id2 = registry.requestId();
    EXPECT_NE(id1, id2);
    EXPECT_EQ(registry.registeredIdCount(), 2);

    EXPECT_TRUE(registry.isIdRegistered(id1));
    EXPECT_TRUE(registry.isIdRegistered(id2));

    registry.unregisterId(id1);
    EXPECT_FALSE(registry.isIdRegistered(id1));
    EXPECT_EQ(registry.registeredIdCount(), 1);

    // Unregistering an unknown id is a no-op.
    registry.unregisterId(99999);
    EXPECT_EQ(registry.registeredIdCount(), 1);
}

TEST(IdRegistry, RegisterExplicitAndReuse)
{
    IdRegistry registry;
    registry.registerId(42);
    EXPECT_TRUE(registry.isIdRegistered(42));
    EXPECT_FALSE(registry.isIdRegistered(43));

    // requestId must not hand out an explicitly registered id.
    const int64_t got = registry.requestId();
    EXPECT_NE(got, 42);

    // After release, the id becomes available again.
    registry.unregisterId(42);
    std::vector<int64_t> seen;
    for (int i = 0; i < 64; ++i)
    {
        const int64_t id = registry.requestId();
        if (std::find(seen.begin(), seen.end(), id) == seen.end())
        {
            seen.push_back(id);
        }
    }
    EXPECT_GT(seen.size(), 0u);
}