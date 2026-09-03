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

// exp_containers: Swiss Table maps, zero-copy views, inline and fixed storage.

#include <cxxkit/containers/array_view.hpp>
#include <cxxkit/containers/fixed_array.hpp>
#include <cxxkit/containers/flat_hash_map.hpp>
#include <cxxkit/containers/inlined_vector.hpp>

#include <iostream>
#include <string>
#include <vector>

using namespace cxxkit;

int main()
{
    // [1] flat_hash_map: std::unordered_map drop-in (insert / iterate / find / operator[] / at / count).
    flat_hash_map<std::string, int> scores;
    scores.insert(std::make_pair(std::string("alice"), 90));
    scores.insert(std::make_pair(std::string("bob"), 75));
    scores["carol"] = 88;
    for (flat_hash_map<std::string, int>::const_iterator it = scores.begin(); it != scores.end(); ++it)
    {
        std::cout << "  " << it->first << " -> " << it->second << '\n';
    }
    std::cout << "  find(bob)=" << scores.find(std::string("bob"))->second
              << " at(carol)=" << scores.at(std::string("carol"))
              << " count(dave)=" << scores.count(std::string("dave")) << std::endl;

    // [2] ArrayView: zero-copy read-only view over vector or C array.
    std::vector<int> numbers;
    numbers.push_back(10);
    numbers.push_back(20);
    numbers.push_back(30);
    int raw[3] = {4, 5, 6};
    ArrayView<const int> view = make_const_array_view(numbers.data(), numbers.size());
    ArrayView<const int> raw_view(raw);
    std::cout << "  vector view:";
    for (ArrayView<const int>::const_iterator it = view.begin(); it != view.end(); ++it)
    {
        std::cout << ' ' << *it;
    }
    std::cout << "\n  raw view: size=" << raw_view.size() << " at(2)=" << raw_view.at(2) << std::endl;

    // [3] InlinedVector: small sequences stay on the stack, no heap allocation.
    InlinedVector<int, 4> inline_nums;
    inline_nums.push_back(7);
    inline_nums.push_back(8);
    inline_nums.push_back(9);
    inline_nums.push_back(10);
    std::cout << "  inlined: size=" << inline_nums.size() << " at(0)=" << inline_nums.at(0)
              << " back=" << inline_nums.back() << std::endl;

    // [4] FixedArray: fixed-length heap array with runtime size (n = 0 is legal).
    FixedArray<int> fixed(3, 0);
    fixed[0] = 42;
    fixed.fill(fixed[0] - 40);
    FixedArray<int> empty_fixed(0);
    std::cout << "  fixed: size=" << fixed.size() << " data[1]=" << fixed[1] << " empty: size=" << empty_fixed.size()
              << " data()==nullptr " << (empty_fixed.data() == nullptr ? "true" : "false") << std::endl;
    return 0;
}
