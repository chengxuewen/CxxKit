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

#include <cxxkit/containers/flat_hash_map.hpp>

#include <gtest/gtest.h>

#include <string>
#include <utility>

namespace {

TEST(FlatHashMap, InsertAndFind) {
    cxxkit::flat_hash_map<std::string, int> map;
    map["hello"] = 1;
    map["world"] = 2;
    EXPECT_EQ(map.size(), 2u);
    EXPECT_EQ(map["hello"], 1);
    EXPECT_EQ(map["world"], 2);
}

TEST(FlatHashMap, InsertDuplicate) {
    cxxkit::flat_hash_map<int, int> map;
    auto res1 = map.insert({1, 10});
    EXPECT_TRUE(res1.second);
    auto res2 = map.insert({1, 20});
    EXPECT_FALSE(res2.second);
    EXPECT_EQ(res2.first->second, 10);
}

TEST(FlatHashMap, Erase) {
    cxxkit::flat_hash_map<int, int> map;
    map[1] = 10;
    map[2] = 20;
    EXPECT_EQ(map.erase(1), 1u);
    EXPECT_EQ(map.size(), 1u);
    EXPECT_EQ(map.find(1), map.end());
}

TEST(FlatHashMap, Contains) {
    cxxkit::flat_hash_map<int, int> map;
    map[1] = 10;
    EXPECT_TRUE(map.contains(1));
    EXPECT_FALSE(map.contains(2));
}

TEST(FlatHashMap, At) {
    cxxkit::flat_hash_map<int, int> map;
    map[1] = 10;
    EXPECT_EQ(map.at(1), 10);
    EXPECT_THROW(map.at(2), std::out_of_range);
}

TEST(FlatHashMap, AtConst) {
    cxxkit::flat_hash_map<int, int> map;
    map[1] = 10;
    const auto& cmap = map;
    EXPECT_EQ(cmap.at(1), 10);
    EXPECT_THROW(cmap.at(2), std::out_of_range);
}

TEST(FlatHashMap, Rehash) {
    cxxkit::flat_hash_map<int, int> map;
    for (int i = 0; i < 1000; ++i) {
        map[i] = i * 2;
    }
    EXPECT_EQ(map.size(), 1000u);
    for (int i = 0; i < 1000; ++i) {
        EXPECT_EQ(map[i], i * 2);
    }
}

TEST(FlatHashMap, Iterator) {
    cxxkit::flat_hash_map<int, int> map;
    map[1] = 10;
    map[2] = 20;
    map[3] = 30;
    int sum = 0;
    for (const auto& kv : map) {
        sum += kv.second;
    }
    EXPECT_EQ(sum, 60);
}

TEST(FlatHashMap, Empty) {
    cxxkit::flat_hash_map<int, int> map;
    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.size(), 0u);
    map[1] = 10;
    EXPECT_FALSE(map.empty());
}

TEST(FlatHashMap, MoveConstruct) {
    cxxkit::flat_hash_map<int, int> map;
    map[1] = 10;
    map[2] = 20;
    cxxkit::flat_hash_map<int, int> map2(std::move(map));
    EXPECT_EQ(map2.size(), 2u);
    EXPECT_EQ(map2[1], 10);
}

TEST(FlatHashMap, CopyConstruct) {
    cxxkit::flat_hash_map<int, int> map;
    map[1] = 10;
    map[2] = 20;
    cxxkit::flat_hash_map<int, int> map2(map);
    EXPECT_EQ(map2.size(), 2u);
    EXPECT_EQ(map2[1], 10);
    // original unchanged
    EXPECT_EQ(map.size(), 2u);
}

TEST(FlatHashMap, Clear) {
    cxxkit::flat_hash_map<int, int> map;
    map[1] = 10;
    map[2] = 20;
    map.clear();
    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.size(), 0u);
}

TEST(FlatHashMap, Reserve) {
    cxxkit::flat_hash_map<int, int> map;
    map.reserve(100);
    EXPECT_GE(map.capacity(), 100u);
}

TEST(FlatHashMap, Emplace) {
    cxxkit::flat_hash_map<int, std::string> map;
    map.emplace(1, "hello");
    map.emplace(2, "world");
    EXPECT_EQ(map[1], "hello");
    EXPECT_EQ(map[2], "world");
}

TEST(FlatHashMap, EraseNonexistent) {
    cxxkit::flat_hash_map<int, int> map;
    map[1] = 10;
    EXPECT_EQ(map.erase(99), 0u);
    EXPECT_EQ(map.size(), 1u);
}

TEST(FlatHashMap, InsertPair) {
    cxxkit::flat_hash_map<int, int> map;
    map.insert(std::make_pair(1, 10));
    map.insert(std::make_pair(2, 20));
    EXPECT_EQ(map.size(), 2u);
    EXPECT_EQ(map[1], 10);
}

}  // namespace
