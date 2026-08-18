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

#include "cxxkit/numerics/safe_conversions.hpp"
#include "cxxkit/tools/logging.hpp"
#include "cxxkit/tools/random.hpp"
#include "cxxkit/tools/checks.hpp"
#include "cxxkit/thread/mutex.hpp"

#include <cstdint>
#include <utility>
#include <limits>
#include <memory>
#include <random>
#include <cmath>

CXXKIT_BEGIN_NAMESPACE

Random::Random(uint64_t seed)
{
    CXXKIT_DCHECK(seed != 0x0ull);
    state_ = seed;
}

uint32_t Random::Rand(uint32_t t)
{
    // Casting the output to 32 bits will give an almost uniform number.
    // Pr[x=0] = (2^32-1) / (2^64-1)
    // Pr[x=k] = 2^32 / (2^64-1) for k!=0
    // Uniform would be Pr[x=k] = 2^32 / 2^64 for all 32-bit integers k.
    uint32_t x = NextOutput();
    // If x / 2^32 is uniform on [0,1), then x / 2^32 * (t+1) is uniform on
    // the interval [0,t+1), so the integer part is uniform on [0,t].
    uint64_t result = x * (static_cast<uint64_t>(t) + 1);
    result >>= 32;
    return result;
}

uint32_t Random::Rand(uint32_t low, uint32_t high)
{
    CXXKIT_DCHECK(low <= high);
    return Rand(high - low) + low;
}

int32_t Random::Rand(int32_t low, int32_t high)
{
    CXXKIT_DCHECK(low <= high);
    const int64_t low_i64{low};
    return utils::dchecked_cast<int32_t>(Rand(utils::dchecked_cast<uint32_t>(high - low_i64)) + low_i64);
}

template <>
float Random::Rand<float>()
{
    double result = NextOutput() - 1;
    result = result / static_cast<double>(0xFFFFFFFFFFFFFFFFull);
    return static_cast<float>(result);
}

template <>
double Random::Rand<double>()
{
    double result = NextOutput() - 1;
    result = result / static_cast<double>(0xFFFFFFFFFFFFFFFFull);
    return result;
}

template <>
bool Random::Rand<bool>()
{
    return Rand(0, 1) == 1;
}

double Random::Gaussian(double mean, double standard_deviation)
{
    // Creating a Normal distribution variable from two independent uniform
    // variables based on the Box-Muller transform, which is defined on the
    // interval (0, 1]. Note that we rely on NextOutput to generate integers
    // in the range [1, 2^64-1]. Normally this behavior is a bit frustrating,
    // but here it is exactly what we need.
    const double kPi = 3.14159265358979323846;
    double u1 = static_cast<double>(NextOutput()) / static_cast<double>(0xFFFFFFFFFFFFFFFFull);
    double u2 = static_cast<double>(NextOutput()) / static_cast<double>(0xFFFFFFFFFFFFFFFFull);
    return mean + standard_deviation * sqrt(-2 * log(u1)) * cos(2 * kPi * u2);
}

double Random::Exponential(double lambda)
{
    double uniform = Rand<double>();
    return -log(uniform) / lambda;
}


namespace
{
namespace detail
{
int stdSecureRandom(uint8_t *buf, size_t len)
{
    std::random_device rd;
    //TODO:del
    //    std::uniform_int_distribution<uint8_t> dist(0, 255);
    //    for (size_t i = 0; i < len; ++i)
    //    {
    //        buf[i] = dist(rd);
    //    }
    return len;
}
} // namespace detail

// The OpenSSL/std RNG.
class SecureRandomGenerator : public RandomGenerator
{
public:
    SecureRandomGenerator() { }
    ~SecureRandomGenerator() override { }
    bool Init(const void * /* seed */, size_t /* len */) override { return true; }
    bool Generate(void *buf, size_t len) override
    {
        // return (RAND_bytes(reinterpret_cast<unsigned char *>(buf), len) > 0);
        return (detail::stdSecureRandom(reinterpret_cast<unsigned char *>(buf), len) > 0);
    }
};

// A test random generator, for predictable output.
class TestRandomGenerator : public RandomGenerator
{
public:
    TestRandomGenerator()
        : seed_(7)
    {
    }
    ~TestRandomGenerator() override { }
    bool Init(const void * /* seed */, size_t /* len */) override { return true; }
    bool Generate(void *buf, size_t len) override
    {
        for (size_t i = 0; i < len; ++i)
        {
            static_cast<uint8_t *>(buf)[i] = static_cast<uint8_t>(GetRandom());
        }
        return true;
    }

private:
    int GetRandom() { return ((seed_ = seed_ * 214013L + 2531011L) >> 16) & 0x7fff; }
    int seed_;
};

// TODO: Use Base64::Base64Table instead.
static const char kBase64[64] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

static const char kHex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

static const char kUuidDigit17[4] = {'8', '9', 'a', 'b'};

// Lock for the global random generator, only needed to serialize changing the generator.
Mutex &GetRandomGeneratorLock()
{
    static Mutex &mutex = *new Mutex();
    return mutex;
}

// This round about way of creating a global RNG is to safe-guard against
// indeterminant static initialization order.
std::unique_ptr<RandomGenerator> &GetGlobalRng()
{
    static std::unique_ptr<RandomGenerator> &rng = *new std::unique_ptr<RandomGenerator>(new SecureRandomGenerator());
    return rng;
}

RandomGenerator &Rng()
{
    return *GetGlobalRng();
}
} // namespace

