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

// exp_memory: intrusive ref-counting, aligned allocation, secure zeroization.
// SharedMemory is an abstract base for DesktopCapturer buffer sharing (no platform
// allocation logic of its own) — no single-process demo, skipped by design.

#include <iostream>

#include <cxxkit/memory/aligned_malloc.hpp>
#include <cxxkit/memory/ref_counted_object.hpp>
#include <cxxkit/memory/zero_memory.hpp>

using namespace cxxkit;

namespace
{

class Payload : public RefCountInterface
{
public:
    explicit Payload(int value)
        : mValue(value)
    {
    }
    int value() const { return mValue; }

private:
    int mValue;
};

} // namespace

int main()
{
    // [1] SharedRefPtr + RefCountInterface: intrusive reference counting.
    {
        SharedRefPtr<Payload> a(new RefCountedObject<Payload>(42)); // count: 0 -> 1
        SharedRefPtr<Payload> b = a;                                // count: 1 -> 2
        const RefCountedObject<Payload> *raw = static_cast<const RefCountedObject<Payload> *>(a.get());
        std::cout << "[1] ref-counted object value=" << a->value()
                  << ", has_one_ref=" << (raw->has_one_ref() ? "true" : "false") << "\n";
        b = SharedRefPtr<Payload>(); // count: 2 -> 1
        std::cout << "[1] after copy released, has_one_ref=" << (raw->has_one_ref() ? "true" : "false") << "\n";
    } // count: 1 -> 0, Payload destroyed

    // [2] aligned_malloc/aligned_free: 32-byte aligned allocation.
    {
        void *p = utils::aligned_malloc(100, 32);
        const bool is_aligned = (reinterpret_cast<uintptr_t>(p) % 32) == 0;
        std::cout << "[2] aligned_malloc(100, 32) -> " << p << ", aligned=" << (is_aligned ? "true" : "false") << "\n";
        utils::aligned_free(p);
    }

    // [3] explicit_zero_memory: secure zeroization (compiler cannot elide it).
    {
        char secret[16] = "s3cret-data!!!!";
        explicit_zero_memory(secret, sizeof(secret));
        std::cout << "[3] after explicit_zero_memory, byte[0..3] = " << static_cast<int>(secret[0]) << " "
                  << static_cast<int>(secret[1]) << " " << static_cast<int>(secret[2]) << " "
                  << static_cast<int>(secret[3]) << "\n";
    }
    return 0;
}
