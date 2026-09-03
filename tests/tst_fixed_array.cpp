/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2026~Present ChengXueWen.
** Copyright 2018 The Abseil Authors.
**
** License: MIT License
**
** This file contains a simplified reimplementation of FixedArray (API-compatible subset of
** abseil-cpp `absl/container/fixed_array.h`, originally governed by the Apache License 2.0).
** Modified for CxxKit.
**
** Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
** documentation files (the "Software"), to deal in the Software without restriction, including without limitation
** the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
** and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all copies or substantial portions
** of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
** THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
** CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
**
***********************************************************************************************************************/

// Tests for FixedArray (containers/fixed_array.hpp), a simplified clean-room
// reimplementation of the abseil FixedArray API subset.

#include <cxxkit/containers/fixed_array.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <utility>

namespace
{

using cxxkit::FixedArray;

// Probe type with static construction/destruction counters.
class Counted
{
public:
    enum
    {
        kMagicValue = 0x5A5A5A5A
    };

    Counted()
        : mValue(kMagicValue)
    {
        ++sConstruct;
    }

    explicit Counted(int value)
        : mValue(value)
    {
        ++sConstruct;
    }

    Counted(const Counted &o)
        : mValue(o.mValue)
    {
        ++sConstruct;
    }

    ~Counted() { ++sDestruct; }

    int value() const { return mValue; }

    void set_value(int v) { mValue = v; }

    static void reset_counters()
    {
        sConstruct = 0;
        sDestruct = 0;
    }

    static int sConstruct;
    static int sDestruct;

private:
    int mValue;
};

int Counted::sConstruct = 0;
int Counted::sDestruct = 0;

} // namespace

