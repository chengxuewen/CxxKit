/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present ChengXueWen.
** Copyright (c) 2015 The WebRTC project authors.
**
** License: MIT License + BSD-3 (ported from libwebrtc api/video/video_frame_buffer.h)
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

#include <cxxkit/memory/ref_count.hpp>
#include <cxxkit/memory/ref_counted_object.hpp>
#include <cxxkit/memory/shared_ref_ptr.hpp>
#include <cxxkit/media/media_global.hpp>
#include <cxxkit/media/video_types.hpp>

#include <cxxkit/memory/ref_count.hpp>
#include <cxxkit/memory/ref_counted_object.hpp>
#include <cxxkit/memory/shared_ref_ptr.hpp>
#include <cxxkit/media/media_global.hpp>
#include <cxxkit/media/video_types.hpp>
#include <cxxkit/tools/checks.hpp>

#include <cstdint>
#include <functional>
#include <string>

namespace cxxkit {

class I420BufferInterface;
class NV12BufferInterface;

// Base class for frame buffers of different types of pixel format and storage.
// The tag in type() indicates how the data is represented, and each type is
// implemented as a subclass. To access the pixel data, call the appropriate
// GetXXX() function, where XXX represents the type. There is also a function
// ToI420() that returns a frame buffer in I420 format, converting from the
// underlying representation if necessary. I420 is the most widely accepted
// format and serves as a fallback for video sinks that can only handle I420.
// Frame metadata such as rotation and timestamp are stored in
// cxxkit::VideoFrame, and not here.
class CXXKIT_MEDIA_API VideoFrameBuffer : public RefCountInterface {
public:
    // This function specifies in what pixel format the data is stored in.
    virtual VideoType type() const = 0;

    // The resolution of the frame in pixels. For formats where some planes are
    // subsampled, this is the highest-resolution plane.
    virtual int width() const = 0;
    virtual int height() const = 0;

    // Returns a memory-backed frame buffer in I420 format. If the pixel data is
    // in another format, a conversion will take place. All implementations must
    // provide a fallback to I420 for compatibility.
    // Conversion may fail, for example if reading the pixel data from a texture
    // fails. If the conversion fails, nullptr is returned.
    virtual SharedRefPtr<I420BufferInterface> ToI420() = 0;

    // GetI420() should return the I420 buffer when the conversion is trivial,
    // i.e. no change for binary data is needed. Otherwise it should return
    // nullptr. It is overridden by subclasses that can return an I420 buffer
    // without any conversion, in particular, I420BufferInterface.
    virtual const I420BufferInterface* GetI420() const;

    // A format specific scale function. First, the image is cropped to
    // `crop_width` and `crop_height` and then scaled to `scaled_width` and
    // `scaled_height`.
    // ponytail: the default I420-conversion implementation needs the concrete
    // I420Buffer (added with the I420Buffer sublib); base default returns
    // nullptr. Concrete buffers override this (e.g. I420Buffer in Task 4).
    virtual SharedRefPtr<VideoFrameBuffer> CropAndScale(int offset_x,
                                                        int offset_y,
                                                        int crop_width,
                                                        int crop_height,
                                                        int scaled_width,
                                                        int scaled_height);

    // Alias for common use case.
    SharedRefPtr<VideoFrameBuffer> Scale(int scaled_width, int scaled_height)
    {
        return CropAndScale(0, 0, width(), height(), scaled_width, scaled_height);
    }

    // These functions should only be called if type() is of the correct type.
    // Calling with a different type will result in a crash.
    const NV12BufferInterface* GetNV12() const;

    // For logging: returns a textual representation of the storage.
    virtual std::string storage_representation() const;

protected:
    ~VideoFrameBuffer() override {}
};

// This interface represents planar formats.
class PlanarYuvBuffer : public VideoFrameBuffer {
public:
    virtual int ChromaWidth() const = 0;
    virtual int ChromaHeight() const = 0;

    // Returns the number of steps (in terms of Data*() return type) between
    // successive rows for a given plane.
    virtual int StrideY() const = 0;
    virtual int StrideU() const = 0;
    virtual int StrideV() const = 0;

protected:
    ~PlanarYuvBuffer() override {}
};

// This interface represents 8-bit color depth formats: VideoType::kI420,
// VideoType::kI422 and VideoType::kI444.
class PlanarYuv8Buffer : public PlanarYuvBuffer {
public:
    // Returns pointer to the pixel data for a given plane. The memory is owned by
    // the VideoFrameBuffer object and must not be freed by the caller.
    virtual const uint8_t* GetDataY() const = 0;
    virtual const uint8_t* GetDataU() const = 0;
    virtual const uint8_t* GetDataV() const = 0;

protected:
    ~PlanarYuv8Buffer() override {}
};

class CXXKIT_MEDIA_API I420BufferInterface : public PlanarYuv8Buffer {
public:
    VideoType type() const override { return VideoType::kI420; }

    int ChromaWidth() const final { return (width() + 1) / 2; }
    int ChromaHeight() const final { return (height() + 1) / 2; }