namespace utils
{
void SetDefaultRandomGenerator()
{
    Mutex::UniqueLock locker(GetRandomGeneratorLock());
    GetGlobalRng().reset(new SecureRandomGenerator());
}

void SetRandomGenerator(std::unique_ptr<RandomGenerator> generator)
{
    Mutex::UniqueLock locker(GetRandomGeneratorLock());
    GetGlobalRng() = std::move(generator);
}

void SetRandomTestMode(bool test)
{
    Mutex::UniqueLock locker(GetRandomGeneratorLock());
    if (!test)
    {
        GetGlobalRng().reset(new SecureRandomGenerator());
    }
    else
    {
        GetGlobalRng().reset(new TestRandomGenerator());
    }
}

bool InitRandom(int seed)
{
    return InitRandom(reinterpret_cast<const char *>(&seed), sizeof(seed));
}

bool InitRandom(const char *seed, size_t len)
{
    if (!Rng().Init(seed, len))
    {
        CXXKIT_ERROR() << "Failed to init random generator!";
        return false;
    }
    return true;
}

std::string CreateRandomString(size_t len)
{
    std::string str;
    CXXKIT_CHECK(CreateRandomString(len, &str));
    return str;
}

static bool CreateRandomString(size_t len, const char *table, int table_size, std::string *str)
{
    str->clear();
    // Avoid biased modulo division below.
    if (256 % table_size)
    {
        CXXKIT_ERROR() << "Table size must divide 256 evenly!";
        return false;
    }
    std::unique_ptr<uint8_t[]> bytes(new uint8_t[len]);
    if (!Rng().Generate(bytes.get(), len))
    {
        CXXKIT_ERROR() << "Failed to generate random string!";
        return false;
    }
    str->reserve(len);
    for (size_t i = 0; i < len; ++i)
    {
        str->push_back(table[bytes[i] % table_size]);
    }
    return true;
}

bool CreateRandomString(size_t len, std::string *str)
{
    return CreateRandomString(len, kBase64, 64, str);
}

bool CreateRandomString(size_t len, StringView table, std::string *str)
{
    return CreateRandomString(len, table.data(), static_cast<int>(table.size()), str);
}

bool CreateRandomData(size_t length, std::string *data)
{
    data->resize(length);
    // std::string is guaranteed to use contiguous memory in c++11 so we can
    // safely write directly to it.
    return Rng().Generate(&data->at(0), length);
}

// Version 4 UUID is of the form:
// xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
// Where 'x' is a hex digit, and 'y' is 8, 9, a or b.
std::string CreateRandomUuid()
{
    std::string str;
    std::unique_ptr<uint8_t[]> bytes(new uint8_t[31]);
    CXXKIT_CHECK(Rng().Generate(bytes.get(), 31));
    str.reserve(36);
    for (size_t i = 0; i < 8; ++i)
    {
        str.push_back(kHex[bytes[i] % 16]);
    }
    str.push_back('-');
    for (size_t i = 8; i < 12; ++i)
    {
        str.push_back(kHex[bytes[i] % 16]);
    }
    str.push_back('-');
    str.push_back('4');
    for (size_t i = 12; i < 15; ++i)
    {
        str.push_back(kHex[bytes[i] % 16]);
    }
    str.push_back('-');
    str.push_back(kUuidDigit17[bytes[15] % 4]);
    for (size_t i = 16; i < 19; ++i)
    {
        str.push_back(kHex[bytes[i] % 16]);
    }
    str.push_back('-');
    for (size_t i = 19; i < 31; ++i)
    {
        str.push_back(kHex[bytes[i] % 16]);
    }
    return str;
}

uint32_t CreateRandomId()
{
    uint32_t id;
    CXXKIT_CHECK(Rng().Generate(&id, sizeof(id)));
    return id;
}

uint64_t CreateRandomId64()
{
    return static_cast<uint64_t>(CreateRandomId()) << 32 | CreateRandomId();
}

uint32_t CreateRandomNonZeroId()
{
    uint32_t id;
    do
    {
        id = CreateRandomId();
    } while (id == 0);
    return id;
}

double CreateRandomDouble()
{
    return CreateRandomId() / (std::numeric_limits<uint32_t>::max() + std::numeric_limits<double>::epsilon());
}
} // namespace utils

CXXKIT_END_NAMESPACE
