/***********************************************************************************************************************
**
** Library: cxxkit
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

#include "cxxkit/tools/result.hpp"
#include "cxxkit/tools/error.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

CXXKIT_BEGIN_NAMESPACE

namespace
{
class TestDomain : public Error::Domain
{
public:
    enum Code
    {
        kTestError1 = 100,
        kTestError2 = 200
    };

    using Error::Domain::Domain;
    TestDomain() = default;

    StringView codeString(ErrorId code) const override
    {
        switch (code)
        {
            case kTestError1: return "Test error 1";
            case kTestError2: return "Test error 2";
            default: return "";
        }
    }
};
CXXKIT_DEFINE_ERROR_DOMAIN(TestDomain, testDomain, "Test domain")
} // namespace

// Test default constructor
TEST(ResultTest, DefaultConstructor)
{
    Result<int> result;
    EXPECT_FALSE(result.ok());
    EXPECT_FALSE(result.isOk());
    EXPECT_FALSE(result.success());
    EXPECT_FALSE(result.isSuccess());
    EXPECT_FALSE(static_cast<bool>(result));
    EXPECT_EQ(result.error(), nullptr);
}

// Test value constructors and value access
TEST(ResultTest, ValueConstructors)
{
    // Test lvalue constructor
    int value = 42;
    Result<int> result1(value);
    EXPECT_TRUE(result1.ok());
    EXPECT_EQ(result1.value(), 42);

    // Test rvalue constructor
    Result<std::string> result2(std::string("test"));
    EXPECT_TRUE(result2.ok());
    EXPECT_EQ(result2.value(), "test");

    // Test move value constructor
    std::vector<int> vec = {1, 2, 3};
    Result<std::vector<int>> result3(std::move(vec));
    EXPECT_TRUE(result3.ok());
    EXPECT_THAT(result3.value(), testing::ElementsAre(1, 2, 3));
    EXPECT_TRUE(vec.empty());
}

// Test error constructors
TEST(ResultTest, ErrorConstructors)
{
    // Test Error::Domain constructor
    Result<int> result1(testDomain(), TestDomain::kTestError1, "Error message");
    EXPECT_FALSE(result1.ok());
    EXPECT_NE(result1.error(), nullptr);
    EXPECT_EQ(result1.error()->domain().id(), testDomain().id());
    EXPECT_EQ(result1.error()->code(), TestDomain::kTestError1);
    EXPECT_EQ(result1.error()->message(), "Error message");

    // Test const char* message constructor
    Result<int> result2(Error::create("Simple error"));
    EXPECT_FALSE(result2.ok());
    EXPECT_NE(result2.error(), nullptr);
    EXPECT_EQ(result2.error()->message(), "Simple error");

    // Test StringView message constructor
    StringView msg = "StringView error";
    Result<int> result3(Error::create(msg));
    EXPECT_FALSE(result3.ok());
    EXPECT_NE(result3.error(), nullptr);
    EXPECT_EQ(result3.error()->message(), "StringView error");

    // Test Error::SharedDataPtr constructor
    auto errorPtr = Error::create(testDomain(), TestDomain::kTestError2, "Pointer error");
    Result<int> result4(errorPtr);
    EXPECT_FALSE(result4.ok());
    EXPECT_EQ(result4.error()->code(), TestDomain::kTestError2);

    // Test Error::SharedDataPtr move constructor
    Result<int> result5(std::move(errorPtr));
    EXPECT_FALSE(result5.ok());
    EXPECT_EQ(result5.error()->code(), TestDomain::kTestError2);
}

// Test copy and move semantics
TEST(ResultTest, CopyAndMoveSemantics)
{
    // Test copy constructor
    Result<int> result1(42);
    Result<int> result2(result1);
    EXPECT_TRUE(result2.ok());
    EXPECT_EQ(result2.value(), 42);

    // Test move constructor
    Result<std::string> result3("test");
    Result<std::string> result4(std::move(result3));
    EXPECT_TRUE(result4.ok());
    EXPECT_EQ(result4.value(), "test");

    // Test copy assignment
    Result<int> result5;
    result5 = result1;
    EXPECT_TRUE(result5.ok());
    EXPECT_EQ(result5.value(), 42);

    // Test move assignment
    Result<std::string> result6;
    Result<std::string> result7("move test");
    result6 = std::move(result7);
    EXPECT_TRUE(result6.ok());
    EXPECT_EQ(result6.value(), "move test");
}

// Test convertible type operations
TEST(ResultTest, ConvertibleTypes)
{
    // Test copy from convertible type
    Result<int> intResult(42);
    Result<double> doubleResult(intResult);
    EXPECT_TRUE(doubleResult.ok());
    EXPECT_DOUBLE_EQ(doubleResult.value(), 42.0);

    // Test move from convertible type
    Result<std::string> stringResult(std::string("test"));
    Result<std::string_view> stringViewResult(std::move(stringResult));
    EXPECT_TRUE(stringViewResult.ok());
    EXPECT_EQ(stringViewResult.value(), "test");

    // Test copy assignment from convertible type
    Result<int> intResult2(100);
    Result<double> doubleResult2;
    doubleResult2 = intResult2;
    EXPECT_TRUE(doubleResult2.ok());
    EXPECT_DOUBLE_EQ(doubleResult2.value(), 100.0);

    // Test move assignment from convertible type
    Result<std::string> stringResult2(std::string("assign test"));
    Result<std::string_view> stringViewResult2;
    stringViewResult2 = std::move(stringResult2);
    EXPECT_TRUE(stringViewResult2.ok());
    EXPECT_EQ(stringViewResult2.value(), "assign test");
}

// Test success/failure checking methods
TEST(ResultTest, SuccessFailureChecking)
{
    Result<int> successResult(42);
    Result<int> failureResult(Error::create("Error"));

    // Test ok() methods
    EXPECT_TRUE(successResult.ok());
    EXPECT_FALSE(failureResult.ok());

    // Test isOk() methods
    EXPECT_TRUE(successResult.isOk());
    EXPECT_FALSE(failureResult.isOk());

    // Test success() methods
    EXPECT_TRUE(successResult.success());
    EXPECT_FALSE(failureResult.success());

    // Test isSuccess() methods
    EXPECT_TRUE(successResult.isSuccess());
    EXPECT_FALSE(failureResult.isSuccess());

    // Test bool conversion
    EXPECT_TRUE(static_cast<bool>(successResult));
    EXPECT_FALSE(static_cast<bool>(failureResult));

    // Test in conditional
    bool condition = false;
    if (successResult)
    {
        condition = true;
    }
    EXPECT_TRUE(condition);
}

// Test value access methods
TEST(ResultTest, ValueAccess)
{
    // Test value()
    Result<int> result1(42);
    EXPECT_EQ(result1.value(), 42);

    // Test valueOr()
    Result<int> result2(Error::create("Error"));
    EXPECT_EQ(result2.valueOr(99), 99);
    EXPECT_EQ(result1.valueOr(99), 42);

    // Test valueOrElse()
    int counter = 0;
    auto defaultFunc = [&counter]()
    {
        counter++;
        return 100;
    };

    EXPECT_EQ(result2.valueOrElse(defaultFunc), 100);
    EXPECT_EQ(counter, 1);
    EXPECT_EQ(result1.valueOrElse(defaultFunc), 42);
    EXPECT_EQ(counter, 1); // defaultFunc should not be called

    // Test value() const
    const Result<int> constResult(55);
    EXPECT_EQ(constResult.value(), 55);

    // Test value() &&
    Result<std::string> moveResult(std::string("move value"));
    std::string moved = std::move(moveResult).value();
    EXPECT_EQ(moved, "move value");
}

// Test error access methods
TEST(ResultTest, ErrorAccess)
{
    Result<int> result(testDomain(), TestDomain::kTestError1, "Test error");

    // Test error()
    EXPECT_NE(result.error(), nullptr);
    EXPECT_EQ(result.error()->code(), TestDomain::kTestError1);

    // Test error() const
    const Result<int> constResult = result;
    EXPECT_NE(constResult.error(), nullptr);

    // Test errorString()
    std::string errorStr = result.errorString();
    EXPECT_FALSE(errorStr.empty());
    std::cout << errorStr << std::endl;
    EXPECT_NE(errorStr.find("Test error 1"), std::string::npos) << errorStr;

    // Test errorString() on success
    Result<int> successResult(42);
    EXPECT_TRUE(successResult.errorString().empty());
}

// Test swap operations
TEST(ResultTest, SwapOperations)
{
    Result<int> result1(42);
    Result<int> result2(Error::create("Error"));

    // Test member swap
    result1.swap(result2);
    EXPECT_FALSE(result1.ok());
    EXPECT_TRUE(result2.ok());
    EXPECT_EQ(result2.value(), 42);

    // Test non-member swap
    swap(result1, result2);
    EXPECT_TRUE(result1.ok());
    EXPECT_FALSE(result2.ok());
    EXPECT_EQ(result1.value(), 42);
}

// Test error with cause
TEST(ResultTest, ErrorWithCause)
{
    auto cause = Error::create(testDomain(), TestDomain::kTestError1, "Root cause");
    Result<int> result(testDomain(), TestDomain::kTestError2, "Wrapper error", cause);

    EXPECT_FALSE(result.ok());
    EXPECT_NE(result.error(), nullptr);
    EXPECT_NE(result.error()->cause(), nullptr);
    EXPECT_EQ(result.error()->cause()->code(), TestDomain::kTestError1);
    EXPECT_EQ(result.error()->cause()->message(), "Root cause");
}

// Test Result of void-like type (not really void, but unit type)
TEST(ResultTest, ResultOfUnitType)
{
    struct Unit
    {
    };

    Result<Unit> result(Unit{});
    EXPECT_TRUE(result.ok());

    // Test value access on unit type
    [[maybe_unused]] auto unit = result.value();
}

CXXKIT_END_NAMESPACE
