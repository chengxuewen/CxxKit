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

#include <cxxkit/tools/utility.hpp>
#include <cxxkit/memory/memory.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

using namespace cxxkit;

TEST(MakeMoveWrapperTest, Empty)
{
    // checks for crashes
    auto p = utils::make_move_wrapper(std::unique_ptr<int>());
}

TEST(MakeMoveWrapperTest, NonEmpty)
{
    auto u = utils::make_unique<int>(5);
    EXPECT_EQ(*u, 5);
    auto p = utils::make_move_wrapper(std::move(u));
    EXPECT_TRUE(!u);
    EXPECT_EQ(**p, 5);
}

TEST(MakeMoveWrapperTest, rvalue)
{
    std::unique_ptr<int> p;
    utils::make_move_wrapper(std::move(p));
}

TEST(MakeMoveWrapperTest, lvalue)
{
    std::unique_ptr<int> p;
    utils::make_move_wrapper(p);
}

TEST(MakeMoveWrapperTest, lvalueCopyable)
{
    std::shared_ptr<int> p;
    utils::make_move_wrapper(p);
}

TEST(MakeMoveWrapperTest, lambda)
{
    auto u = utils::make_unique<int>(5);
    auto moveU = utils::make_move_wrapper(std::move(u));
    EXPECT_TRUE(!u);
    EXPECT_TRUE((*moveU).get());
    [moveU]() { EXPECT_TRUE((*moveU).get()); }();
    EXPECT_TRUE(!(*moveU).get());
}

TEST(MakeMoveWrapperTest, lambdaRef)
{
    auto u = utils::make_unique<int>(5);
    auto moveU = utils::make_move_wrapper(std::move(u));
    EXPECT_TRUE(!u);
    EXPECT_TRUE(moveU.ref().get());
    [moveU]() { EXPECT_TRUE(moveU.ref().get()); }();
    EXPECT_TRUE(!moveU.ref().get());
}

TEST(MakeMoveWrapperTest, lambdaGet)
{
    auto u = utils::make_unique<int>(5);
    auto moveU = utils::make_move_wrapper(std::move(u));
    EXPECT_TRUE(!u);
    EXPECT_TRUE(moveU.get()->get());
    [moveU]() { EXPECT_TRUE(moveU.get()->get()); }();
    EXPECT_TRUE(!moveU.get()->get());
}

TEST(MakeMoveWrapperTest, lambdaMove)
{
    auto u = utils::make_unique<int>(5);
    auto moveU = utils::make_move_wrapper(std::move(u));
    EXPECT_TRUE(!u);
    EXPECT_TRUE((*moveU).get());
    [moveU]() mutable { EXPECT_TRUE(moveU.move().get()); }();
    EXPECT_TRUE(!moveU.move().get());
}