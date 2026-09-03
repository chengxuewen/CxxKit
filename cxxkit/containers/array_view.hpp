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

#include <cxxkit/base/global.hpp>
#include <cxxkit/tools/checks.hpp>
#include <cxxkit/tools/limits.hpp>
#include <cxxkit/tools/iterator.hpp>
#include <cxxkit/tools/type_traits.hpp>

#include <array>
#include <algorithm>
#include <stdexcept>

CXXKIT_BEGIN_NAMESPACE

// tl;dr: rtc::ArrayView is the same thing as gsl::span from the Guideline
//        Support Library.
//
// Many functions read from or write to arrays. The obvious way to do this is
// to use two arguments, a Pointer to the first element and an element count:
//
//   bool Contains17(const int* arr, size_t size) {
//     for (size_t i = 0; i < size; ++i) {
//       if (arr[i] == 17)
//         return true;
//     }
//     return false;
//   }
//
// This is flexible, since it doesn't matter how the array is stored (C array,
// std::vector, rtc::Buffer, ...), but it's error-prone because the caller has
// to correctly specify the array length:
//
//   Contains17(arr, arraysize(arr));     // C array
//   Contains17(arr.data(), arr.size());  // std::vector
//   Contains17(arr, size);               // Pointer + size
//   ...
//
// It's also kind of messy to have two separate arguments for what is
// conceptually a single thing.
//
// Enter rtc::ArrayView<T>. It contains a T Pointer (to an array it doesn't
// own) and a count, and supports the basic things you'd expect, such as
// indexing and iteration. It allows us to write our function like this:
//
//   bool Contains17(rtc::ArrayView<const int> arr) {
//     for (auto e : arr) {
//       if (e == 17)
//         return true;
//     }
//     return false;
//   }
//
// And even better, because a bunch of things will implicitly convert to
// ArrayView, we can call it like this:
//
//   Contains17(arr);                             // C array
//   Contains17(arr);                             // std::vector
//   Contains17(rtc::ArrayView<int>(arr, size));  // Pointer + size
//   Contains17(nullptr);                         // nullptr -> empty ArrayView
//   ...
//
// ArrayView<T> stores both a Pointer and a size, but you may also use
// ArrayView<T, N>, which has a size that's fixed at compile time (which means
// it only has to store the Pointer).
//
// One important point is that ArrayView<T> and ArrayView<const T> are
// different types, which allow and don't allow mutation of the array elements,
// respectively. The implicit conversions work just like you'd hope, so that
// e.g. vector<int> will convert to either ArrayView<int> or ArrayView<const
// int>, but const vector<int> will convert only to ArrayView<const int>.
// (ArrayView itself can be the source type in such conversions, so
// ArrayView<int> will convert to ArrayView<const int>.)
//
// Note: ArrayView is tiny (just a Pointer and a count if variable-sized, just
// a Pointer if fix-sized) and trivially copyable, so it's probably cheaper to
// pass it by value than by const Reference.

namespace detail
{
// Magic constant for indicating that the size of an ArrayView is variable instead of fixed.
enum : ptrdiff_t
{
    kArrayViewVarSize = -4711
};

// Base class for ArrayViews of fixed nonzero size.
template <typename T, ptrdiff_t Size>
class ArrayViewBase
{
    static_assert(Size > 0, "ArrayView size must be variable or non-negative");

public:
    ArrayViewBase(T *data, size_t /* size */)
        : mData(data)
    {
    }

    static constexpr size_t size() { return Size; }
    static constexpr bool empty() { return false; }
    T *data() const { return mData; }

protected:
    static constexpr bool fixed_size() { return true; }

private:
    T *mData;
};

// Specialized base class for ArrayViews of fixed zero size.
template <typename T>
class ArrayViewBase<T, 0>
{
public:
    explicit ArrayViewBase(T * /* data */, size_t /* size */) { }

    static constexpr size_t size() { return 0; }
    static constexpr bool empty() { return true; }
    T *data() const { return nullptr; }

protected:
    static constexpr bool fixed_size() { return true; }
};

// Specialized base class for ArrayViews of variable size.
template <typename T>
class ArrayViewBase<T, detail::kArrayViewVarSize>
{
public:
    ArrayViewBase(T *data, size_t size)
        : mData(size == 0 ? nullptr : data)
        , mSize(size)
    {
    }

    size_t size() const { return mSize; }
    bool empty() const { return mSize == 0; }
    T *data() const { return mData; }

protected:
    static constexpr bool fixed_size() { return false; }

private:
    T *mData;
    size_t mSize;
};
} // namespace detail

template <typename T, std::ptrdiff_t Size = detail::kArrayViewVarSize>
class ArrayView final : public detail::ArrayViewBase<T, Size>
{
public:
    using value_type = T;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using const_iterator = const T *;

    using ValueType = value_type;
    using Reference = reference;
    using ConstReference = const_reference;
    using Pointer = pointer;
    using ConstPointer = const_pointer;
    using ConstIterator = const_iterator;