namespace
{

// 1. Construction default-constructs exactly n elements; destruction destroys them all.
TEST(FixedArrayTest, ConstructDestroyCounted)
{
    Counted::reset_counters();
    {
        FixedArray<Counted> a(4);
        EXPECT_EQ(4u, a.size());
        EXPECT_EQ(4, Counted::sConstruct);
        EXPECT_EQ(0, Counted::sDestruct);
    }
    EXPECT_EQ(4, Counted::sConstruct);
    EXPECT_EQ(4, Counted::sDestruct);
}

// 2. fill(n, value) via value ctor + operator[] reads back; re-fill overwrites.
TEST(FixedArrayTest, FillAndReadBack)
{
    Counted::reset_counters();
    {
        FixedArray<Counted> a(3, Counted(7));
        ASSERT_EQ(3u, a.size());
        for (size_t i = 0; i < a.size(); ++i)
        {
            EXPECT_EQ(7, a[i].value());
        }
        a.fill(Counted(9));
        for (size_t i = 0; i < a.size(); ++i)
        {
            EXPECT_EQ(9, a[i].value());
        }
    }
    EXPECT_EQ(Counted::sConstruct, Counted::sDestruct);
}

// 3. operator[] boundary elements, const and non-const.
TEST(FixedArrayTest, SubscriptBoundary)
{
    FixedArray<int> a(5, 41);
    a[0] = 1;
    a[4] = 99;
    EXPECT_EQ(1, a[0]);
    EXPECT_EQ(41, a[2]);
    EXPECT_EQ(99, a[a.size() - 1]);

    const FixedArray<int> &ca = a;
    EXPECT_EQ(1, ca[0]);
    EXPECT_EQ(99, ca[4]);
}

// 4. initializer_list construction preserves order.
TEST(FixedArrayTest, InitializerList)
{
    FixedArray<int> a = {10, 20, 30};
    ASSERT_EQ(3u, a.size());
    EXPECT_EQ(10, a[0]);
    EXPECT_EQ(20, a[1]);
    EXPECT_EQ(30, a[2]);

    FixedArray<int> empty_list = {};
    EXPECT_TRUE(empty_list.empty());
}

// 5. Move transfers the buffer; source left empty and destructible.
TEST(FixedArrayTest, MoveSemantics)
{
    Counted::reset_counters();
    {
        FixedArray<Counted> a(3, Counted(1));
        const Counted *bufferBefore = a.data();

        FixedArray<Counted> b(std::move(a));
        EXPECT_EQ(3u, b.size());
        EXPECT_EQ(bufferBefore, b.data()); // buffer changed hands
        EXPECT_EQ(0u, a.size());
        EXPECT_TRUE(a.empty());
        EXPECT_EQ(a.begin(), a.end());

        FixedArray<Counted> c(1);
        const Counted *bufferC = c.data();
        c = std::move(b);
        EXPECT_EQ(3u, c.size());
        EXPECT_EQ(bufferBefore, c.data()); // took b's buffer
        EXPECT_EQ(0u, b.size());
        EXPECT_NE(bufferC, c.data()); // c's old buffer (1 elem) was destroyed+freed

        a = FixedArray<Counted>(0); // moved-from source is assignable/destructible
        EXPECT_TRUE(a.empty());
    }
    EXPECT_EQ(Counted::sConstruct, Counted::sDestruct);
}

// 6. const correctness: const operator[]/data/begin/cend.
TEST(FixedArrayTest, ConstCorrectness)
{
    const FixedArray<int> a(3, 5);
    static_cast<void>(a);
    EXPECT_EQ(3u, a.size());
    EXPECT_EQ(5, a[0]);
    EXPECT_EQ(5, *a.begin());
    EXPECT_EQ(5, *a.cbegin());
    EXPECT_EQ(a.data() + 3, a.cend());
    const int *p = a.data();
    EXPECT_EQ(p, a.begin());

    FixedArray<int>::const_iterator it = a.begin();
    EXPECT_EQ(5, it[1]);
}

// 7. Range-for iteration via begin/end.
TEST(FixedArrayTest, RangeForIteration)
{
    FixedArray<int> a = {1, 2, 3, 4, 5};
    int sum = 0;
    for (int v : a)
    {
        sum += v;
    }
    EXPECT_EQ(15, sum);

    const FixedArray<int> &ca = a;
    int csum = 0;
    for (const int &v : ca)
    {
        csum += v;
    }
    EXPECT_EQ(15, csum);
}

// 8. n=0 is legal: size 0, empty(), data() may be null (documented).
TEST(FixedArrayTest, EmptyArray)
{
    Counted::reset_counters();
    {
        FixedArray<Counted> a(0);
        EXPECT_EQ(0u, a.size());
        EXPECT_TRUE(a.empty());
        EXPECT_EQ(nullptr, a.data()); // documented: data() == nullptr when empty
        EXPECT_EQ(a.begin(), a.end());
        a.fill(Counted(1));          // no-op, must not crash
        FixedArray<Counted> copy(a); // copying empty is fine
        EXPECT_TRUE(copy.empty());
    }
    // Only the fill(Counted(1)) temporary: construct/destruct balanced, array elements none.
    EXPECT_EQ(1, Counted::sConstruct);
    EXPECT_EQ(1, Counted::sDestruct);
}

// 9. n=1 minimal array.
TEST(FixedArrayTest, SingleElement)
{
    FixedArray<int> a(1, 123);
    ASSERT_EQ(1u, a.size());
    EXPECT_FALSE(a.empty());
    EXPECT_EQ(123, a[0]);
    EXPECT_EQ(a.data(), a.begin());
    EXPECT_EQ(a.data() + 1, a.end());
}

// 10. Copy is deep: mutating the copy leaves the original intact.
TEST(FixedArrayTest, DeepCopy)
{
    FixedArray<int> a(3, 7);
    FixedArray<int> b(a);
    ASSERT_EQ(3u, b.size());
    EXPECT_NE(a.data(), b.data());
    b[1] = 42;
    EXPECT_EQ(7, a[1]);
    EXPECT_EQ(42, b[1]);

    FixedArray<int> c(1);
    c = a; // copy-assign shrinks/grows
    ASSERT_EQ(3u, c.size());
    c[2] = 99;
    EXPECT_EQ(7, a[2]);
    EXPECT_EQ(99, c[2]);

    FixedArray<int> d = {1, 2};
    d = a; // assign over a different size
    EXPECT_EQ(3u, d.size());
    EXPECT_EQ(7, d[0]);
}

} // namespace
