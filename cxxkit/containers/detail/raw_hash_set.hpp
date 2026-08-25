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

inline bool IsEmpty(Ctrl c)    { return c == kEmpty; }
inline bool IsDeleted(Ctrl c)  { return c == kDeleted; }
inline bool IsFull(Ctrl c)     { return c >= 0; }          // 0..127 = occupied
inline bool IsSentinel(Ctrl c) { return c == kSentinel; }
inline bool IsEmptyOrDeleted(Ctrl c) { return c < kSentinel; }  // < -1

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
        : pos_(H1(hash) & mask), mask_(mask), stride_(0) {}

    size_t pos() const { return pos_; }

    void next() {
        stride_ += kWidth;
        pos_ = (pos_ + stride_) & mask_;
    }

private:
    size_t pos_;
    size_t mask_;
    size_t stride_;
};

// ---------------------------------------------------------------------------
// Capacity / growth helpers
// ---------------------------------------------------------------------------
static constexpr size_t kMinCapacity = 16;

// Growth threshold: 7/8 for capacity >= 16, 1 (full) for small tables
inline size_t GrowthThreshold(size_t capacity) {
    return capacity - capacity / 8;   // capacity * 7/8
}

inline size_t NextCapacity(size_t capacity) {
    return capacity == 0 ? kMinCapacity : capacity * 2;
}

inline bool IsValidCapacity(size_t n) {
    return n == 0 || ((n & (n - 1)) == 0);   // power of 2
}

// ---------------------------------------------------------------------------
// Slot + ctrl layout helpers
// ---------------------------------------------------------------------------
inline Ctrl*  SlotToCtrl(void* slot_array) {
    return static_cast<Ctrl*>(slot_array);
}
inline void* CtrlToSlot(Ctrl* ctrl, size_t capacity) {
    // ctrl array occupies capacity + 1 + (kWidth - 1) bytes, aligned to slot alignment
    return ctrl + capacity + 1 + (kWidth - 1);
}