    // Construct an ArrayView from a Pointer and a length.
    template <typename U>
    ArrayView(U *data, size_t size)
        : detail::ArrayViewBase<T, Size>::ArrayViewBase(data, size)
    {
        CXXKIT_DCHECK_EQ(size == 0 ? nullptr : data, this->data());
        CXXKIT_DCHECK_EQ(size, this->size());
        CXXKIT_DCHECK_EQ(!this->data(), this->size() == 0); // data is null iff size == 0.
    }

    // Construct an empty ArrayView. Note that fixed-size ArrayViews of size > 0 cannot be empty.
    ArrayView()
        : ArrayView(nullptr, 0)
    {
    }
    ArrayView(std::nullptr_t)
        : ArrayView()
    {
    } // NOLINT
    ArrayView(std::nullptr_t, size_t size)
        : ArrayView(static_cast<T *>(nullptr), size)
    {
        static_assert(Size == 0 || Size == detail::kArrayViewVarSize, "");
        CXXKIT_DCHECK_EQ(0, size);
    }

    // Construct an ArrayView from a C-style array.
    template <typename U, size_t N>
    ArrayView(U (&array)[N])
        : ArrayView(array, N) // NOLINT
    {
        static_assert(Size == N || Size == detail::kArrayViewVarSize, "Array size must match ArrayView size");
    }

    // (Only if size is fixed.) Construct a fixed size ArrayView<T, N> from a
    // non-const std::array instance. For an ArrayView with variable size, the
    // used ctor is ArrayView(U& u) instead.
    template <typename U, size_t N, typename std::enable_if<Size == static_cast<std::ptrdiff_t>(N)>::type * = nullptr>
    ArrayView(std::array<U, N> &u)
        : ArrayView(u.data(), u.size())
    {
    } // NOLINT

    // (Only if size is fixed.) Construct a fixed size ArrayView<T, N> where T is
    // const from a const(expr) std::array instance. For an ArrayView with
    // variable size, the used ctor is ArrayView(U& u) instead.
    template <typename U, size_t N, typename std::enable_if<Size == static_cast<std::ptrdiff_t>(N)>::type * = nullptr>
    ArrayView(const std::array<U, N> &u) // NOLINT
        : ArrayView(u.data(), u.size())
    {
    }

    // (Only if size is fixed.) Construct an ArrayView from any type U that has a
    // static constexpr size() method whose return value is equal to Size, and a
    // data() method whose return value converts implicitly to T*. In particular,
    // this means we allow conversion from ArrayView<T, N> to ArrayView<const T,
    // N>, but not the other way around. We also don't allow conversion from
    // ArrayView<T> to ArrayView<T, N>, or from ArrayView<T, M> to ArrayView<T,
    // N> when M != N.
    template <
        typename U,
        typename std::enable_if<Size != detail::kArrayViewVarSize && HasDataAndSize<U, T>::value>::type * = nullptr>
    ArrayView(U &u) // NOLINT
        : ArrayView(u.data(), u.size())
    {
        static_assert(U::size() == Size, "Sizes must match exactly");
    }
    template <
        typename U,
        typename std::enable_if<Size != detail::kArrayViewVarSize && HasDataAndSize<U, T>::value>::type * = nullptr>
    ArrayView(const U &u) // NOLINT(runtime/explicit)
        : ArrayView(u.data(), u.size())
    {
        static_assert(U::size() == Size, "Sizes must match exactly");
    }

    // (Only if size is variable.) Construct an ArrayView from any type U that
    // has a size() method whose return value converts implicitly to size_t, and
    // a data() method whose return value converts implicitly to T*. In
    // particular, this means we allow conversion from ArrayView<T> to
    // ArrayView<const T>, but not the other way around. Other allowed
    // conversions include
    // ArrayView<T, N> to ArrayView<T> or ArrayView<const T>,
    // std::vector<T> to ArrayView<T> or ArrayView<const T>,
    // const std::vector<T> to ArrayView<const T>,
    // rtc::Buffer to ArrayView<uint8_t> or ArrayView<const uint8_t>, and
    // const rtc::Buffer to ArrayView<const uint8_t>.
    template <
        typename U,
        typename std::enable_if<Size == detail::kArrayViewVarSize && HasDataAndSize<U, T>::value>::type * = nullptr>
    ArrayView(U &u) // NOLINT
        : ArrayView(u.data(), u.size())
    {
    }
    template <
        typename U,
        typename std::enable_if<Size == detail::kArrayViewVarSize && HasDataAndSize<U, T>::value>::type * = nullptr>
    ArrayView(const U &u) // NOLINT(runtime/explicit)
        : ArrayView(u.data(), u.size())
    {
    }

