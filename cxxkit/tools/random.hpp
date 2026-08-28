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

#include <cxxkit/tools/tools_global.hpp>

#include <cxxkit/tools/checks.hpp>

#include <limits>

CXXKIT_BEGIN_NAMESPACE

class CXXKIT_TOOLS_API Random
{
public:
    // TODO(tommi): Change this so that the seed can be initialized internally,
    // e.g. by offering two ways of constructing or offer a static method that
    // returns a seed that's suitable for initialization.
    // The problem now is that callers are calling mClock->time_in_microseconds()
    // which calls TickTime::Now().Ticks(), which can return a very low value on
    // Mac and can result in a seed of 0 after conversion to microseconds.
    // Besides the quality of the random seed being poor, this also requires
    // the client to take on extra dependencies to generate a seed.
    // If we go for a static seed generator in Random, we can use something from
    // webrtc/rtc_base and make sure that it works the same way across platforms.
    // See also discussion here: https://codereview.webrtc.org/1623543002/
    explicit Random(uint64_t seed);

    Random() = delete;
    Random(const Random &) = delete;
    Random &operator=(const Random &) = delete;

    // Return pseudo-random integer of the specified type.
    // We need to limit the size to 32 bits to keep the output close to uniform.
    template <typename T>
    T rand()
    {
        static_assert(std::numeric_limits<T>::is_integer && std::numeric_limits<T>::radix == 2 &&
                          std::numeric_limits<T>::digits <= 32,
                      "rand is only supported for built-in integer types that are "
                      "32 bits or smaller.");
        return static_cast<T>(next_output());
    }

    // Uniformly distributed pseudo-random number in the interval [0, t].
    uint32_t rand(uint32_t t);

    // Uniformly distributed pseudo-random number in the interval [low, high].
    uint32_t rand(uint32_t low, uint32_t high);

    // Uniformly distributed pseudo-random number in the interval [low, high].
    int32_t rand(int32_t low, int32_t high);

    // Normal Distribution.
    double gaussian(double mean, double standard_deviation);

    // exponential Distribution.
    double exponential(double lambda);

private:
    // Outputs a nonzero 64-bit random number using Xorshift algorithm.
    // https://en.wikipedia.org/wiki/Xorshift
    uint64_t next_output()
    {
        mState ^= mState >> 12;
        mState ^= mState << 25;
        mState ^= mState >> 27;
        CXXKIT_DCHECK(mState != 0x0ULL);
        return mState * 2685821657736338717ull;
    }

    uint64_t mState;
};

// Return pseudo-random number in the interval [0.0, 1.0).
template <>
float Random::rand<float>();

// Return pseudo-random number in the interval [0.0, 1.0).
template <>
double Random::rand<double>();

// Return pseudo-random boolean value.
template <>
bool Random::rand<bool>();


// Interface for RNG implementations.
class RandomGenerator
{
public:
    virtual ~RandomGenerator() { }
    virtual bool init(const void *seed, size_t len) = 0;
    virtual bool generate(void *buf, size_t len) = 0;
};

namespace utils
{
// Sets the default random generator as the source of randomness. The default
// source uses the OpenSSL RNG and provides cryptographically secure randomness.
void set_default_random_generator();

// set a custom random generator. Results produced by CreateRandomXyz
// are cryptographically random iff the output of the supplied generator is
// cryptographically random.
void set_random_generator(std::unique_ptr<RandomGenerator> generator);

// For testing, we can return predictable data.
void set_random_test_mode(bool test);

// Initializes the RNG, and seeds it with the specified entropy.
bool init_random(int seed);
bool init_random(const char *seed, size_t len);

// Generates a (cryptographically) random string of the given length.
// We generate base64 values so that they will be printable.
CXXKIT_TOOLS_API std::string create_random_string(size_t length);

// Generates a (cryptographically) random string of the given length.
// We generate base64 values so that they will be printable.
// Return false if the random number generator failed.
CXXKIT_TOOLS_API bool create_random_string(size_t length, std::string *str);

// Generates a (cryptographically) random string of the given length,
// with characters from the given table. Return false if the random
// number generator failed.
// For ease of implementation, the function requires that the table
// size evenly divide 256; otherwise, it returns false.
CXXKIT_TOOLS_API bool create_random_string(size_t length, StringView table, std::string *str);

// Generates (cryptographically) random data of the given length.
// Return false if the random number generator failed.
bool create_random_data(size_t length, std::string *data);

// Generates a (cryptographically) random UUID version 4 string.
std::string create_random_uuid();

// Generates a random id.
uint32_t create_random_id();

// Generates a 64 bit random id.
CXXKIT_TOOLS_API uint64_t create_random_id64();

// Generates a random id > 0.
uint32_t create_random_non_zero_id();

// Generates a random double between 0.0 (inclusive) and 1.0 (exclusive).
double create_random_double();
} // namespace utils

CXXKIT_END_NAMESPACE