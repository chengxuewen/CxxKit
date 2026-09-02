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

#include <cxxkit/containers/flat_hash_set.hpp>

#include <gtest/gtest.h>

#include <string>

namespace
{

TEST(FlatHashSet, InsertAndFind)
{
    cxxkit::flat_hash_set<int> set;
    set.insert(1);
    set.insert(2);
    set.insert(3);
    EXPECT_EQ(set.size(), 3u);
    EXPECT_TRUE(set.contains(2));
    EXPECT_FALSE(set.contains(4));
}

TEST(FlatHashSet, InsertDuplicate)
{
    cxxkit::flat_hash_set<int> set;
    EXPECT_TRUE(set.insert(1).second);
    EXPECT_FALSE(set.insert(1).second);
    EXPECT_EQ(set.size(), 1u);
}

TEST(FlatHashSet, Erase)
{
    cxxkit::flat_hash_set<int> set;
    set.insert(1);
    set.insert(2);
    EXPECT_EQ(set.erase(1), 1u);
    EXPECT_EQ(set.size(), 1u);
    EXPECT_FALSE(set.contains(1));
}

TEST(FlatHashSet, Contains)
{
    cxxkit::flat_hash_set<int> set;
    set.insert(42);
    EXPECT_TRUE(set.contains(42));
    EXPECT_FALSE(set.contains(99));
}

TEST(FlatHashSet, Rehash)
{
    cxxkit::flat_hash_set<int> set;
    for (int i = 0; i < 1000; ++i)
    {
        set.insert(i);
    }
    EXPECT_EQ(set.size(), 1000u);
    for (int i = 0; i < 1000; ++i)
    {
        EXPECT_TRUE(set.contains(i));
    }
}

TEST(FlatHashSet, Iterator)
{
    cxxkit::flat_hash_set<int> set;
    set.insert(1);
    set.insert(2);
    set.insert(3);
    int sum = 0;
    for (int v : set)
    {
        sum += v;
    }
    EXPECT_EQ(sum, 6);
}

TEST(FlatHashSet, Empty)
{
    cxxkit::flat_hash_set<int> set;
    EXPECT_TRUE(set.empty());
    EXPECT_EQ(set.size(), 0u);
    set.insert(1);
    EXPECT_FALSE(set.empty());
}

TEST(FlatHashSet, String)
{
    cxxkit::flat_hash_set<std::string> set;
    set.insert("hello");
    set.insert("world");
    EXPECT_EQ(set.size(), 2u);
    EXPECT_TRUE(set.contains("hello"));
    EXPECT_FALSE(set.contains("foo"));
}

TEST(FlatHashSet, CopyConstruct)
{
    cxxkit::flat_hash_set<int> set;
    set.insert(1);
    set.insert(2);
    cxxkit::flat_hash_set<int> set2(set);
    EXPECT_EQ(set2.size(), 2u);
    EXPECT_TRUE(set2.contains(1));
    EXPECT_EQ(set.size(), 2u);
}

TEST(FlatHashSet, MoveConstruct)
{
    cxxkit::flat_hash_set<int> set;
    set.insert(1);
    set.insert(2);
    cxxkit::flat_hash_set<int> set2(std::move(set));
    EXPECT_EQ(set2.size(), 2u);
    EXPECT_TRUE(set2.contains(1));
}

TEST(FlatHashSet, clear)
{
    cxxkit::flat_hash_set<int> set;
    set.insert(1);
    set.insert(2);
    set.clear();
    EXPECT_TRUE(set.empty());
}

TEST(FlatHashSet, EraseNonexistent)
{
    cxxkit::flat_hash_set<int> set;
    set.insert(1);
    EXPECT_EQ(set.erase(99), 0u);
    EXPECT_EQ(set.size(), 1u);
}

TEST(FlatHashSet, Emplace)
{
    cxxkit::flat_hash_set<std::string> set;
    set.emplace("hello");
    set.emplace("world");
    EXPECT_EQ(set.size(), 2u);
    EXPECT_TRUE(set.contains("hello"));
}

TEST(FlatHashSet, IteratorAfterErase)
{
    cxxkit::flat_hash_set<int> set;
    set.insert(1);
    set.insert(2);
    set.insert(3);
    set.erase(2);
    int sum = 0;
    for (int v : set)
    {
        sum += v;
    }
    EXPECT_EQ(sum, 4); // 1 + 3
}

} // namespace
