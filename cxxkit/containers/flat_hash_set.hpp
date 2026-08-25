#pragma once
// Swiss Table hash set — C++11, no SIMD, no custom allocator
// Port from abseil-cpp/flat_hash_set.h (20220623.2)

#include <cxxkit/containers/detail/raw_hash_set.hpp>
#include <functional>

namespace cxxkit {

// Policy for flat_hash_set: value_type = K, key = identity
template <class K>
struct FlatHashSetPolicy {
    using key_type   = K;
    using value_type = K;

    static const key_type& key(const value_type& v) { return v; }

    template <class... Args>
    static value_type construct(Args&&... args) {
        return value_type(std::forward<Args>(args)...);
    }
};

template <class K,
          class Hash = std::hash<K>,
          class Eq   = std::equal_to<K>>
class flat_hash_set
    : public detail::raw_hash_set<FlatHashSetPolicy<K>, Hash, Eq> {
    using Base = detail::raw_hash_set<FlatHashSetPolicy<K>, Hash, Eq>;

public:
    using key_type   = K;
    using value_type = K;
    using Base::Base;
};

}  // namespace cxxkit
