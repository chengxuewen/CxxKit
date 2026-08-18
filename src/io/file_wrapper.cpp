/***********************************************************************************************************************
**
** Library: cxxkit
**
** Copyright (C) 2025~Present ChengXueWen.
** Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
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

#include "cxxkit/io/file_wrapper.hpp"
#include "cxxkit/numerics/safe_conversions.hpp"
#include "cxxkit/tools/checks.hpp"

#include <stddef.h>

#include <cerrno>
#include <cstdint>
#include <string>

#ifdef _WIN32
#    include <Windows.h>
#else
#endif

#include <utility>

CXXKIT_BEGIN_NAMESPACE

namespace
{
FILE *FileOpen(StringView file_name_utf8, bool read_only, int *error)
{
    CXXKIT_CHECK_EQ(file_name_utf8.find_first_of('\0'), StringView::npos) << "Invalid filename, containing NUL character";
    std::string file_name(file_name_utf8);
#if defined(_WIN32)
    int len = MultiByteToWideChar(CP_UTF8, 0, file_name.c_str(), -1, nullptr, 0);
    std::wstring wstr(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, file_name.c_str(), -1, &wstr[0], len);
    FILE *file = _wfopen(wstr.c_str(), read_only ? L"rb" : L"wb");
#else
    FILE *file = fopen(file_name.c_str(), read_only ? "rb" : "wb");
#endif
    if (!file && error)
    {
        *error = errno;
    }
    return file;
}

} // namespace

// static
FileWrapper FileWrapper::OpenReadOnly(StringView file_name_utf8)
{
    return FileWrapper(FileOpen(file_name_utf8, true, nullptr));
}

// static
FileWrapper FileWrapper::OpenWriteOnly(StringView file_name_utf8, int *error /*=nullptr*/)
{
    return FileWrapper(FileOpen(file_name_utf8, false, error));
}

FileWrapper::FileWrapper(FileWrapper &&other)
{
    operator=(std::move(other));
}

FileWrapper &FileWrapper::operator=(FileWrapper &&other)
{
    Close();
    file_ = other.file_;
    other.file_ = nullptr;
    return *this;
}

bool FileWrapper::SeekRelative(int64_t offset)
{
    CXXKIT_DCHECK(file_);
    return fseek(file_, utils::checked_cast<long>(offset), SEEK_CUR) == 0;
}

bool FileWrapper::SeekTo(int64_t position)
{
    CXXKIT_DCHECK(file_);
    return fseek(file_, utils::checked_cast<long>(position), SEEK_SET) == 0;
}

Optional<size_t> FileWrapper::FileSize()
{
    if (file_ == nullptr)
        return utils::nullopt;
    long original_position = ftell(file_);
    if (original_position < 0)
        return utils::nullopt;
    int seek_error = fseek(file_, 0, SEEK_END);
    if (seek_error)
        return utils::nullopt;
    long file_size = ftell(file_);
    seek_error = fseek(file_, original_position, SEEK_SET);
    if (seek_error)
        return utils::nullopt;
    return utils::checked_cast<size_t>(file_size);
}

bool FileWrapper::Flush()
{
    CXXKIT_DCHECK(file_);
    return fflush(file_) == 0;
}

size_t FileWrapper::Read(void *buf, size_t length)
{
    CXXKIT_DCHECK(file_);
    return fread(buf, 1, length, file_);
}

bool FileWrapper::ReadEof() const
{
    CXXKIT_DCHECK(file_);
    return feof(file_);
}

bool FileWrapper::Write(const void *buf, size_t length)
{
    CXXKIT_DCHECK(file_);
    return fwrite(buf, 1, length, file_) == length;
}

bool FileWrapper::Close()
{
    if (file_ == nullptr)
        return true;

    bool success = fclose(file_) == 0;
    file_ = nullptr;
    return success;
}

FILE *FileWrapper::Release()
{
    FILE *file = file_;
    file_ = nullptr;
    return file;
}

CXXKIT_END_NAMESPACE