    // Indexing and iteration. These allow mutation even if the ArrayView is
    // const, because the ArrayView doesn't own the array. (To prevent mutation,
    // use a const element type.)
    /// @brief Bounds-checked element access; DCHECKs idx < size() and data() != nullptr.
    T &operator[](size_t idx) const
    {
        CXXKIT_DCHECK_LT(idx, this->size());
        CXXKIT_DCHECK(this->data());
        return this->data()[idx];
    }
    T *begin() const { return this->data(); }
    T *end() const { return this->data() + this->size(); }
    const T *cbegin() const { return this->data(); }
    const T *cend() const { return this->data() + this->size(); }
    std::reverse_iterator<T *> rbegin() const { return utils::make_reverse_iterator(this->end()); }
    std::reverse_iterator<T *> rend() const { return utils::make_reverse_iterator(this->begin()); }
    std::reverse_iterator<const T *> crbegin() const { return utils::make_reverse_iterator(this->cend()); }
    std::reverse_iterator<const T *> crend() const { return utils::make_reverse_iterator(this->cbegin()); }

    /// @brief Returns a subview starting at @p offset with at most @p size elements.
    ArrayView<T> subview(size_t offset, size_t size) const
    {
        return offset < this->size() ? ArrayView<T>(this->data() + offset, utils::math_min(size, this->size() - offset))
                                     : ArrayView<T>();
    }
    ArrayView<T> subview(size_t offset) const { return subview(offset, this->size()); }
    /// @brief Returns a subview from @p offset to the end of the view.

    /// @brief Bounds-checked element access; throws std::out_of_range if idx >= size().
    T &at(size_t idx)
    {
        if (idx >= this->size())
        {
            throw std::out_of_range("ArrayView::at");
        }
        return this->data()[idx];
    }

    /// @brief Bounds-checked element access (const); throws std::out_of_range if idx >= size().
    const T &at(size_t idx) const
    {
        if (idx >= this->size())
        {
            throw std::out_of_range("ArrayView::at");
        }
        return this->data()[idx];
    }
};


// Comparing two ArrayViews compares their (Pointer,size) pairs; it does *not*
// dereference the pointers.
/**
 * @brief Compares two ArrayViews by (pointer, size) pair only.
 * Does NOT dereference the pointers — checks identity of the underlying data.
 */
template <typename T, std::ptrdiff_t Size1, std::ptrdiff_t Size2>
bool operator==(const ArrayView<T, Size1> &a, const ArrayView<T, Size2> &b)
{
    return a.data() == b.data() && a.size() == b.size();
}
template <typename T, std::ptrdiff_t Size1, std::ptrdiff_t Size2>
/** @brief Negated equality. Two ArrayViews are unequal iff their data() or size() differ. */
bool operator!=(const ArrayView<T, Size1> &a, const ArrayView<T, Size2> &b)
{
    return !(a == b);
}

// Variable-size ArrayViews are the size of two pointers; fixed-size ArrayViews
// are the size of one Pointer. (And as a special case, fixed-size ArrayViews
// of size 0 require no storage.)
static_assert(sizeof(ArrayView<int>) == 2 * sizeof(int *), "");
static_assert(sizeof(ArrayView<int, 17>) == sizeof(int *), "");
static_assert(std::is_empty<ArrayView<int, 0>>::value, "");

namespace utils
{
template <typename T>
/// @brief Factory: creates an ArrayView from a raw pointer and size.
inline ArrayView<T> make_array_view(T *data, size_t size)
{
    return ArrayView<T>(data, size);
}

// Only for primitive types that have the same size and aligment.
// Allow reinterpret cast of the array view to another primitive type of the
// same size.
// Template arguments order is (U, T, Size) to allow deduction of the template
// arguments in client calls: reinterpret_array_view<target_type>(array_view).
/**
 * @brief Reinterpret-cast an ArrayView to another fundamental type of identical size and alignment.
 * Requires both types to be fundamental (primitive) with matching sizeof/alignof.
 * @tparam U Target type.
 * @tparam T Source type.
 * @tparam Size Compile-time size (shared between source and target views).
 * @param view Source view.
 * @return ArrayView<U, Size> over the same memory region.
 */
template <typename U, typename T, std::ptrdiff_t Size>
inline ArrayView<U, Size> reinterpret_array_view(ArrayView<T, Size> view)
{
    static_assert(sizeof(U) == sizeof(T) && alignof(U) == alignof(T),
                  "ArrayView reinterpret_cast is only supported for casting "
                  "between views that represent the same chunk of memory.");
    static_assert(std::is_fundamental<T>::value && std::is_fundamental<U>::value,
                  "ArrayView reinterpret_cast is only supported for casting between "
                  "fundamental types.");
    return ArrayView<U, Size>(reinterpret_cast<U *>(view.data()), view.size());
}
} // namespace utils

/// @brief Factory: creates a mutable ArrayView from a raw pointer and size.
template <typename T>
inline ArrayView<T> make_array_view(T *data, size_t size)
{
    return ArrayView<T>(data, size);
}

/// @brief Factory: creates a const ArrayView from a raw pointer and size.
template <typename T>
inline ArrayView<const T> make_const_array_view(const T *data, size_t size)
{
    return ArrayView<const T>(data, size);
}


CXXKIT_END_NAMESPACE