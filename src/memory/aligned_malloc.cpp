/***********************************************************************************************************************
**
** Library: cxxkit
**
** Copyright (C) 2025~Present chengxuewen.
** Copyright (c) 2011 The WebRTC project authors.
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

#include <cxxkit/memory/aligned_malloc.hpp>
#include <cxxkit/tools/assert.hpp>

#include <stdlib.h>  // for free, malloc
#include <string.h>  // for memcpy

#ifdef _WIN32
#   include <windows.h>
#else
#   include <stdint.h>
#endif

// Reference on memory alignment:
// http://stackoverflow.com/questions/227897/solve-the-memory-alignment-in-c-interview-question-that-stumped-me
CXXKIT_BEGIN_NAMESPACE
namespace utils
{
uintptr_t getRightAlign(uintptr_t start_pos, size_t alignment)
{
    // The pointer should be aligned with `alignment` bytes. The - 1 guarantees
    // that it is aligned towards the closest higher (right) address.
    return (start_pos + alignment - 1) & ~(alignment - 1);
}

// Alignment must be an integer power of two.
bool validAlignment(size_t alignment)
{
    if (!alignment)
    {
        return false;
    }
    return (alignment & (alignment - 1)) == 0;
}

void *getRightAlign(const void *pointer, size_t alignment)
{
    if (!pointer)
    {
        return NULL;
    }
    if (!validAlignment(alignment))
    {
        return NULL;
    }
    uintptr_t start_pos = reinterpret_cast<uintptr_t>(pointer);
    return reinterpret_cast<void *>(getRightAlign(start_pos, alignment));
}

void *alignedMalloc(size_t size, size_t alignment)
{
    if (size == 0)
    {
        return NULL;
    }
    if (!validAlignment(alignment))
    {
        return NULL;
    }

    // The memory is aligned towards the lowest address that so only
    // alignment - 1 bytes needs to be allocated.
    // A pointer to the start of the memory must be stored so that it can be
    // retreived for deletion, ergo the sizeof(uintptr_t).
    void *memory_pointer = malloc(size + sizeof(uintptr_t) + alignment - 1);
    CXXKIT_ASSERT_X(memory_pointer, "alignedMalloc()", "Couldn't allocate memory in alignedMalloc");

    // Aligning after the sizeof(uintptr_t) bytes will leave room for the header
    // in the same memory block.
    uintptr_t align_start_pos = reinterpret_cast<uintptr_t>(memory_pointer);
    align_start_pos += sizeof(uintptr_t);
    uintptr_t aligned_pos = getRightAlign(align_start_pos, alignment);
    void *aligned_pointer = reinterpret_cast<void *>(aligned_pos);

    // Store the address to the beginning of the memory just before the aligned
    // memory.
    uintptr_t header_pos = aligned_pos - sizeof(uintptr_t);
    void *header_pointer = reinterpret_cast<void *>(header_pos);
    uintptr_t memory_start = reinterpret_cast<uintptr_t>(memory_pointer);
    memcpy(header_pointer, &memory_start, sizeof(uintptr_t));

    return aligned_pointer;
}

void alignedFree(void *mem_block)
{
    if (mem_block == NULL)
    {
        return;
    }
    uintptr_t aligned_pos = reinterpret_cast<uintptr_t>(mem_block);
    uintptr_t header_pos = aligned_pos - sizeof(uintptr_t);

    // Read out the address of the AlignedMemory struct from the header.
    uintptr_t memory_start_pos = *reinterpret_cast<uintptr_t *>(header_pos);
    void *memory_start = reinterpret_cast<void *>(memory_start_pos);
    free(memory_start);
}
} // namespace utils
CXXKIT_END_NAMESPACE
