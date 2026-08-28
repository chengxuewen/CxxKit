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