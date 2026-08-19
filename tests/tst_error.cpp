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

#include <cxxkit/tools/error.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

CXXKIT_BEGIN_NAMESPACE

namespace
{
class TestDomain : public Error::Domain
{
public:
    enum Code
    {
        kTestError1 = 100,
        kTestError2 = 200,
        kTestError3 = 300
    };

    using Error::Domain::Domain;
    TestDomain() = default;

    StringView codeString(ErrorId code) const override
    {
        switch (code)
        {
            case kTestError1: return "Test error 1";
            case kTestError2: return "Test error 2";
            case kTestError3: return "Test error 3";
            default: return "";
        }
    }
};
CXXKIT_DEFINE_ERROR_DOMAIN(TestDomain, testDomain, "Test domain")

class AnotherDomain : public Error::Domain
{
public:
    enum Code
    {
        kAnotherError = 400
    };

    using Error::Domain::Domain;
    AnotherDomain() = default;

    StringView codeString(ErrorId code) const override
    {
        switch (code)
        {
            case kAnotherError: return "Another error";
            default: return "";
        }
    }
};
CXXKIT_DEFINE_ERROR_DOMAIN(AnotherDomain, anotherDomain, "Another domain")
} // namespace

TEST(ErrorTest, CreateSuccess)
{
    auto error = Error::create(testDomain(), TestDomain::kTestError1, "Test message");

    ASSERT_NE(error, nullptr);
    EXPECT_EQ(&error->domain(), &testDomain());
    EXPECT_EQ(error->code(), TestDomain::kTestError1);
    EXPECT_EQ(error->message(), "Test message");
    EXPECT_EQ(error->cause(), nullptr);
}

TEST(ErrorTest, CreateWithDefaultDomain)
{
    const auto error = Error::create("Default domain Test");
    auto domain = error->domain();
    EXPECT_TRUE(domain.id() == Error::kInvalidId);

    const auto errorShare = error;
    EXPECT_EQ(error->refCount(), 2);
    EXPECT_EQ(errorShare->refCount(), 2);

    EXPECT_EQ(error->message(), "Default domain Test");
    EXPECT_EQ(error->code(), Error::kInvalidId);

    EXPECT_EQ(error->refCount(), 2);
    EXPECT_EQ(errorShare->refCount(), 2);
    auto errorCopy = error;
    EXPECT_EQ(errorCopy->refCount(), 1);
}

TEST(ErrorTest, CreateWithEmptyMessage)
{
    auto error = Error::create(testDomain(), TestDomain::kTestError2, "");

    ASSERT_NE(error, nullptr);
    EXPECT_TRUE(error->message().empty());
}

TEST(ErrorTest, CreateWithCause)
{
    auto cause = Error::create(testDomain(), TestDomain::kTestError1, "Root cause");
    auto error = Error::create(testDomain(), TestDomain::kTestError2, "Wrapper error", cause);

    EXPECT_NE(error->cause(), nullptr);
    EXPECT_EQ(error->cause()->code(), TestDomain::kTestError1);
    EXPECT_EQ(error->cause()->message(), "Root cause");
}

TEST(ErrorTest, ToString)
{
    auto domain = testDomain();
    auto error = Error::create(domain, TestDomain::kTestError1, "Something went wrong");

    std::string string = error->toString();
    EXPECT_FALSE(string.empty());
    EXPECT_NE(string.find(domain.type().data()), std::string::npos) << string << ", " << domain.type().data();
    EXPECT_NE(string.find("100"), std::string::npos);
    EXPECT_NE(string.find("Something went wrong"), std::string::npos);
}

TEST(ErrorTest, ToStringWithCause)
{
    auto domain = testDomain();
    auto cause = Error::create(domain, TestDomain::kTestError1, "Root cause");
    auto error = Error::create(domain, TestDomain::kTestError2, "Wrapper", cause);

    std::string string = error->toString();
    EXPECT_NE(string.find("Wrapper"), std::string::npos);
    EXPECT_NE(string.find("Root cause"), std::string::npos);
    EXPECT_NE(string.find("Caused by"), std::string::npos);
}

TEST(ErrorTest, Depth)
{
    auto domain = testDomain();

    auto error0 = Error::create(domain, TestDomain::kTestError1, "Level 0");
    EXPECT_EQ(error0->depth(), 0u);

    auto error1 = Error::create(domain, TestDomain::kTestError2, "Level 1", error0);
    EXPECT_EQ(error1->depth(), 1u);

    auto error2 = Error::create(domain, TestDomain::kTestError3, "Level 2", error1);
    EXPECT_EQ(error2->depth(), 2u);
}

