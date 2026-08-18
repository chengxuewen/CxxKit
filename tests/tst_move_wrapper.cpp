/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cxxkit/tools/utility.hpp>
#include <cxxkit/memory/memory.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

using namespace cxxkit;

TEST(MakeMoveWrapperTest, Empty)
{
    // checks for crashes
    auto p = utils::makeMoveWrapper(std::unique_ptr<int>());
}

TEST(MakeMoveWrapperTest, NonEmpty)
{
    auto u = utils::make_unique<int>(5);
    EXPECT_EQ(*u, 5);
    auto p = utils::makeMoveWrapper(std::move(u));
    EXPECT_TRUE(!u);
    EXPECT_EQ(**p, 5);
}

TEST(MakeMoveWrapperTest, rvalue)
{
    std::unique_ptr<int> p;
    utils::makeMoveWrapper(std::move(p));
}

TEST(MakeMoveWrapperTest, lvalue)
{
    std::unique_ptr<int> p;
    utils::makeMoveWrapper(p);
}

TEST(MakeMoveWrapperTest, lvalueCopyable)
{
    std::shared_ptr<int> p;
    utils::makeMoveWrapper(p);
}

TEST(MakeMoveWrapperTest, lambda)
{
    auto u = utils::make_unique<int>(5);
    auto moveU = utils::makeMoveWrapper(std::move(u));
    EXPECT_TRUE(!u);
    EXPECT_TRUE((*moveU).get());
    [moveU]() { EXPECT_TRUE((*moveU).get()); }();
    EXPECT_TRUE(!(*moveU).get());
}

TEST(MakeMoveWrapperTest, lambdaRef)
{
    auto u = utils::make_unique<int>(5);
    auto moveU = utils::makeMoveWrapper(std::move(u));
    EXPECT_TRUE(!u);
    EXPECT_TRUE(moveU.ref().get());
    [moveU]() { EXPECT_TRUE(moveU.ref().get()); }();
    EXPECT_TRUE(!moveU.ref().get());
}

TEST(MakeMoveWrapperTest, lambdaGet)
{
    auto u = utils::make_unique<int>(5);
    auto moveU = utils::makeMoveWrapper(std::move(u));
    EXPECT_TRUE(!u);
    EXPECT_TRUE(moveU.get()->get());
    [moveU]() { EXPECT_TRUE(moveU.get()->get()); }();
    EXPECT_TRUE(!moveU.get()->get());
}

TEST(MakeMoveWrapperTest, lambdaMove)
{
    auto u = utils::make_unique<int>(5);
    auto moveU = utils::makeMoveWrapper(std::move(u));
    EXPECT_TRUE(!u);
    EXPECT_TRUE((*moveU).get());
    [moveU]() mutable { EXPECT_TRUE(moveU.move().get()); }();
    EXPECT_TRUE(!moveU.move().get());
}