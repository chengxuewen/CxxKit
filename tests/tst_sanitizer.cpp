/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2026~Present ChengXueWen.
** Copyright 2018 The WebRTC Project Authors.
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

#include <cxxkit/tools/sanitizer.hpp>
#include <cxxkit/tools/logging.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#if CXXKIT_HAS_MSAN
#    include <sanitizer/msan_interface.h>
#endif

CXXKIT_BEGIN_NAMESPACE

namespace
{

// Test traits::is_trivially_copyable_v (at compile time).
// Trivially copyable.

struct TrTrTr
{
    TrTrTr(const TrTrTr &) = default;
    TrTrTr &operator=(const TrTrTr &) = default;
    ~TrTrTr() = default;
};
static_assert(traits::is_trivially_copyable_v<TrTrTr>, "");

struct TrDeTr
{
    TrDeTr(const TrDeTr &) = default;
    TrDeTr &operator=(const TrDeTr &) = delete;
    ~TrDeTr() = default;
};
static_assert(traits::is_trivially_copyable_v<TrDeTr>, "");

// Non trivially copyable.

struct TrTrNt
{
    TrTrNt(const TrTrNt &) = default;
    TrTrNt &operator=(const TrTrNt &) = default;
    ~TrTrNt();
};
static_assert(!traits::is_trivially_copyable_v<TrTrNt>, "");

struct TrNtTr
{
    TrNtTr(const TrNtTr &) = default;
    TrNtTr &operator=(const TrNtTr &);
    ~TrNtTr() = default;
};
static_assert(!traits::is_trivially_copyable_v<TrNtTr>, "");

struct TrNtNt
{
    TrNtNt(const TrNtNt &) = default;
    TrNtNt &operator=(const TrNtNt &);
    ~TrNtNt();
};
static_assert(!traits::is_trivially_copyable_v<TrNtNt>, "");

struct TrDeNt
{
    TrDeNt(const TrDeNt &) = default;
    TrDeNt &operator=(const TrDeNt &) = delete;
    ~TrDeNt();
};
static_assert(!traits::is_trivially_copyable_v<TrDeNt>, "");

struct NtTrTr
{
    NtTrTr(const NtTrTr &);
    NtTrTr &operator=(const NtTrTr &) = default;
    ~NtTrTr() = default;
};
static_assert(!traits::is_trivially_copyable_v<NtTrTr>, "");

struct NtTrNt
{
    NtTrNt(const NtTrNt &);
    NtTrNt &operator=(const NtTrNt &) = default;
    ~NtTrNt();
};
static_assert(!traits::is_trivially_copyable_v<NtTrNt>, "");

struct NtNtTr
{
    NtNtTr(const NtNtTr &);
    NtNtTr &operator=(const NtNtTr &);
    ~NtNtTr() = default;
};
static_assert(!traits::is_trivially_copyable_v<NtNtTr>, "");

struct NtNtNt
{
    NtNtNt(const NtNtNt &);
    NtNtNt &operator=(const NtNtNt &);
    ~NtNtNt();
};
static_assert(!traits::is_trivially_copyable_v<NtNtNt>, "");

struct NtDeTr
{
    NtDeTr(const NtDeTr &);
    NtDeTr &operator=(const NtDeTr &) = delete;
    ~NtDeTr() = default;
};
static_assert(!traits::is_trivially_copyable_v<NtDeTr>, "");

struct NtDeNt
{
    NtDeNt(const NtDeNt &);
    NtDeNt &operator=(const NtDeNt &) = delete;
    ~NtDeNt();
};
static_assert(!traits::is_trivially_copyable_v<NtDeNt>, "");

// Trivially copyable types.

struct Foo
{
    uint32_t field1;
    uint16_t field2;
};

struct Bar
{
    uint32_t ID;
    Foo foo;
};

// Run the callback, and expect a crash if it *doesn't* make an uninitialized
// memory read. If MSan isn't on, just run the callback.
template <typename F>
void MsanExpectUninitializedRead(F &&f)
{
#if CXXKIT_HAS_MSAN
    EXPECT_DEATH(f(), "");
#else
    f();
#endif
}

} // namespace

TEST(SanitizerTest, MsanUninitialized)
{
    Bar bar = MsanUninitialized<Bar>({});
    // Check that a read after initialization is OK.
    bar.ID = 1;
    EXPECT_EQ(1u, bar.ID);
    CXXKIT_INFO() << "read after init passed";
    // Check that other fields are uninitialized and equal to zero.
    MsanExpectUninitializedRead([&] { EXPECT_EQ(0u, bar.foo.field1); });
    MsanExpectUninitializedRead([&] { EXPECT_EQ(0u, bar.foo.field2); });
    CXXKIT_INFO() << "read with no init passed";
}

CXXKIT_END_NAMESPACE