TEST(ErrorTest, DomainBasicOperations)
{
    auto domain = testDomain();

    // Test basic domain information
    EXPECT_TRUE(domain.isValid());
    EXPECT_NE(domain.id(), Error::kInvalidId);
    EXPECT_EQ(domain.type(), "TestDomain");
    EXPECT_EQ(domain.name(), "testDomain");
    EXPECT_EQ(domain.description(), "Test domain");

    // Test domain comparison operations
    auto sameDomain = testDomain();
    auto another = anotherDomain();

    EXPECT_EQ(domain, sameDomain);
    EXPECT_NE(domain, another);
    EXPECT_NE(domain, Error::Domain()); // Compare with default constructed invalid domain
}

TEST(ErrorTest, InvalidDomain)
{
    Error::Domain invalidDomain;
    EXPECT_FALSE(invalidDomain.isValid());
    EXPECT_EQ(invalidDomain.id(), Error::kInvalidId);
    EXPECT_EQ(invalidDomain.type(), "");
    EXPECT_EQ(invalidDomain.name(), "");
    EXPECT_EQ(invalidDomain.description(), "");

    // Test creating an error with an invalid domain
    auto error = Error::create(invalidDomain, 123, "Test");
    EXPECT_FALSE(error->domain().isValid()); // Should use default domain
}

TEST(ErrorTest, CopyAndMoveSemantics)
{
    // Test Error copy semantics
    auto cause = Error::create(testDomain(), TestDomain::kTestError2, "Cause");
    // Create original error with cause directly
    const auto original = Error::create(testDomain(), TestDomain::kTestError1, "Original", cause);

    const auto copied = original;
    EXPECT_EQ(copied->code(), original->code());
    EXPECT_EQ(copied->message(), original->message());
    EXPECT_EQ(&copied->domain(), &original->domain());
    EXPECT_EQ(copied->cause(), original->cause());
    EXPECT_EQ(copied->refCount(), 2); // Should share reference count

    // Test Error move semantics
    auto moved = std::move(original);
    EXPECT_EQ(moved->code(), TestDomain::kTestError1);
    EXPECT_EQ(moved->message(), "Original");
}

TEST(ErrorTest, DomainCopyAndMoveSemantics)
{
    auto domain = testDomain();

    // Test domain copy
    Error::Domain copied(domain);
    EXPECT_EQ(copied, domain);
    EXPECT_EQ(copied.id(), domain.id());
    EXPECT_EQ(copied.type(), domain.type());

    // Test domain move
    Error::Domain moved(std::move(copied));
    EXPECT_EQ(moved, domain);
    EXPECT_EQ(moved.id(), domain.id());

    // Test copy assignment
    Error::Domain copyAssigned;
    copyAssigned = domain;
    EXPECT_EQ(copyAssigned, domain);

    // Test move assignment
    Error::Domain moveAssigned;
    moveAssigned = std::move(copyAssigned);
    EXPECT_EQ(moveAssigned, domain);
}

TEST(ErrorTest, DomainRegistry)
{
    // Test domain registration (indirectly tested via CXXKIT_DEFINE_ERROR_DOMAIN macro)
    EXPECT_TRUE(testDomain().isValid());
    EXPECT_TRUE(anotherDomain().isValid());
    EXPECT_NE(testDomain().id(), anotherDomain().id());

    // Test domain type uniqueness
    EXPECT_EQ(testDomain().type(), "TestDomain");
    EXPECT_EQ(anotherDomain().type(), "AnotherDomain");
}

// Note: Hash collision tests may be difficult to trigger as the hash algorithm is well-designed
// but we can verify registration failure scenarios
TEST(ErrorTest, DomainRegistryConflictHandling)
{
    // Attempt to register a domain with the same type as an existing one (should fail)
    // Note: This requires access to Registry::registerDomain, which may require adjusting access permissions
    // Or testing conflict handling logic through other means
}

TEST(ErrorTest, ErrorChainMaxDepth)
{
    const int kMaxDepth = 10;
    auto error = Error::create(testDomain(), TestDomain::kTestError1, "Level 0");

    // Create an error chain with depth 11
    Error::SharedDataPtr current = error;
    for (int i = 1; i <= kMaxDepth + 1; ++i)
    {
        current = Error::create(testDomain(), TestDomain::kTestError1, "Level " + std::to_string(i), current);
    }

    // Check depth calculation
    EXPECT_EQ(current->depth(), static_cast<size_t>(kMaxDepth + 1));

    // Check if string representation is truncated
    std::string errorStr = current->toString();
    EXPECT_TRUE(errorStr.find("error chain too deep") != std::string::npos);
}

TEST(ErrorTest, MultipleDomains)
{
    // Test errors from different domains
    auto error1 = Error::create(testDomain(), TestDomain::kTestError1, "Test error");
    auto error2 = Error::create(anotherDomain(), AnotherDomain::kAnotherError, "Another error");

    EXPECT_NE(&error1->domain(), &error2->domain());
    EXPECT_NE(error1->code(), error2->code());
    EXPECT_NE(error1->toString(), error2->toString());
}

CXXKIT_END_NAMESPACE