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

#pragma once

#include "cxxkit/text/string_view.hpp"
#include "cxxkit/tools/optional.hpp"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string>

// Implementation that can read (exclusive) or write from/to a file.

CXXKIT_BEGIN_NAMESPACE

// This class is a thin wrapper around FILE*. It's main features are that it
// owns the FILE*, calling fclose on destruction, and that on windows, file
// names passed to the open methods are always treated as utf-8, regardless of
// system code page.

// Most of the methods return only a success/fail indication. When needed, an
// optional argument |int* error| should be added to all methods, in the same
// way as for the OpenWriteOnly methods.
class FileWrapper final
{
public:
    // Opens a file, in read or write mode. Use the is_open() method on the
    // returned object to check if the open operation was successful. On failure,
    // and if `error` is non-null, the system errno value is stored at |*error|.
    // The file is closed by the destructor.
    static FileWrapper OpenReadOnly(StringView file_name_utf8);
    static FileWrapper OpenWriteOnly(StringView file_name_utf8, int *error = nullptr);

    FileWrapper() = default;

    // Takes over ownership of `file`, closing it on destruction. Calling with
    // null `file` is allowed, and results in a FileWrapper with is_open() false.
    explicit FileWrapper(FILE *file)
        : file_(file)
    {
    }
    ~FileWrapper() { Close(); }

    // Copying is not supported.
    FileWrapper(const FileWrapper &) = delete;
    FileWrapper &operator=(const FileWrapper &) = delete;

    // Support for move semantics.
    FileWrapper(FileWrapper &&);
    FileWrapper &operator=(FileWrapper &&);

    // Returns true if a file has been opened. If the file is not open, no methods
    // but is_open and Close may be called.
    bool is_open() const { return file_ != nullptr; }

    // Closes the file, and implies Flush. Returns true on success, false if
    // writing buffered data fails. On failure, the file is nevertheless closed.
    // Calling Close on an already closed file does nothing and returns success.
    bool Close();

    // Releases and returns the wrapped file without closing it. This call passes
    // the ownership of the file to the caller, and the wrapper is no longer
    // responsible for closing it. Similarly the previously wrapped file is no
    // longer available for the wrapper to use in any aspect.
    FILE *Release();

    // Write any buffered data to the underlying file. Returns true on success,
    // false on write error. Note: Flushing when closing, is not required.
    bool Flush();

    // Seeks to the beginning of file. Returns true on success, false on failure,
    // e.g., if the underlying file isn't seekable.
    bool Rewind() { return SeekTo(0); }
    // TODO(nisse): The seek functions are used only by the WavReader. If that
    // code is demoted to test code, seek functions can be deleted from this
    // utility.
    // Seek relative to current file position.
    bool SeekRelative(int64_t offset);
    // Seek to given position.
    bool SeekTo(int64_t position);

    // Returns the file size or -1 if a size could not be determined.
    // (A file size might not exists for non-seekable files or file-like
    // objects, for example /dev/tty on unix.)
    Optional<size_t> FileSize();

    // Returns number of bytes read. Short count indicates EOF or error.
    size_t Read(void *buf, size_t length);

    // If the most recent Read() returned a short count, this methods returns true
    // if the short count was due to EOF, and false it it was due to some i/o
    // error.
    bool ReadEof() const;

    // Returns true if all data was successfully written (or buffered), or false
    // if there was an error. Writing buffered data can fail later, and is
    // reported with return value from Flush or Close.
    bool Write(const void *buf, size_t length);

private:
    FILE *file_ = nullptr;
};

CXXKIT_END_NAMESPACE