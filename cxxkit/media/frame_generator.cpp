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
        : width_(width)
        , height_(height)
        , frame_display_count_(frame_repeat_count)
        , current_display_count_(0)
        , random_generator_(1234)
    {
        CXXKIT_DCHECK_GT(width, 0);
        CXXKIT_DCHECK_GT(height, 0);
        CXXKIT_DCHECK_GT(frame_repeat_count, 0);
    }

    SharedRefPtr<VideoFrameBuffer> GetNextFrame() override
    {
        if (current_display_count_ == 0)
        {
            generateNewFrame();
        }
        if (++current_display_count_ >= frame_display_count_)
        {
            current_display_count_ = 0;
        }
        return buffer_;
    }

    int width() const override { return width_; }
    int height() const override { return height_; }

private:
    void generateNewFrame()
    {
        // The squares should have a varying order of magnitude in order to
        // simulate variation in the slides' complexity.
        const int kSquareNum = 1 << (4 + (random_generator_.Rand(0, 3) * 2));

        buffer_ = I420Buffer::Create(width_, height_);
        memset(buffer_->MutableDataY(), 127, static_cast<size_t>(height_) * buffer_->StrideY());
        memset(buffer_->MutableDataU(), 127, static_cast<size_t>(buffer_->ChromaHeight()) * buffer_->StrideU());
        memset(buffer_->MutableDataV(), 127, static_cast<size_t>(buffer_->ChromaHeight()) * buffer_->StrideV());

        for (int i = 0; i < kSquareNum; ++i)
        {
            const int length = random_generator_.Rand(1, width_ > 4 ? width_ / 4 : 1);
            // Limit the length of later squares so that they don't overwrite
            // the previous ones too much.
            const int capped_length = (length * (kSquareNum - i)) / kSquareNum;

            const int x = random_generator_.Rand(0, width_ - capped_length);
            const int y = random_generator_.Rand(0, height_ - capped_length);
            const uint8_t yuv_y = static_cast<uint8_t>(random_generator_.Rand(0, 255));
            const uint8_t yuv_u = static_cast<uint8_t>(random_generator_.Rand(0, 255));
            const uint8_t yuv_v = static_cast<uint8_t>(random_generator_.Rand(0, 255));

            for (int yy = y; yy < y + capped_length; ++yy)
            {
                uint8_t* pos_y = buffer_->MutableDataY() + x + yy * buffer_->StrideY();
                memset(pos_y, yuv_y, static_cast<size_t>(capped_length));
            }
            for (int yy = y; yy < y + capped_length; yy += 2)
            {
                uint8_t* pos_u = buffer_->MutableDataU() + x / 2 + yy / 2 * buffer_->StrideU();
                memset(pos_u, yuv_u, static_cast<size_t>(capped_length) / 2);
                uint8_t* pos_v = buffer_->MutableDataV() + x / 2 + yy / 2 * buffer_->StrideV();
                memset(pos_v, yuv_v, static_cast<size_t>(capped_length) / 2);
            }
        }
    }

    const int width_;
    const int height_;
    const int frame_display_count_;
    int current_display_count_;
    Random random_generator_;
    SharedRefPtr<I420Buffer> buffer_;
};

}  // namespace

std::unique_ptr<FrameGenerator> FrameGenerator::CreateSlideShow(std::vector<std::string> filenames,
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