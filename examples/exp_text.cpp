/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2026~Present ChengXueWen.
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

// exp_text: string toolkit — StringBuilder / str_split / base64 / crc32 walkthrough.

#include <iostream>
#include <string>
#include <vector>

#include <cxxkit/text/string_builder.hpp>
#include <cxxkit/text/str_split.hpp>
#include <cxxkit/text/base64.hpp>
#include <cxxkit/text/crc32.hpp>
#include <cxxkit/text/string_utils.hpp>

using namespace cxxkit;

namespace
{
void print_tokens(const char *label, const std::vector<StringView> &tokens)
{
    std::cout << label << " [";
    for (size_t i = 0; i < tokens.size(); ++i)
    {
        if (i != 0)
        {
            std::cout << ", ";
        }
        std::cout << '"' << tokens[i] << '"';
    }
    std::cout << "]" << std::endl;
}
} // namespace

int main()
{
    // 1. StringBuilder: chained << assembly, release() moves the string out.
    StringBuilder sb;
    sb << "user=" << 42 << " score=" << 9.5;
    std::cout << "1) StringBuilder: " << sb.release() << std::endl;

    // 2. str_split: char / by_any_of / by_string delimiters; skip_empty drops empties.
    const std::string source = "a,,b c-d";
    print_tokens("2a) char ',' allow_empty:", str_split(source, ','));
    print_tokens("2b) char ',' skip_empty:  ", str_split(source, ',', skip_empty()));
    print_tokens("2c) by_any_of(\" ,-\") skip_empty:", str_split(source, by_any_of(" ,-"), skip_empty()));
    print_tokens("2d) by_string(\", \"):", str_split(source, by_string(", ")));

    // 3. Base64 round-trip.
    const std::string plain = "hello cxxkit";
    const std::string encoded = Base64::encode(plain);
    const std::string decoded = Base64::decode(encoded, Base64::DO_STRICT);
    std::cout << "3) base64: " << plain << " -> " << encoded << " -> " << decoded
              << (plain == decoded ? " (round-trip ok)" : " (MISMATCH)") << std::endl;

    // 4. crc32: one-shot vs incremental chaining must match ("123456789" -> 0xCBF43926).
    const std::string check = "123456789";
    const uint32_t one_shot = crc32(check);
    const uint32_t chained = crc32(check.substr(5), crc32(check.substr(0, 5)));
    std::cout << "4) crc32(\"123456789\") = 0x" << std::hex << one_shot << std::dec
              << (one_shot == 0xCBF43926u ? " (expected value)" : " UNEXPECTED")
              << "; incremental == one-shot: " << (one_shot == chained ? "yes" : "no") << std::endl;

    return 0;
}