inline size_t AllocSize(size_t capacity, size_t slot_size) {
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

        iterator() : set_(nullptr), ctrl_(nullptr) {}

        reference operator*()  const { return set_->slot_at_mut(ctrl_); }
        pointer   operator->() const { return &set_->slot_at_mut(ctrl_); }

        iterator& operator++() {
            ++ctrl_;
            skip_empty_or_deleted();
            return *this;
        }
        iterator operator++(int) { auto tmp = *this; ++*this; return tmp; }

        bool operator==(const iterator& o) const { return ctrl_ == o.ctrl_; }
        bool operator!=(const iterator& o) const { return ctrl_ != o.ctrl_; }

    private:
        iterator(raw_hash_set* set, Ctrl* ctrl)
            : set_(set), ctrl_(ctrl) { skip_empty_or_deleted(); }

        void skip_empty_or_deleted() {
            while (IsEmptyOrDeleted(*ctrl_)) ++ctrl_;
        }

        raw_hash_set* set_;
        Ctrl* ctrl_;
    };

    class const_iterator {
        friend class raw_hash_set;
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = const raw_hash_set::value_type;
        using difference_type   = ptrdiff_t;
        using pointer           = const value_type*;
        using reference         = const value_type&;

        const_iterator() : set_(nullptr), ctrl_(nullptr) {}
        // implicit conversion from iterator
        const_iterator(iterator it) : set_(it.set_), ctrl_(it.ctrl_) {}

        reference operator*()  const { return set_->slot_at(ctrl_); }
        pointer   operator->() const { return &set_->slot_at(ctrl_); }

        const_iterator& operator++() {
            ++ctrl_;
            skip_empty_or_deleted();
            return *this;
        }
        const_iterator operator++(int) { auto tmp = *this; ++*this; return tmp; }

        bool operator==(const const_iterator& o) const { return ctrl_ == o.ctrl_; }
        bool operator!=(const const_iterator& o) const { return ctrl_ != o.ctrl_; }

    private:
        const_iterator(const raw_hash_set* set, Ctrl* ctrl)
            : set_(set), ctrl_(ctrl) { skip_empty_or_deleted(); }

        void skip_empty_or_deleted() {
            while (IsEmptyOrDeleted(*ctrl_)) ++ctrl_;
        }

        const raw_hash_set* set_;
        Ctrl* ctrl_;
    };

    // --- constructors / destructor --------------------------------------
    raw_hash_set() : ctrl_(nullptr), slots_(nullptr), capacity_(0), size_(0) {}

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
        if (o.capacity_ > 0) {
            resize(o.capacity_);
            for (auto it = o.begin(); it != o.end(); ++it) {
                unchecked_insert(*it);
            }
        }
    }

    raw_hash_set& operator=(const raw_hash_set& o) {
        if (this == &o) return *this;
        clear();
        if (o.capacity_ > 0) {
            if (capacity_ < o.capacity_) resize(o.capacity_);
            for (auto it = o.begin(); it != o.end(); ++it) {
                unchecked_insert(*it);
            }
        }
        return *this;
    }

    // move
    raw_hash_set(raw_hash_set&& o) noexcept
        : ctrl_(o.ctrl_), slots_(o.slots_),
          capacity_(o.capacity_), size_(o.size_) {
        o.ctrl_ = nullptr; o.slots_ = nullptr;
        o.capacity_ = 0; o.size_ = 0;
    }

    raw_hash_set& operator=(raw_hash_set&& o) noexcept {
        if (this == &o) return *this;
        destroy_all();
        ctrl_ = o.ctrl_; slots_ = o.slots_;
        capacity_ = o.capacity_; size_ = o.size_;
        o.ctrl_ = nullptr; o.slots_ = nullptr;
        o.capacity_ = 0; o.size_ = 0;
        return *this;
    }

    // --- capacity / size ------------------------------------------------
    bool      empty()     const { return size_ == 0; }
    size_type size()      const { return size_; }
    size_type capacity()  const { return capacity_; }

    // --- iterators ------------------------------------------------------
    iterator       begin()        { return iterator(this, ctrl_); }
    const_iterator begin()  const { return const_iterator(this, ctrl_); }
    const_iterator cbegin() const { return const_iterator(this, ctrl_); }

    iterator       end()          { return iterator(this, sentinel()); }
    const_iterator end()    const { return const_iterator(this, sentinel()); }
    const_iterator cend()   const { return const_iterator(this, sentinel()); }

    // --- lookup ---------------------------------------------------------
    template <class K>
    iterator find(const K& key) {
        if (capacity_ == 0) return end();
        size_t h = Hash{}(key);
        ProbeSeq seq(h, capacity_ - 1);
        while (true) {
            Ctrl* g = ctrl_ + seq.pos();
            // check group (scalar: 1 byte at a time)
            if (IsFull(*g) && H2(h) == *g && Eq{}(key, Policy::key(slot_at(g)))) {
                return iterator(this, g);
            }
            if (IsEmpty(*g)) return end();
            seq.next();
        }
    }

    template <class K>
    const_iterator find(const K& key) const {
        if (capacity_ == 0) return end();
        size_t h = Hash{}(key);
        ProbeSeq seq(h, capacity_ - 1);
        while (true) {
            const Ctrl* g = ctrl_ + seq.pos();
            if (IsFull(*g) && H2(h) == *g && Eq{}(key, Policy::key(slot_at(g)))) {
                return const_iterator(this, const_cast<Ctrl*>(g));
            }
            if (IsEmpty(*g)) return end();
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
        erase_at(it.ctrl_);
        return 1;
    }

    void erase(iterator it) { erase_at(it.ctrl_); }

    // --- clear ----------------------------------------------------------
    void clear() {
        if (capacity_ == 0) return;
        for (size_t i = 0; i < capacity_; ++i) {
            if (IsFull(ctrl_[i])) {
                destroy_slot(reinterpret_cast<value_type*>(slots_) + i);
                ctrl_[i] = kEmpty;
            }
        }
        // clear clones + sentinel
        initialize_ctrl();
        size_ = 0;
    }

    // --- reserve --------------------------------------------------------
    void reserve(size_type n) {
        if (n > GrowthThreshold(capacity_)) {
            size_t cap = capacity_ == 0 ? kMinCapacity : capacity_;
            while (GrowthThreshold(cap) < n) cap *= 2;
            resize(cap);
        }
    }

    // --- swap -----------------------------------------------------------
    void swap(raw_hash_set& o) noexcept {
        using std::swap;
        swap(ctrl_, o.ctrl_);
        swap(slots_, o.slots_);
        swap(capacity_, o.capacity_);
        swap(size_, o.size_);
    }

private:
    // --- memory layout --------------------------------------------------
    // ctrl[capacity] + sentinel(1) + clones[kWidth-1] + slots[capacity]
    Ctrl*   ctrl_;
    void*   slots_;      // raw pointer to slot storage
    size_t  capacity_;
    size_t  size_;

    Ctrl* sentinel() const { return ctrl_ + capacity_; }

    void initialize_ctrl() {
        if (capacity_ == 0) return;
        std::memset(ctrl_, kEmpty, capacity_ * sizeof(Ctrl));
        ctrl_[capacity_] = kSentinel;
        // clones (for SIMD boundary; scalar mode: just copy first kWidth-1)
        for (size_t i = 0; i < kWidth - 1; ++i) {
            ctrl_[capacity_ + 1 + i] = ctrl_[i];
        }
    }

    void resize(size_t new_cap) {
        assert(IsValidCapacity(new_cap));
        Ctrl*  old_ctrl = ctrl_;
        void*  old_slots = slots_;
        size_t old_cap = capacity_;

        // allocate new backing array (ctrl + slots as one block)
        size_t ctrl_bytes = (new_cap + 1 + (kWidth - 1)) * sizeof(Ctrl);
        size_t ctrl_aligned = (ctrl_bytes + alignof(void*) - 1) & ~(alignof(void*) - 1);
        size_t total = ctrl_aligned + new_cap * sizeof(value_type);
        void* block = ::operator new(total);

        ctrl_ = static_cast<Ctrl*>(block);
        slots_ = static_cast<char*>(block) + ctrl_aligned;
        capacity_ = new_cap;
        size_ = 0;
        initialize_ctrl();

        // re-insert old elements
        if (old_cap > 0) {
            for (size_t i = 0; i < old_cap; ++i) {
                if (IsFull(old_ctrl[i])) {
                    unchecked_insert(std::move(reinterpret_cast<value_type*>(static_cast<char*>(old_slots))[i]));
                    destroy_slot(reinterpret_cast<value_type*>(static_cast<char*>(old_slots)) + i);
                }
            }
            ::operator delete(old_ctrl);
        }
    }

    // --- slot access ----------------------------------------------------
    value_type& slot_at(Ctrl* c) {
        size_t idx = static_cast<size_t>(c - ctrl_);
        return reinterpret_cast<value_type*>(slots_)[idx];
    }
    const value_type& slot_at(const Ctrl* c) const {
        size_t idx = static_cast<size_t>(c - ctrl_);
        return reinterpret_cast<const value_type*>(slots_)[idx];
    }
    value_type& slot_at_mut(Ctrl* c) {
        size_t idx = static_cast<size_t>(c - ctrl_);
        return reinterpret_cast<value_type*>(slots_)[idx];
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
        if (capacity_ == 0) reserve(kMinCapacity);
        auto res = find_or_prepare_insert(Policy::key(v));
        if (res.second) {
            construct_slot(reinterpret_cast<value_type*>(slots_) + res.first, std::forward<V>(v));
            ++size_;
        }
        return {iterator(this, ctrl_ + res.first), res.second};
    }

    // Returns {index, true} if key is new (slot prepared), {index, false} if key exists.
    std::pair<size_t, bool> find_or_prepare_insert(const key_type& key) {
        size_t h = Hash{}(key);
        ProbeSeq seq(h, capacity_ - 1);

        size_t first_deleted = capacity_;  // sentinel: no deleted slot found yet
        while (true) {
            size_t pos = seq.pos();
            Ctrl c = ctrl_[pos];
            if (IsFull(c)) {
                if (c == H2(h) && Eq{}(key, Policy::key(reinterpret_cast<value_type*>(slots_)[pos]))) {
                    return {pos, false};  // key already present
                }
            } else {  // empty or deleted
                if (IsEmpty(c)) {
                    // use first deleted slot if any, otherwise this empty slot
                    size_t target = first_deleted != capacity_ ? first_deleted : pos;
                    if (size_ + 1 > GrowthThreshold(capacity_)) {
                        resize(NextCapacity(capacity_));
                        return find_or_prepare_insert(key);  // re-probe after resize
                    }
                    ctrl_[target] = H2(h);
                    return {target, true};
                }
                if (first_deleted == capacity_) first_deleted = pos;
            }
            seq.next();
        }
    }

    void unchecked_insert(const value_type& v) {
        auto res = find_or_prepare_insert(Policy::key(v));
        assert(res.second);
        construct_slot(reinterpret_cast<value_type*>(slots_) + res.first, v);
        ++size_;
    }

    void unchecked_insert(value_type&& v) {
        auto res = find_or_prepare_insert(Policy::key(v));
        assert(res.second);
        construct_slot(reinterpret_cast<value_type*>(slots_) + res.first, std::move(v));
        ++size_;
    }

    void erase_at(Ctrl* c) {
        size_t idx = static_cast<size_t>(c - ctrl_);
        destroy_slot(reinterpret_cast<value_type*>(slots_) + idx);
        // try to convert to empty (if group has no full slots after this, it was never full)
        size_t next = (idx + 1) & (capacity_ - 1);
        if (IsEmpty(ctrl_[next])) {
            ctrl_[idx] = kEmpty;
        } else {
            ctrl_[idx] = kDeleted;
        }
        --size_;
    }

    void destroy_all() {
        if (capacity_ == 0) return;
        for (size_t i = 0; i < capacity_; ++i) {
            if (IsFull(ctrl_[i])) {
                destroy_slot(reinterpret_cast<value_type*>(slots_) + i);
            }
        }
        ::operator delete(ctrl_);
        ctrl_ = nullptr; slots_ = nullptr;
        capacity_ = 0; size_ = 0;
    }
};

}  // namespace detail
}  // namespace cxxkit
