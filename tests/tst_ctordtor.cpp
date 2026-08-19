/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present ChengXueWen.
** Copyright 2016 The WebRTC Project Authors.
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

#include <cxxkit/base/macros.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

CXXKIT_BEGIN_NAMESPACE

namespace
{
static int test_flag = 0;

static void ctor_func(void)
{
    fprintf(stdout, "tst_decl_ctor\n");
    test_flag = -1;
}
CXXKIT_CONSTRUCTOR_FUNCTION(ctor_func)

static void dtor_func(void)
{
    fprintf(stdout, "tst_decl_dtor\n");
    if (1 != test_flag)
    {
        abort();
    }
}
CXXKIT_DESTRUCTOR_FUNCTION(dtor_func)
} // namespace

TEST(CtorDtorTest, ToStringWithCause)
{
    EXPECT_EQ(test_flag, -1);
    test_flag = 1;
}

CXXKIT_END_NAMESPACE