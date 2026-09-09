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
** Jhe above copyright notice and this permission notice shall be included in all copies or substantial portions
** of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
** TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
** THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
** CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
**
***********************************************************************************************************************/

// cxxkit::memory WeakPtr/WeakPtrFactory tests — non-intrusive weak references (Chromium shape).
#include <cxxkit/memory/weak_ptr.hpp>

#include <gtest/gtest.h>

using namespace cxxkit;

namespace
{

struct Thing
{
    int value = 0;
    explicit Thing(int v)
        : value(v)
    {
    }
};

} // namespace

TEST(WeakPtr, get_returns_owner_while_alive)
{
    // Arrange
    Thing thing(42);
    WeakPtrFactory<Thing> factory(&thing);

    // Act
    WeakPtr<Thing> weak = factory.get_weak_ptr();

    // Assert
    ASSERT_TRUE(weak);
    ASSERT_EQ(&thing, weak.get());
    ASSERT_EQ(42, weak.get()->value);
}

TEST(WeakPtr, get_returns_null_after_factory_destroyed)
{
    // Arrange
    Thing thing(7);
    std::unique_ptr<WeakPtrFactory<Thing>> factory(new WeakPtrFactory<Thing>(&thing));
    WeakPtr<Thing> weak = factory->get_weak_ptr();
    ASSERT_TRUE(weak);

    // Act
    factory.reset();

    // Assert
    EXPECT_FALSE(weak);
    EXPECT_EQ(nullptr, weak.get());
}

TEST(WeakPtr, multiple_weak_ptrs_share_invalidation)
{
    // Arrange
    Thing thing(1);
    WeakPtrFactory<Thing> factory(&thing);
    WeakPtr<Thing> weak_a = factory.get_weak_ptr();
    WeakPtr<Thing> weak_b = factory.get_weak_ptr();
    WeakPtr<Thing> weak_c = weak_a;
    ASSERT_TRUE(weak_a);
    ASSERT_TRUE(weak_b);
    ASSERT_TRUE(weak_c);

    // Act
    factory.~WeakPtrFactory();
    new (&factory) WeakPtrFactory<Thing>(&thing); // placement-new: restore for destruction balance

    // Assert
    EXPECT_FALSE(weak_a);
    EXPECT_FALSE(weak_b);
    EXPECT_FALSE(weak_c);
    EXPECT_EQ(nullptr, weak_a.get());
    EXPECT_EQ(nullptr, weak_b.get());
    EXPECT_EQ(nullptr, weak_c.get());
}

TEST(WeakPtr, copy_and_move_preserve_liveness)
{
    // Arrange
    Thing thing(3);
    std::unique_ptr<WeakPtrFactory<Thing>> factory(new WeakPtrFactory<Thing>(&thing));
    WeakPtr<Thing> source = factory->get_weak_ptr();

    // Act
    WeakPtr<Thing> copied = source;
    WeakPtr<Thing> moved = source;

    // Assert
    EXPECT_TRUE(copied);
    EXPECT_TRUE(moved);
    EXPECT_EQ(&thing, copied.get());
    EXPECT_EQ(&thing, moved.get());

    // Act: invalidate through the factory; all copies must see it
    factory.reset();

    // Assert
    EXPECT_FALSE(source);
    EXPECT_FALSE(copied);
    EXPECT_FALSE(moved);
    EXPECT_EQ(nullptr, source.get());
    EXPECT_EQ(nullptr, copied.get());
    EXPECT_EQ(nullptr, moved.get());
}

TEST(WeakPtr, reset_clears_and_bool_operator)
{
    // Arrange
    Thing thing(5);
    WeakPtrFactory<Thing> factory(&thing);
    WeakPtr<Thing> weak = factory.get_weak_ptr();
    ASSERT_TRUE(weak);
    WeakPtr<Thing> empty;
    EXPECT_FALSE(empty);
    EXPECT_EQ(nullptr, empty.get());

    // Act
    weak.reset();

    // Assert
    EXPECT_FALSE(weak);
    EXPECT_EQ(nullptr, weak.get());

    // Act: reset() must not affect the factory or other weak ptrs
    WeakPtr<Thing> other = factory.get_weak_ptr();

    // Assert
    EXPECT_TRUE(other);
    EXPECT_EQ(&thing, other.get());
}
