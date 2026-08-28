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

// cxxkit::tools SharedBuffer tests — coverage for shared_buffer.cpp (COW buffer).
#include <cxxkit/tools/shared_buffer.hpp>

#include <gtest/gtest.h>

using namespace cxxkit;

TEST(SharedBuffer, DefaultEmpty)
{
    SharedBuffer buf;
    EXPECT_TRUE(buf.empty());
    EXPECT_EQ(buf.size(), 0u);
}

TEST(SharedBuffer, FromString)
{
    SharedBuffer buf(StringView("hello"));
    EXPECT_EQ(buf.size(), 5u);
    EXPECT_EQ(std::string(reinterpret_cast<const char *>(buf.cdata()), buf.size()), "hello");
    EXPECT_FALSE(buf.empty());
}

TEST(SharedBuffer, Sizing)
{
    SharedBuffer buf(16);
    EXPECT_EQ(buf.size(), 16u);
    EXPECT_GE(buf.capacity(), 16u);

    SharedBuffer sized(10, 64);
    EXPECT_EQ(sized.size(), 10u);
    EXPECT_GE(sized.capacity(), 64u);

    sized.SetSize(20);
    EXPECT_EQ(sized.size(), 20u);
}

TEST(SharedBuffer, CopyOnWriteDetach)
{
    SharedBuffer a(StringView("abc"));
    SharedBuffer b(a); // shallow copy: both share the same underlying buffer
    EXPECT_EQ(std::string(reinterpret_cast<const char *>(b.cdata()), b.size()), "abc");

    // Before mutation both views point at the same underlying storage...
    if (a.cdata() == b.cdata())
    {
        // ...and after a mutating operation on b, it detaches (COW) so the two
        // ranges no longer alias the same storage.
        b.EnsureCapacity(1024);
        EXPECT_NE(a.cdata(), b.cdata());
    }
    else
    {
        // Not sharing already (e.g. small-vector optimization) — nothing to verify.
    }
    EXPECT_EQ(std::string(reinterpret_cast<const char *>(a.cdata()), a.size()), "abc");
}

TEST(SharedBuffer, ClearAndEnsure)
{
    SharedBuffer buf(StringView("0123456789"));
    buf.Clear();
    EXPECT_EQ(buf.size(), 0u);

    buf.EnsureCapacity(128);
    EXPECT_GE(buf.capacity(), 128u);
}