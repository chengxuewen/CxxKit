#pragma once
// Swiss Table hash map — C++11, no SIMD, no custom allocator
// Port from abseil-cpp/flat_hash_map.h (20220623.2)

#include <cxxkit/containers/detail/raw_hash_set.hpp>
#include <functional>
#include <stdexcept>
#include <utility>

namespace cxxkit {

// Policy for flat_hash_map: value_type = pair<K,V>, key = first
template <class K, class V>
struct FlatHashMapPolicy {
    using key_type   = K;
    using mapped_type = V;
    using value_type = std::pair<K, V>;

    static const key_type& key(const value_type& v) { return v.first; }

    template <class... Args>
    static value_type construct(Args&&... args) {
        return value_type(std::forward<Args>(args)...);
    }
};

template <class K, class V,
          class Hash = std::hash<K>,
          class Eq   = std::equal_to<K>>
class flat_hash_map
    : public detail::raw_hash_set<FlatHashMapPolicy<K, V>, Hash, Eq> {
    using Base = detail::raw_hash_set<FlatHashMapPolicy<K, V>, Hash, Eq>;

public:
    using key_type    = K;
    using mapped_type = V;
    using value_type  = std::pair<K, V>;
    using Base::Base;

    // operator[] — insert default-constructed mapped_type if key missing
    V& operator[](const K& key) {
        auto res = this->emplace(key, V());
        return res.first->second;
    }

    V& operator[](K&& key) {
        auto res = this->emplace(std::move(key), V());
        return res.first->second;
    }

    // at() — bounds-checked access
    V& at(const K& key) {
        auto it = this->find(key);
        if (it == this->end()) throw std::out_of_range("flat_hash_map::at");
        return it->second;
    }
    const V& at(const K& key) const {
        auto it = this->find(key);
        if (it == this->end()) throw std::out_of_range("flat_hash_map::at");
        return it->second;
    }
};

}  // namespace cxxkit
