/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present ChengXueWen.
**
** License: MIT License (derived from OpenCTK frame_generator.cpp SlideGenerator,
** trimmed to a self-contained I420 slideshow generator)
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

#include <cxxkit/media/frame_generator.hpp>

#include <cxxkit/media/i420_buffer.hpp>
#include <cxxkit/tools/checks.hpp>
#include <cxxkit/tools/random.hpp>

#include <cstring>

namespace cxxkit {

namespace {

// Fills frames with randomly sized and colored squares. Between each new
// generated frame the squares are freshly randomized (a new "slide").
class SlideShowVideoFrameGenerator : public FrameGenerator
{
public:
    SlideShowVideoFrameGenerator(int width, int height, int frame_repeat_count)
        : mWidth(width)
        , mHeight(height)
        , mFrameDisplayCount(frame_repeat_count)
        , mCurrentDisplayCount(0)
        , mRandomGenerator(1234)
    {
        CXXKIT_DCHECK_GT(width, 0);
        CXXKIT_DCHECK_GT(height, 0);
        CXXKIT_DCHECK_GT(frame_repeat_count, 0);
    }

    SharedRefPtr<VideoFrameBuffer> get_next_frame() override
    {
        if (mCurrentDisplayCount == 0)
        {
            generate_new_frame();
        }
        if (++mCurrentDisplayCount >= mFrameDisplayCount)
        {
            mCurrentDisplayCount = 0;
        }
        return mBuffer;
    }

    int width() const override { return mWidth; }
    int height() const override { return mHeight; }

private:
    void generate_new_frame()
    {
        // The squares should have a varying order of magnitude in order to
        // simulate variation in the slides' complexity.
        const int kSquareNum = 1 << (4 + (mRandomGenerator.rand(0, 3) * 2));

        mBuffer = I420Buffer::create(mWidth, mHeight);
        memset(mBuffer->mutable_data_y(), 127, static_cast<size_t>(mHeight) * mBuffer->stride_y());
        memset(mBuffer->mutable_data_u(), 127, static_cast<size_t>(mBuffer->chroma_height()) * mBuffer->stride_u());
        memset(mBuffer->mutable_data_v(), 127, static_cast<size_t>(mBuffer->chroma_height()) * mBuffer->stride_v());

        for (int i = 0; i < kSquareNum; ++i)
        {
            const int length = mRandomGenerator.rand(1, mWidth > 4 ? mWidth / 4 : 1);
            // Limit the length of later squares so that they don't overwrite
            // the previous ones too much.
            const int capped_length = (length * (kSquareNum - i)) / kSquareNum;

            const int x = mRandomGenerator.rand(0, mWidth - capped_length);
            const int y = mRandomGenerator.rand(0, mHeight - capped_length);
            const uint8_t yuv_y = static_cast<uint8_t>(mRandomGenerator.rand(0, 255));
            const uint8_t yuv_u = static_cast<uint8_t>(mRandomGenerator.rand(0, 255));
            const uint8_t yuv_v = static_cast<uint8_t>(mRandomGenerator.rand(0, 255));

            for (int yy = y; yy < y + capped_length; ++yy)
            {
                uint8_t* pos_y = mBuffer->mutable_data_y() + x + yy * mBuffer->stride_y();
                memset(pos_y, yuv_y, static_cast<size_t>(capped_length));
            }
            for (int yy = y; yy < y + capped_length; yy += 2)
            {
                uint8_t* pos_u = mBuffer->mutable_data_u() + x / 2 + yy / 2 * mBuffer->stride_u();
                memset(pos_u, yuv_u, static_cast<size_t>(capped_length) / 2);
                uint8_t* pos_v = mBuffer->mutable_data_v() + x / 2 + yy / 2 * mBuffer->stride_v();
                memset(pos_v, yuv_v, static_cast<size_t>(capped_length) / 2);
            }
        }
    }

    const int mWidth;
    const int mHeight;
    const int mFrameDisplayCount;
    int mCurrentDisplayCount;
    Random mRandomGenerator;
    SharedRefPtr<I420Buffer> mBuffer;
};

}  // namespace

std::unique_ptr<FrameGenerator> FrameGenerator::create_slide_show(std::vector<std::string> filenames,
                                                                OutputType type,
                                                                int width,
                                                                int height,
                                                                int frame_repeat_count)
{
    // ponytail: only the synthetic I420 slideshow is implemented — YUV-file
    // input was trimmed (M5). Pass an empty filenames list; non-empty returns
    // nullptr until a YuvFileGenerator is ported.
    if (!filenames.empty())
    {
        return std::unique_ptr<FrameGenerator>();
    }
    CXXKIT_DCHECK(type == OutputType::kI420);
    return std::unique_ptr<FrameGenerator>(
        new SlideShowVideoFrameGenerator(width, height, frame_repeat_count));
}

}  // namespace cxxkit