    // Trivial conversion: the buffer is already I420.
    SharedRefPtr<I420BufferInterface> ToI420() final { return SharedRefPtr<I420BufferInterface>(this); }
    const I420BufferInterface* GetI420() const final { return this; }

protected:
    ~I420BufferInterface() override {}
};

// Represents VideoType::kNV12. NV12 is full resolution Y and half-resolution
// interleaved UV.
class BiplanarYuvBuffer : public VideoFrameBuffer {
public:
    virtual int ChromaWidth() const = 0;
    virtual int ChromaHeight() const = 0;

    // Returns the number of steps (in terms of Data*() return type) between
    // successive rows for a given plane.
    virtual int StrideY() const = 0;
    virtual int StrideUV() const = 0;

protected:
    ~BiplanarYuvBuffer() override {}
};

class BiplanarYuv8Buffer : public BiplanarYuvBuffer {
public:
    virtual const uint8_t* GetDataY() const = 0;
    virtual const uint8_t* GetDataUV() const = 0;

protected:
    ~BiplanarYuv8Buffer() override {}
};

class CXXKIT_MEDIA_API NV12BufferInterface : public BiplanarYuv8Buffer {
public:
    VideoType type() const override { return VideoType::kNV12; }

    int ChromaWidth() const final { return (width() + 1) / 2; }
    int ChromaHeight() const final { return (height() + 1) / 2; }

protected:
    ~NV12BufferInterface() override {}
};

// ---------------------------------------------------------------------------
// Inline default implementations (kept header-only).
// ---------------------------------------------------------------------------

inline const I420BufferInterface* VideoFrameBuffer::GetI420() const
{
    // Overridden by subclasses that can return an I420 buffer without any
    // conversion, in particular, I420BufferInterface.
    return nullptr;
}

inline SharedRefPtr<VideoFrameBuffer> VideoFrameBuffer::CropAndScale(int /*offset_x*/,
                                                                     int /*offset_y*/,
                                                                     int /*crop_width*/,
                                                                     int /*crop_height*/,
                                                                     int /*scaled_width*/,
                                                                     int /*scaled_height*/)
{
    // ponytail: default conversion-to-I420 impl lives with the concrete
    // I420Buffer (see i420_buffer.cpp); a bare interface cannot produce a
    // scaled destination buffer. Real buffers override this.
    return SharedRefPtr<VideoFrameBuffer>();
}

inline const NV12BufferInterface* VideoFrameBuffer::GetNV12() const
{
    // Only callable when type() is kNV12; calling with a different type results
    // in a crash (CHECK).
    CXXKIT_CHECK(type() == VideoType::kNV12);
    return static_cast<const NV12BufferInterface*>(this);
}

inline std::string VideoFrameBuffer::storage_representation() const
{
    return "?";
}

// ---------------------------------------------------------------------------
// Wrapped buffer for external memory with a release callback.
// ---------------------------------------------------------------------------

namespace detail {

// Wraps externally-owned I420 planes; calls `no_longer_used` when the last
// reference is dropped. Ref-count is provided by RefCountedObject<WrappedI420Buffer>.
class WrappedI420Buffer : public I420BufferInterface {
public:
    WrappedI420Buffer(int width,
                      int height,
                      const uint8_t* y_plane,
                      int y_stride,
                      const uint8_t* u_plane,
                      int u_stride,
                      const uint8_t* v_plane,
                      int v_stride,
                      std::function<void()> no_longer_used)
        : width_(width)
        , height_(height)
        , y_plane_(y_plane)
        , u_plane_(u_plane)
        , v_plane_(v_plane)
        , y_stride_(y_stride)
        , u_stride_(u_stride)
        , v_stride_(v_stride)
        , no_longer_used_(no_longer_used)
    {
    }

    int width() const override { return width_; }
    int height() const override { return height_; }
    const uint8_t* GetDataY() const override { return y_plane_; }
    const uint8_t* GetDataU() const override { return u_plane_; }
    const uint8_t* GetDataV() const override { return v_plane_; }
    int StrideY() const override { return y_stride_; }
    int StrideU() const override { return u_stride_; }
    int StrideV() const override { return v_stride_; }

private:
    friend class RefCountedObject<WrappedI420Buffer>;
    ~WrappedI420Buffer() override { no_longer_used_(); }

    const int width_;
    const int height_;
    const uint8_t* const y_plane_;
    const uint8_t* const u_plane_;
    const uint8_t* const v_plane_;
    const int y_stride_;
    const int u_stride_;
    const int v_stride_;
    std::function<void()> no_longer_used_;
};

}  // namespace detail

// Creates an I420BufferInterface wrapping externally-owned memory. The
// `no_longer_used` callback is invoked (exactly once) when the last reference
// to the wrapped buffer is released.
inline SharedRefPtr<I420BufferInterface> WrapI420Buffer(int width,
                                                        int height,
                                                        const uint8_t* y_plane,
                                                        int y_stride,
                                                        const uint8_t* u_plane,
                                                        int u_stride,
                                                        const uint8_t* v_plane,
                                                        int v_stride,
                                                        std::function<void()> no_longer_used)
{
    return SharedRefPtr<I420BufferInterface>(
        new RefCountedObject<detail::WrappedI420Buffer>(width,
                                                        height,
                                                        y_plane,
                                                        y_stride,
                                                        u_plane,
                                                        u_stride,
                                                        v_plane,
                                                        v_stride,
                                                        no_longer_used));
}

}  // namespace cxxkit