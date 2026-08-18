/***********************************************************************************************************************
**
** Library: cxxkit
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

#ifndef _CXXKIT_INLINED_VECTOR_HPP
#define _CXXKIT_INLINED_VECTOR_HPP

#include <cxxkit/base/global.hpp>
#include <cxxkit/tools/checks.hpp>
#include <cxxkit/tools/exception.hpp>
#include <cxxkit/tools/type_traits.hpp>

#include <vector>
#include <cassert>
#include <algorithm>
#include <type_traits>

CXXKIT_BEGIN_NAMESPACE
namespace detail
{
template <typename Iterator>
using IsAtLeastForwardIterator =
    std::is_convertible<typename std::iterator_traits<Iterator>::iterator_category, std::forward_iterator_tag>;
template <typename Iterator>
using EnableIfAtLeastForwardIterator = typename std::enable_if<IsAtLeastForwardIterator<Iterator>::value, int>::type;
template <typename Iterator>
using DisableIfAtLeastForwardIterator = typename std::enable_if<!IsAtLeastForwardIterator<Iterator>::value, int>::type;
} // namespace detail
template <typename T, size_t N, typename A = std::allocator<T>> class InlinedVector : public std::vector<T, A>
{
public:
    using Self = InlinedVector<T, N>;
    using Base = std::vector<T, A>;

    using value_type = T;
    using size_type = size_t;
    using allocator_type = A;
    using reference = typename Base::reference;
    using const_reference = typename Base::const_reference;
    using iterator = typename Base::iterator;
    using const_iterator = typename Base::const_iterator;
    using reverse_iterator = typename Base::reverse_iterator;
    using const_reverse_iterator = typename Base::const_reverse_iterator;

    /**
     * @brief Creates an empty inlined vector with a value-initialized allocator.
     */
    InlinedVector() noexcept(noexcept(allocator_type()))
        : Base()
    {
        this->reserve(N);
    }
    /**
     * @brief Creates an empty inlined vector with a copy of `allocator`.
     * @param allocator
     */
    explicit InlinedVector(const allocator_type &allocator) noexcept
        : Base(allocator)
    {
        this->reserve(N);
    }
    /**
     * @brief Creates an inlined vector with `n` copies of `value_type()`.
     * @param n
     * @param allocator
     */
    explicit InlinedVector(size_type n, const allocator_type &allocator = allocator_type())
        : Base(allocator)
    {
        this->reserve(N);
        this->resize(n);
    }
    /**
     * @brief Creates an inlined vector with `n` copies of `v`.
     * @param n
     * @param v
     * @param allocator
     */
    InlinedVector(size_type n, const_reference v, const allocator_type &allocator = allocator_type())
        : Base(n, v, allocator)
    {
        this->reserve(N);
    }
    /**
     * @brief Creates an inlined vector with copies of the elements of `list`.
     * @param list
     * @param allocator
     */
    InlinedVector(std::initializer_list<value_type> list, const allocator_type &allocator = allocator_type())
        : Base(list.begin(), list.end(), allocator)
    {
        this->reserve(N);
    }
    /**
     * @brief Creates an inlined vector with elements constructed from the provided
     * forward iterator range [`first`, `last`).
     *
     * NOTE: the `enable_if` prevents ambiguous interpretation between a call to this constructor with two integral
     * arguments and a call to the above `InlinedVector(size_type, const_reference)` constructor.
     * @tparam ForwardIterator
     * @param first
     * @param last
     * @param allocator
     */
    template <typename ForwardIterator, detail::EnableIfAtLeastForwardIterator<ForwardIterator> = 0>
    InlinedVector(ForwardIterator first, ForwardIterator last, const allocator_type &allocator = allocator_type())
        : Base(first, last, allocator)
    {
        this->reserve(N);
    }

    /**
     * @brief Creates an inlined vector with elements constructed from the provided input iterator range [`first`, `last`).
     * @tparam InputIterator
     * @param first
     * @param last
     * @param allocator
     */
    template <typename InputIterator, detail::DisableIfAtLeastForwardIterator<InputIterator> = 0>
    InlinedVector(InputIterator first, InputIterator last, const allocator_type &allocator = allocator_type())
        : Base(first, last, allocator)
    {
        this->reserve(N);
    }

    /**
     * @brief Creates an inlined vector by copying the contents of `other` using `other`'s allocator.
     * @param other
     */
    InlinedVector(const InlinedVector &other)
        : InlinedVector(other, other.get_allocator())
    {
        this->reserve(N);
    }
    /**
     * @brief Creates an inlined vector by copying the contents of `other` using the provided `allocator`.
     * @param other
     * @param allocator
     */
    InlinedVector(const InlinedVector &other, const allocator_type &allocator)
        : Base(other, allocator)
    {
        this->reserve(N);
    }
    /**
     * @brief Creates an inlined vector by moving in the contents of `other` without allocating.
     * If `other` contains allocated memory, the newly-created inlined vector will take ownership of that memory.
     * However, if `other` does not contain allocated memory, the newly-created inlined vector will perform
     * element-wise move construction of the contents of `other`.
     *
     * NOTE: since no allocation is performed for the inlined vector in either case, the `noexcept(...)` specification
     * depends on whether moving the underlying objects can throw.
     * It is assumed assumed that...
     *  a) move constructors should only throw due to allocation failure.
     *  b) if `value_type`'s move constructor allocates, it uses the same allocation
     *     function as the inlined vector's allocator.
     *     Thus, the move constructor is non-throwing if the allocator is non-throwing
     *     or `value_type`'s move constructor is specified as `noexcept`.
     * @param other
     */
    InlinedVector(InlinedVector &&other) noexcept(std::is_nothrow_move_constructible<value_type>::value)
        : Base(std::forward<InlinedVector &&>(other), other.get_allocator())
    {
        this->reserve(N);
    }
    /**
     * @brief Creates an inlined vector by moving in the contents of `other` with a copy of `allocator`.
     * NOTE: if `other`'s allocator is not equal to `allocator`, even if `other`
     * contains allocated memory, this move constructor will still allocate.
     * Since allocation is performed, this constructor can only be `noexcept`
     * if the specified allocator is also `noexcept`.
     * @param other
     * @param allocator
     */
    InlinedVector(InlinedVector &&other, const allocator_type &allocator) noexcept
        : Base(std::forward<InlinedVector &&>(other), allocator)
    {
        this->reserve(N);
    }
    /**
     * @brief Replaces the elements of the inlined vector with copies of the elements of `il`.
     * @param list
     * @return
     */
    InlinedVector &operator=(std::initializer_list<value_type> il)
    {
        this->assign(il.begin(), il.end());
        return *this;
    }
    /**
     * @brief Overload of `InlinedVector::operator=(...)` that replaces the elements of the inlined vector with
     * copies of the elements of `other`.
     * @param other
     * @return
     */
    InlinedVector &operator=(const InlinedVector &other)
    {
        Base::operator=(other);
        return *this;
    }
    /**
     * @brief Overload of `InlinedVector::operator=(...)` that moves the elements of `other` into the inlined vector.
     * NOTE: as a result of calling this overload, `other` is left in a valid but unspecified state.
     * @param other
     * @return
     */
    InlinedVector &operator=(InlinedVector &&other)
    {
        Base::operator=(std::forward<InlinedVector &&>(other));
        return *this;
    }

    CXXKIT_CXX20_CONSTEXPR reference operator[](size_type i)
    {
        CXXKIT_CHECK(i < this->size());
        return this->data()[i];
    }

    CXXKIT_CXX20_CONSTEXPR const_reference operator[](size_type i) const
    {
        CXXKIT_CHECK(i < this->size());
        return this->data()[i];
    }

    reference at(size_type i)
    {
        if (i >= this->size())
        {
            CXXKIT_THROW_STD_OUT_OF_RANGE("`InlinedVector::at(i:%" CXXKIT_SIZE_FORMAT ", size:%" CXXKIT_SIZE_FORMAT
                                        ")` failed bounds check",
                                        i,
                                        this->size());
        }
        return this->data()[i];
    }
    const_reference at(size_type i) const
    {
        if (i >= this->size())
        {
            CXXKIT_THROW_STD_OUT_OF_RANGE("`InlinedVector::at(i:%" CXXKIT_SIZE_FORMAT ", size:%" CXXKIT_SIZE_FORMAT
                                        ")` failed bounds check",
                                        i,
                                        this->size());
        }
        CXXKIT_CHECK(i < this->size());
        return this->data()[i];
    }

    void shrink_to_fit()
    {
        if (this->capacity() > N)
        {
            Base::shrink_to_fit();
            this->reserve(N);
        }
    }
};
CXXKIT_END_NAMESPACE

#endif // _CXXKIT_INLINED_VECTOR_HPP
