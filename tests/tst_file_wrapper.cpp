/***********************************************************************************************************************
**
** Library: OpenCTK
**
** Copyright (C) 2025~Present ChengXueWen.
** Copyright (c) 2021 The WebRTC project authors. All Rights Reserved.
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

#include "p/file_utils_p.hpp"
#include "cxxkit/io/file_wrapper.hpp"
#include "cxxkit/tools/checks.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

CXXKIT_BEGIN_NAMESPACE

TEST(FileWrapper, FileSize)
{
    auto test_info = ::testing::UnitTest::GetInstance()->current_test_info();
    std::string test_name = std::string(test_info->test_case_name()) + "_" + test_info->name();
    std::replace(test_name.begin(), test_name.end(), '/', '_');
    const std::string temp_filename = test::OutputPathWithRandomDirectory() + test_name;

    // Write
    {
        FileWrapper file = FileWrapper::OpenWriteOnly(temp_filename);
        ASSERT_TRUE(file.is_open());
        EXPECT_EQ(file.FileSize(), 0);

        EXPECT_TRUE(file.Write("foo", 3));
        EXPECT_EQ(file.FileSize(), 3);

        // FileSize() doesn't change the file size.
        EXPECT_EQ(file.FileSize(), 3);

        // FileSize() doesn't move the write position.
        EXPECT_TRUE(file.Write("bar", 3));
        EXPECT_EQ(file.FileSize(), 6);
    }

    // Read
    {
        FileWrapper file = FileWrapper::OpenReadOnly(temp_filename);
        ASSERT_TRUE(file.is_open());
        EXPECT_EQ(file.FileSize(), 6);

        char buf[10];
        size_t bytes_read = file.Read(buf, 3);
        EXPECT_EQ(bytes_read, 3u);
        EXPECT_EQ(memcmp(buf, "foo", 3), 0);

        // FileSize() doesn't move the read position.
        EXPECT_EQ(file.FileSize(), 6);

        // Attempting to read past the end reads what is available
        // and sets the EOF flag.
        bytes_read = file.Read(buf, 5);
        EXPECT_EQ(bytes_read, 3u);
        EXPECT_EQ(memcmp(buf, "bar", 3), 0);
        EXPECT_TRUE(file.ReadEof());
    }

    // Clean up temporary file.
    remove(temp_filename.c_str());
}

CXXKIT_END_NAMESPACE
