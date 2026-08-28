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
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
** THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
** SOFTWARE.
**
***********************************************************************************************************************/

#pragma once
// Swiss Table open-addressing hash set — simplified C++11 implementation
// Inspired by abseil-cpp/raw_hash_set.h (20220623.2)
// Scalar fallback only (no SIMD), no custom allocator, no exception safety.
//
// Layout: ctrl[capacity] + sentinel + clones[kWidth-1] + slots[capacity]
// Control bytes: kEmpty(-128), kDeleted(-2), kSentinel(-1), or H2 (0..127)

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iterator>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace cxxkit {
namespace detail {

// ---------------------------------------------------------------------------
// Control byte values
// ---------------------------------------------------------------------------
enum Ctrl : int8_t {
    kEmpty    = -128,  // 0x80
    kDeleted  = -2,    // 0xFE
    kSentinel = -1,    // 0xFF
};

inline bool is_empty(Ctrl c)    { return c == kEmpty; }
inline bool is_deleted(Ctrl c)  { return c == kDeleted; }
inline bool is_full(Ctrl c)     { return c >= 0; }          // 0..127 = occupied
inline bool is_sentinel(Ctrl c) { return c == kSentinel; }
inline bool is_empty_or_deleted(Ctrl c) { return c < kSentinel; }  // < -1

// ---------------------------------------------------------------------------
// H1 / H2 hash split
// H1 = upper bits → probe seed   H2 = lower 7 bits → control byte
// ---------------------------------------------------------------------------
inline size_t H1(size_t hash) { return hash >> 7; }
inline Ctrl   H2(size_t hash) { return static_cast<Ctrl>(hash & 0x7F); }

// ---------------------------------------------------------------------------
// Group width — scalar mode: 1 byte at a time (SIMD would be 16)
// ---------------------------------------------------------------------------
static constexpr size_t kWidth = 1;

// ---------------------------------------------------------------------------
// Probe sequence — quadratic probing with triangular numbers
// ---------------------------------------------------------------------------
class ProbeSeq {
public:
    ProbeSeq(size_t hash, size_t mask)
        : mPos(H1(hash) & mask), mMask(mask), mStride(0) {}

    size_t pos() const { return mPos; }

    void next() {
        mStride += kWidth;
        mPos = (mPos + mStride) & mMask;
    }

private:
    size_t mPos;
    size_t mMask;
    size_t mStride;
};

// ---------------------------------------------------------------------------
// Capacity / growth helpers
// ---------------------------------------------------------------------------
static constexpr size_t kMinCapacity = 16;

// Growth threshold: 7/8 for capacity >= 16, 1 (full) for small tables
inline size_t growth_threshold(size_t capacity) {
    return capacity - capacity / 8;   // capacity * 7/8
}

inline size_t next_capacity(size_t capacity) {
    return capacity == 0 ? kMinCapacity : capacity * 2;
}

inline bool is_valid_capacity(size_t n) {
    return n == 0 || ((n & (n - 1)) == 0);   // power of 2
}

// ---------------------------------------------------------------------------
// Slot + ctrl layout helpers
// ---------------------------------------------------------------------------
inline Ctrl*  slot_to_ctrl(void* slot_array) {
    return static_cast<Ctrl*>(slot_array);
}
inline void* ctrl_to_slot(Ctrl* ctrl, size_t capacity) {
    // ctrl array occupies capacity + 1 + (kWidth - 1) bytes, aligned to slot alignment
    return ctrl + capacity + 1 + (kWidth - 1);
}

inline size_t alloc_size(size_t capacity, size_t slot_size) {
    // ctrl[capacity] + sentinel(1) + clones[kWidth-1] + slots[capacity]
    size_t ctrl_bytes = (capacity + 1 + (kWidth - 1)) * sizeof(Ctrl);
    // align slot start to alignof(void*)
    size_t ctrl_aligned = (ctrl_bytes + alignof(void*) - 1) & ~(alignof(void*) - 1);
    return ctrl_aligned + capacity * slot_size;
}

// ---------------------------------------------------------------------------
// Policy: maps value_type → key_type
// ---------------------------------------------------------------------------
// Expected interface:
//   Policy::key_type
//   Policy::value_type
//   Policy::key(const value_type&) → const key_type&
//   Policy::construct(Args&&...) → value_type

// ---------------------------------------------------------------------------
// raw_hash_set: core Swiss Table
// ---------------------------------------------------------------------------
template <class Policy, class Hash, class Eq>
class raw_hash_set {
public:
    using key_type    = typename Policy::key_type;
    using value_type  = typename Policy::value_type;
    using size_type   = size_t;
    using hasher      = Hash;
    using key_equal   = Eq;

    // --- iterator --------------------------------------------------------
    class iterator {
        friend class raw_hash_set;
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = raw_hash_set::value_type;
        using difference_type   = ptrdiff_t;
        using pointer           = value_type*;
        using reference         = value_type&;

        iterator() : mSet(nullptr), mCtrl(nullptr) {}

        reference operator*()  const { return mSet->slot_at_mut(mCtrl); }
        pointer   operator->() const { return &mSet->slot_at_mut(mCtrl); }

        iterator& operator++() {
            ++mCtrl;
            skip_empty_or_deleted();
            return *this;
        }
        iterator operator++(int) { auto tmp = *this; ++*this; return tmp; }

        bool operator==(const iterator& o) const { return mCtrl == o.mCtrl; }
        bool operator!=(const iterator& o) const { return mCtrl != o.mCtrl; }

    private:
        iterator(raw_hash_set* set, Ctrl* ctrl)
            : mSet(set), mCtrl(ctrl) { skip_empty_or_deleted(); }

        void skip_empty_or_deleted() {
            while (is_empty_or_deleted(*mCtrl)) ++mCtrl;
        }

        raw_hash_set* mSet;
        Ctrl* mCtrl;
    };

    class const_iterator {
        friend class raw_hash_set;
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = const raw_hash_set::value_type;
        using difference_type   = ptrdiff_t;
        using pointer           = const value_type*;
        using reference         = const value_type&;

        const_iterator() : mSet(nullptr), mCtrl(nullptr) {}
        // implicit conversion from iterator
        const_iterator(iterator it) : mSet(it.mSet), mCtrl(it.mCtrl) {}

        reference operator*()  const { return mSet->slot_at(mCtrl); }
        pointer   operator->() const { return &mSet->slot_at(mCtrl); }

        const_iterator& operator++() {
            ++mCtrl;
            skip_empty_or_deleted();
            return *this;
        }
        const_iterator operator++(int) { auto tmp = *this; ++*this; return tmp; }

        bool operator==(const const_iterator& o) const { return mCtrl == o.mCtrl; }
        bool operator!=(const const_iterator& o) const { return mCtrl != o.mCtrl; }

    private:
        const_iterator(const raw_hash_set* set, Ctrl* ctrl)
            : mSet(set), mCtrl(ctrl) { skip_empty_or_deleted(); }

        void skip_empty_or_deleted() {
            while (is_empty_or_deleted(*mCtrl)) ++mCtrl;
        }

        const raw_hash_set* mSet;
        Ctrl* mCtrl;
    };

    // --- constructors / destructor --------------------------------------
    raw_hash_set() : mCtrl(nullptr), mSlots(nullptr), mCapacity(0), mSize(0) {}

    explicit raw_hash_set(size_t bucket_count_hint) : raw_hash_set() {
        if (bucket_count_hint > 0) {
            size_t cap = kMinCapacity;
            while (cap < bucket_count_hint) cap *= 2;
            resize(cap);
        }
    }

    ~raw_hash_set() { destroy_all(); }

    // copy
    raw_hash_set(const raw_hash_set& o) : raw_hash_set() {
        if (o.mCapacity > 0) {
            resize(o.mCapacity);
            for (auto it = o.begin(); it != o.end(); ++it) {
                unchecked_insert(*it);
            }
        }
    }

    raw_hash_set& operator=(const raw_hash_set& o) {
        if (this == &o) return *this;
        clear();
        if (o.mCapacity > 0) {
            if (mCapacity < o.mCapacity) resize(o.mCapacity);
            for (auto it = o.begin(); it != o.end(); ++it) {
                unchecked_insert(*it);
            }
        }
        return *this;
    }

    // move
    raw_hash_set(raw_hash_set&& o) noexcept
        : mCtrl(o.mCtrl), mSlots(o.mSlots),
          mCapacity(o.mCapacity), mSize(o.mSize) {
        o.mCtrl = nullptr; o.mSlots = nullptr;
        o.mCapacity = 0; o.mSize = 0;
    }

    raw_hash_set& operator=(raw_hash_set&& o) noexcept {
        if (this == &o) return *this;
        destroy_all();
        mCtrl = o.mCtrl; mSlots = o.mSlots;
        mCapacity = o.mCapacity; mSize = o.mSize;
        o.mCtrl = nullptr; o.mSlots = nullptr;
        o.mCapacity = 0; o.mSize = 0;
        return *this;
    }

    // --- capacity / size ------------------------------------------------
    bool      empty()     const { return mSize == 0; }
    size_type size()      const { return mSize; }
    size_type capacity()  const { return mCapacity; }

    // --- iterators ------------------------------------------------------
    iterator       begin()        { return iterator(this, mCtrl); }
    const_iterator begin()  const { return const_iterator(this, mCtrl); }
    const_iterator cbegin() const { return const_iterator(this, mCtrl); }

    iterator       end()          { return iterator(this, sentinel()); }
    const_iterator end()    const { return const_iterator(this, sentinel()); }
    const_iterator cend()   const { return const_iterator(this, sentinel()); }

    // --- lookup ---------------------------------------------------------
    template <class K>
    iterator find(const K& key) {
        if (mCapacity == 0) return end();
        size_t h = Hash{}(key);
        ProbeSeq seq(h, mCapacity - 1);
        while (true) {
            Ctrl* g = mCtrl + seq.pos();
            // check group (scalar: 1 byte at a time)
            if (is_full(*g) && H2(h) == *g && Eq{}(key, Policy::key(slot_at(g)))) {
                return iterator(this, g);
            }
            if (is_empty(*g)) return end();
            seq.next();
        }
    }

    template <class K>
    const_iterator find(const K& key) const {
        if (mCapacity == 0) return end();
        size_t h = Hash{}(key);
        ProbeSeq seq(h, mCapacity - 1);
        while (true) {
            const Ctrl* g = mCtrl + seq.pos();
            if (is_full(*g) && H2(h) == *g && Eq{}(key, Policy::key(slot_at(g)))) {
                return const_iterator(this, const_cast<Ctrl*>(g));
            }
            if (is_empty(*g)) return end();
            seq.next();
        }
    }

    template <class K>
    size_type count(const K& key) const { return find(key) != end() ? 1 : 0; }

    template <class K>
    bool contains(const K& key) const { return find(key) != end(); }

    // --- insert ---------------------------------------------------------
    std::pair<iterator, bool> insert(const value_type& v) {
        return emplace_impl(v);
    }
    std::pair<iterator, bool> insert(value_type&& v) {
        return emplace_impl(std::move(v));
    }

    template <class... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        return emplace_impl(Policy::construct(std::forward<Args>(args)...));
    }

    // --- erase ----------------------------------------------------------
    template <class K>
    size_type erase(const K& key) {
        auto it = find(key);
        if (it == end()) return 0;
        erase_at(it.mCtrl);
        return 1;
    }

    void erase(iterator it) { erase_at(it.mCtrl); }

    // --- clear ----------------------------------------------------------
    void clear() {
        if (mCapacity == 0) return;
        for (size_t i = 0; i < mCapacity; ++i) {
            if (is_full(mCtrl[i])) {
                destroy_slot(reinterpret_cast<value_type*>(mSlots) + i);
                mCtrl[i] = kEmpty;
            }
        }
        // clear clones + sentinel
        initialize_ctrl();
        mSize = 0;
    }

    // --- reserve --------------------------------------------------------
    void reserve(size_type n) {
        if (n > growth_threshold(mCapacity)) {
            size_t cap = mCapacity == 0 ? kMinCapacity : mCapacity;
            while (growth_threshold(cap) < n) cap *= 2;
            resize(cap);
        }
    }

    // --- swap -----------------------------------------------------------
    void swap(raw_hash_set& o) noexcept {
        using std::swap;
        swap(mCtrl, o.mCtrl);
        swap(mSlots, o.mSlots);
        swap(mCapacity, o.mCapacity);
        swap(mSize, o.mSize);
    }

private:
    // --- memory layout --------------------------------------------------
    // ctrl[capacity] + sentinel(1) + clones[kWidth-1] + slots[capacity]
    Ctrl*   mCtrl;
    void*   mSlots;      // raw pointer to slot storage
    size_t  mCapacity;
    size_t  mSize;

    Ctrl* sentinel() const { return mCtrl + mCapacity; }

    void initialize_ctrl() {
        if (mCapacity == 0) return;
        std::memset(mCtrl, kEmpty, mCapacity * sizeof(Ctrl));
        mCtrl[mCapacity] = kSentinel;
        // clones (for SIMD boundary; scalar mode: just copy first kWidth-1)
        for (size_t i = 0; i < kWidth - 1; ++i) {
            mCtrl[mCapacity + 1 + i] = mCtrl[i];
        }
    }

    void resize(size_t new_cap) {
        assert(is_valid_capacity(new_cap));
        Ctrl*  old_ctrl = mCtrl;
        void*  old_slots = mSlots;
        size_t old_cap = mCapacity;

        // allocate new backing array (ctrl + slots as one block)
        size_t ctrl_bytes = (new_cap + 1 + (kWidth - 1)) * sizeof(Ctrl);
        size_t ctrl_aligned = (ctrl_bytes + alignof(void*) - 1) & ~(alignof(void*) - 1);
        size_t total = ctrl_aligned + new_cap * sizeof(value_type);
        void* block = ::operator new(total);

        mCtrl = static_cast<Ctrl*>(block);
        mSlots = static_cast<char*>(block) + ctrl_aligned;
        mCapacity = new_cap;
        mSize = 0;
        initialize_ctrl();

        // re-insert old elements
        if (old_cap > 0) {
            for (size_t i = 0; i < old_cap; ++i) {
                if (is_full(old_ctrl[i])) {
                    unchecked_insert(std::move(reinterpret_cast<value_type*>(static_cast<char*>(old_slots))[i]));
                    destroy_slot(reinterpret_cast<value_type*>(static_cast<char*>(old_slots)) + i);
                }
            }
            ::operator delete(old_ctrl);
        }
    }

    // --- slot access ----------------------------------------------------
    value_type& slot_at(Ctrl* c) {
        size_t idx = static_cast<size_t>(c - mCtrl);
        return reinterpret_cast<value_type*>(mSlots)[idx];
    }
    const value_type& slot_at(const Ctrl* c) const {
        size_t idx = static_cast<size_t>(c - mCtrl);
        return reinterpret_cast<const value_type*>(mSlots)[idx];
    }
    value_type& slot_at_mut(Ctrl* c) {
        size_t idx = static_cast<size_t>(c - mCtrl);
        return reinterpret_cast<value_type*>(mSlots)[idx];
    }

    void construct_slot(value_type* p, value_type&& v) {
        ::new (static_cast<void*>(p)) value_type(std::move(v));
    }
    void construct_slot(value_type* p, const value_type& v) {
        ::new (static_cast<void*>(p)) value_type(v);
    }
    void destroy_slot(value_type* p) { p->~value_type(); }

    // --- core operations ------------------------------------------------
    template <class V>
    std::pair<iterator, bool> emplace_impl(V&& v) {
        if (mCapacity == 0) reserve(kMinCapacity);
        auto res = find_or_prepare_insert(Policy::key(v));
        if (res.second) {
            construct_slot(reinterpret_cast<value_type*>(mSlots) + res.first, std::forward<V>(v));
            ++mSize;
        }
        return {iterator(this, mCtrl + res.first), res.second};
    }

    // Returns {index, true} if key is new (slot prepared), {index, false} if key exists.
    std::pair<size_t, bool> find_or_prepare_insert(const key_type& key) {
        size_t h = Hash{}(key);
        ProbeSeq seq(h, mCapacity - 1);

        size_t first_deleted = mCapacity;  // sentinel: no deleted slot found yet
        while (true) {
            size_t pos = seq.pos();
            Ctrl c = mCtrl[pos];
            if (is_full(c)) {
                if (c == H2(h) && Eq{}(key, Policy::key(reinterpret_cast<value_type*>(mSlots)[pos]))) {
                    return {pos, false};  // key already present
                }
            } else {  // empty or deleted
                if (is_empty(c)) {
                    // use first deleted slot if any, otherwise this empty slot
                    size_t target = first_deleted != mCapacity ? first_deleted : pos;
                    if (mSize + 1 > growth_threshold(mCapacity)) {
                        resize(next_capacity(mCapacity));
                        return find_or_prepare_insert(key);  // re-probe after resize
                    }
                    mCtrl[target] = H2(h);
                    return {target, true};
                }
                if (first_deleted == mCapacity) first_deleted = pos;
            }
            seq.next();
        }
    }

    void unchecked_insert(const value_type& v) {
        auto res = find_or_prepare_insert(Policy::key(v));
        assert(res.second);
        construct_slot(reinterpret_cast<value_type*>(mSlots) + res.first, v);
        ++mSize;
    }

    void unchecked_insert(value_type&& v) {
        auto res = find_or_prepare_insert(Policy::key(v));
        assert(res.second);
        construct_slot(reinterpret_cast<value_type*>(mSlots) + res.first, std::move(v));
        ++mSize;
    }

    void erase_at(Ctrl* c) {
        size_t idx = static_cast<size_t>(c - mCtrl);
        destroy_slot(reinterpret_cast<value_type*>(mSlots) + idx);
        // try to convert to empty (if group has no full slots after this, it was never full)
        size_t next = (idx + 1) & (mCapacity - 1);
        if (is_empty(mCtrl[next])) {
            mCtrl[idx] = kEmpty;
        } else {
            mCtrl[idx] = kDeleted;
        }
        --mSize;
    }

    void destroy_all() {
        if (mCapacity == 0) return;
        for (size_t i = 0; i < mCapacity; ++i) {
            if (is_full(mCtrl[i])) {
                destroy_slot(reinterpret_cast<value_type*>(mSlots) + i);
            }
        }
        ::operator delete(mCtrl);
        mCtrl = nullptr; mSlots = nullptr;
        mCapacity = 0; mSize = 0;
    }
};

}  // namespace detail
}  // namespace cxxkit
