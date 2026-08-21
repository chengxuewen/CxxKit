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

#pragma once

#include <cxxkit/tools/tools_global.hpp>

#include <cxxkit/text/string_view.hpp>
#include <cxxkit/memory/shared_data.hpp>

#include <map>
#include <limits>
#include <ostream>

#define CXXKIT_DECLARE_ERROR_DOMAIN(Export, Name) Export const cxxkit::Error::Domain &Name();

#define CXXKIT_DEFINE_ERROR_DOMAIN(Type, Name, Description)                                                            \
    const cxxkit::Error::Domain &Name()                                                                                \
    {                                                                                                                  \
        static const Type domain(cxxkit::Error::Domain::Registry::registerDomain(#Type, #Name, Description));          \
        return domain;                                                                                                 \
    }

CXXKIT_BEGIN_NAMESPACE

using ErrorId = int32_t;
class CXXKIT_TOOLS_API Error : public SharedData
{
public:
    using SharedDataPtr = ImplicitlySharedDataPointer<Error>;

    CXXKIT_STATIC_CONSTANT_NUMBER(kInvalidId, std::numeric_limits<ErrorId>::max())

    class CXXKIT_TOOLS_API Domain
    {
        StringView mType;
        StringView mName;
        StringView mDescription;
        ErrorId mId{kInvalidId};
        mutable std::atomic<bool> mCacheInitialized{false};

    public:
        struct CXXKIT_TOOLS_API Registry
        {
            static ErrorId registerDomain(StringView type, StringView name, StringView description = "");
        };

        Domain() = default;
        explicit Domain(ErrorId id);
        Domain(Domain &&other);
        Domain(const Domain &other);
        virtual ~Domain();

        Domain &operator=(const Domain &other)
        {
            mId = other.mId;
            return *this;
        }
        Domain &operator=(Domain &&other)
        {
            std::swap(mId, other.mId);
            return *this;
        }

        ErrorId id() const { return mId; }
        bool isValid() const { return kInvalidId != mId; }

        StringView type() const { return mType; }
        StringView name() const { return mName; }
        StringView description() const { return mDescription; }

        bool operator==(const Domain &other) const { return mId == other.mId; }
        bool operator!=(const Domain &other) const { return mId != other.mId; }

        virtual StringView codeString(ErrorId /*code*/) const { return ""; }
        virtual std::string toString(ErrorId code, StringView message = "") const
        {
            if (!this->isValid())
            {
                return std::string(message);
            }
            auto codeMessage = std::string(this->codeString(code));
            if (!codeMessage.empty())
            {
                codeMessage = "<" + codeMessage + ">";
            }
            if (!message.empty())
            {
                if (!codeMessage.empty())
                {
                    codeMessage += ": ";
                }
                codeMessage += message.data();
            }
            return std::string(this->type()) + "[" + std::to_string(code) + "]:" + codeMessage;
        }
    };

    Error(const Domain &domain, ErrorId code, StringView message, const SharedDataPtr &cause = {});
    Error(StringView message, const SharedDataPtr &cause = {});
    Error(const char *message, const SharedDataPtr &cause = {});
    Error(const Error &other);
    virtual ~Error();

    static SharedDataPtr create(const Domain &domain, ErrorId code, StringView message, const SharedDataPtr &cause = {})
    {
        return SharedDataPtr(new Error(domain, code, message, cause));
    }
    static SharedDataPtr create(StringView message, const SharedDataPtr &cause = {})
    {
        return SharedDataPtr(new Error(message, cause));
    }

    ErrorId code() const { return mCode; }
    const std::string &message() const { return mMessage; }
    const Domain &domain() const { return mDomain; }
    const Error *cause() const { return mCause.data(); }

    std::string toString() const
    {
        std::string string = mDomain.toString(mCode, mMessage);
        const Error *current = this;
        const int maxDepth = 10;
        int depth = 0;
        while (current->mCause && depth < maxDepth)
        {
            current = current->mCause.data();
            string += "\nCaused by: " + current->mDomain.toString(current->mCode, current->mMessage);
            ++depth;
        }
        if (current->mCause && depth >= maxDepth)
        {
            string += "\nCaused by: ... (error chain too deep)";
        }

        return string;
    }
    size_t depth() const
    {
        size_t depth = 0;
        for (const Error *error = mCause.data(); error != nullptr; error = error->mCause.data())
        {
            ++depth;
        }
        return depth;
    }

private:
    const Domain &mDomain;
    const ErrorId mCode;
    std::string mMessage;
    SharedDataPtr mCause;
};

inline std::ostream &operator<<(std::ostream &os, const Error &error)
{
    os << error.toString();
    return os;
}

CXXKIT_DECLARE_ERROR_DOMAIN(CXXKIT_TOOLS_API, invalidDomain)

CXXKIT_END_NAMESPACE
