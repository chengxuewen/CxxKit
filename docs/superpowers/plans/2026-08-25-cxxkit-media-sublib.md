# CxxKit media 子库实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为 CxxKit 新增 `cxxkit::media` 编译子库，提供视频帧基础设施（VideoFrameBuffer 接口家族、I420Buffer、VideoFrameBufferPool、VideoFrame、webrtc_libyuv 转换层）+ 帧生成器/捕获器 + 帧率控制器 + 色彩空间，并引入 vendored libyuv 作为格式转换后端。

**Architecture:** 遵循 CxxKit 现有约定——abseil 式目录=子库=CMake target、扁平 `cxxkit::` 命名空间、C++11 起步（测试 C++14）、内部 include 尖括号 `<cxxkit/...>`。依赖 CxxKit 现有子库（memory 的 ref_count/aligned_malloc、thread 的 race_checker、units 的 time_delta）。libyuv 作为 vendored 3rdparty 包（复用 OpenCTK `FindWrapLibyuv.cmake` + `libyuv.7z`）。

**Tech Stack:** CMake 3.15...3.31、C++11（库）/ C++14（测试）、GoogleTest 1.12.1、libyuv（vendored）、参考实现 libwebrtc m125 + OpenCTK media。

**Spec:** media 移植调研（团队 3 路分析 + 9 项交互式确认），详见本计划首部"决策记录"。

---

## 决策记录（2026-08-25 交互式逐项确认）

| # | 候选 | 来源 | 优先级 | 决策 |
|---|------|------|--------|------|
| 1 | VideoFrameBuffer 接口家族 | libwebrtc | **P0** | 移植（header-only，零 libyuv）|
| 2 | I420Buffer | libwebrtc | **P0** | 移植 + 引入 libyuv |
| 3 | VideoFrameBufferPool | libwebrtc | **P1** | 移植（裁到 I420+NV12）|
| 4 | VideoFrame（裁剪 RTP）| libwebrtc | **P1** | 移植（~250 行）|
| 5 | webrtc_libyuv 转换层 | libwebrtc | **P1** | 移植（纯 libyuv 封装）|
| 6 | framerate_controller | OpenCTK | **P0** | 移植 |
| 7 | color_space + hdr_metadata | OpenCTK | **P0** | 移植 |
| 8 | 帧生成器/捕获器 | OpenCTK | **P0** | 移植 |
| 9 | v4l2 camera capture | OpenCTK | **P1** | 移植（零依赖 Linux）|
| L1 | **libyuv（vendored）** | 上游 | — | 引入 |
| 10a/10b | pipewire + encoded_image | OpenCTK | — | 排除 |

---

## Global Constraints

1. **C++11 兼容性**: 库代码 `cxx_std_11`；移植 libwebrtc/OpenCTK 代码时降级 C++14/17 语法（`auto` 返回类型推导、`if constexpr`、泛型 lambda、`_t`/`_v` 别名/变量模板、数字分隔符）
2. **D2 命名空间**: 统一 `cxxkit::` 命名空间；内部实现 `cxxkit::detail::`；宏前缀 `CXXKIT_*`
3. **D8 尖括号 include**: 内部 include 一律 `<cxxkit/...>` 尖括号；tonistic 三方头 `<cxxkit/3rdparty/libyuv/...>`
4. **D9 #pragma once**: 新头用 `#pragma once`（不用旧式 `#ifndef` 守卫）
5. **D14 abseil 式目录**: 头 + 源 .cpp + CMakeLists 同目录，无 src/
6. **C10 target 注册**: 用 `cxxkit_add_library` helper，不直接 `add_library`
7. **C1/D4 三方依赖**: libyuv vendored + stamp，禁 FetchContent；复用 OpenCTK `FindWrapLibyuv.cmake` 适配 CxxKit 命名空间
8. **测试**: 每项移植配套 `tst_*.cpp` gtest 测试套件，通过 `cxxkit_add_test` 注册；测试 C++14
9. **格式化**: 提交前 `clang-format -i <file>`
10. **验证**: 每步完成后 `cmake --build build` 确认 0 error；`ctest --test-dir build -R tst_media --output-on-failure`
11. **命名迁移**: `rtc::scoped_refptr`→`cxxkit::RefPtr`、`RTC_CHECK`→`CXXKIT_CHECK`、`RTC_EXPORT`→`CXXKIT_MEDIA_API`、`webrtc::`/`octk::`→`cxxkit::`
12. **测试链接（B2）**: 所有 `tst_media_*` 的 `cxxkit_add_test` 必须 `LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} cxxkit::media`（CXXKIT_TEST_LINK_LIBRARIES 本身不含 media；Task 2 Step 7 模板即规范）
13. **API 命名规范源（M2）**: 以 **libwebrtc m125 为唯一规范命名源**，OpenCTK 旧缩写作废——`dataY()→GetDataY()`、`strideY()→StrideY()`、`VideoRotation::kAngle90→kVideoRotation_90`、`I420PSNR→I420Psnr`、`I420SSIM→I420Ssim`、`shouldDropFrame→ShouldDropFrame`。每个 Task 执行时先查 libwebrtc 对应头的签名
14. **裁剪原则**: 不迁移 RTP 相关（rtp_packet_infos/packet_infos/encoded_image）、不迁移编码器/解码器（openh264/av1/h264/vp8/vp9）、不迁移 pipewire/portal
15. **libyuv 版本**: OpenCTK 现有 `libyuv.7z`（428KB，SVN rev 1916）；复制到 `CxxKit/3rdparty/`

---

## 文件结构总览

```
cxxkit/
├── media/                          (新子库，编译)
│   ├── CMakeLists.txt
│   ├── media_global.hpp            (CXXKIT_MEDIA_API 导出宏)
│   ├── video_types.hpp             (VideoType 枚举 = OpenCTK video_type)
│   ├── video_rotation.hpp          (VideoRotation 枚举)
│   ├── color_space.hpp             (P0#7: 色彩空间 + cc)
│   ├── hdr_metadata.hpp            (P0#7: HDR 元数据 + cc)
│   ├── video_frame_buffer.hpp      (P0#1: 接口家族，header-only)
│   ├── detail/
│   │   └── video_frame_buffer_p.hpp  (接口私有实现，可内联)
│   ├── i420_buffer.hpp             (P0#2: I420Buffer + cc)
│   ├── video_frame.hpp             (P1#4: VideoFrame 裁剪版 + cc)
│   ├── video_frame_buffer_pool.hpp (P1#3: 缓冲池 + cc)
│   ├── webrtc_libyuv.hpp           (P1#5: 转换层 + cc)
│   ├── framerate_controller.hpp    (P0#6: 帧率控制 + cc)
│   ├── frame_generator.hpp         (P0#8: 帧生成器 + cc)
│   └── frame_generator_capturer.hpp(P0#8: 捕获器 + cc)
├── 3rdparty/
│   └── libyuv.7z                   (L1: 新增 vendored 包)
└── cmake/wrap/
    └── FindWrapLibyuv.cmake        (L1: 新增 wrap)
tests/
└── tst_media_*.cpp                 (每功能配套)
```

**说明**: v4l2 camera（P1#9）作为独立 Task 12，平台门控 Linux only + `CXXKIT_ENABLE_LIB_MEDIA_CAMERA` option。为控制范围，本计划优先 P0 四项 + libyuv（Task 1-8），P1 项（Task 9-11）和 v4l2（Task 12）列后可选执行。

---

## Task 1: libyuv vendored 引入（L1）

**Files:**
- Copy: `3rdparty/libyuv.7z`（从 `../OpenCTK/3rdparty/libyuv.7z`）
- Create: `cmake/wrap/FindWrapLibyuv.cmake`
- Modify: `CMakeLists.txt`（顶层 wrap 追加 Libyuv）

**Interfaces:**
- Consumes: `cxxkit_fetch_3rdparty` / `cxxkit_stamp_file_info` helper（OpenCTK `octk_*` 对应）
- Produces: `CxxKitWrapLibyuv::WrapLibyuv` INTERFACE IMPORTED target；`find_package(CxxKitWrapLibyuv)` 可被 media 子库消费

- [ ] **Step 1: 复制 libyuv.7z**

```bash
cp /home/ubuntu/Documents/remote_control_host_computer/src/3rdparty/OpenCTK/3rdparty/libyuv.7z \
   /home/ubuntu/Documents/remote_control_host_computer/src/3rdparty/CxxKit/3rdparty/
ls -la 3rdparty/libyuv.7z   # 期望 ~428KB
```

- [ ] **Step 2: 参考 OpenCTK FindWrapLibyuv.cmake 创建 CxxKit 版**

```bash
# 先读 OpenCTK 现成实现，复制后用 CxxKit helper 名替换
sed -n '30,109p' /home/ubuntu/Documents/remote_control_host_computer/src/3rdparty/OpenCTK/cmake/FindWrapLibyuv.cmake
```

适配要点：`octk_stamp_file_info`→`cxxkit_stamp_file_info`、`octk_fetch_3rdparty`→`cxxkit_fetch_3rdparty`、`OCTKWrapLibyuv::WrapLibyuv`→`CxxKitWrapLibyuv::WrapLibyuv`、`OCTK_NUMBER_OF_ASYNC_JOBS`→`CXXKIT_NUMBER_OF_ASYNC_JOBS`、target 前缀 `OpenCTKWrapLibyuv`→`CxxKitWrapLibyuv`。

- [ ] **Step 3: 创建 `cmake/wrap/FindWrapLibyuv.cmake`**

```cmake
# 基于 OpenCTK FindWrapLibyuv.cmake，适配 CxxKit 命名空间
if(TARGET CxxKitWrapLibyuv::WrapLibyuv)
    set(CxxKitWrapLibyuv_FOUND ON)
    return()
endif()

set(CxxKitWrapLibyuv_NAME "libyuv")
set(CxxKitWrapLibyuv_PKG_NAME "${CxxKitWrapLibyuv_NAME}.7z")
set(CxxKitWrapLibyuv_DIR_NAME "${CxxKitWrapLibyuv_NAME}-${CXXKIT_LOWER_BUILD_TYPE}")
set(CxxKitWrapLibyuv_URL_PATH "${PROJECT_SOURCE_DIR}/3rdparty/${CxxKitWrapLibyuv_PKG_NAME}")
set(CxxKitWrapLibyuv_ROOT_DIR "${PROJECT_BINARY_DIR}/3rdparty/${CxxKitWrapLibyuv_DIR_NAME}")
set(CxxKitWrapLibyuv_BUILD_DIR "${CxxKitWrapLibyuv_ROOT_DIR}/build" CACHE INTERNAL "" FORCE)
set(CxxKitWrapLibyuv_SOURCE_DIR "${CxxKitWrapLibyuv_ROOT_DIR}/source" CACHE INTERNAL "" FORCE)
set(CxxKitWrapLibyuv_INSTALL_DIR "${CxxKitWrapLibyuv_ROOT_DIR}/install" CACHE INTERNAL "" FORCE)
cxxkit_stamp_file_info(CxxKitWrapLibyuv OUTPUT_DIR "${CxxKitWrapLibyuv_ROOT_DIR}")
cxxkit_fetch_3rdparty(CxxKitWrapLibyuv URL "${CxxKitWrapLibyuv_URL_PATH}" OUTPUT_NAME "${CxxKitWrapLibyuv_DIR_NAME}")
if(NOT EXISTS "${CxxKitWrapLibyuv_STAMP_FILE_PATH}")
    # ... (复制 OpenCTK 的 configure/build/install execute_process 序列)
endif()
```

- [ ] **Step 4: 注册 wrap 到顶层**

在 `CMakeLists.txt` 顶层 wrap 列表中追加 `cxxkit_find_package(Libyuv)` 或按现有模式加入。参考 `cmake/wrap/FindWrapFmt.cmake` 的注册方式。

- [ ] **Step 5: 验证 wrap 可 find**

```bash
cmake -S . -B build 2>&1 | grep -i libyuv   # 期望: libyuv configure/build success
```

Expected: libyuv 编译成功（~30 秒），stamp 缓存。

- [ ] **Step 6: 提交**

```bash
git add 3rdparty/libyuv.7z cmake/wrap/FindWrapLibyuv.cmake CMakeLists.txt
git commit -m "feat(wrap): vendor libyuv (BSD-3, SVN 1916)

Add vendored libyuv as CxxKit 3rdparty package. Reuse OpenCTK
FindWrapLibyuv.cmake adapted to CxxKit namespace (CxxKitWrapLibyuv).
Native CMake build, ~30s first compile, stamp-cached thereafter.
Future base for media format conversion (I420/NV12/PSNR/SSIM).
"
```

---

## Task 2: media 子库骨架 + video_types + color_space + hdr_metadata（P0#7）

**Files:**
- Create: `cxxkit/media/CMakeLists.txt`
- Create: `cxxkit/media/media_global.hpp`
- Create: `cxxkit/media/video_types.hpp`（OpenCTK video_type）
- Create: `cxxkit/media/video_rotation.hpp`（libwebrtc video_rotation）
- Copy+adapt: `cxxkit/media/color_space.hpp/.cpp`（OpenCTK）
- Copy+adapt: `cxxkit/media/hdr_metadata.hpp/.cpp`（OpenCTK）
- Create: `tests/tst_media_color_space.cpp`

**Interfaces:**
- Consumes: `CxxKitWrapLibyuv::WrapLibyuv`（Task 1）、`cxxkit::base`/`cxxkit::memory`
- Produces:
  - `class cxxkit::ColorSpace`（PrimaryID/TransferID/MatrixID/RangeID 枚举 + `primaries()/transfer()/matrix()/range()` 访问器 + 参数构造，贴合 OpenCTK 源 API）
  - `struct cxxkit::HdrMetadata`（mastering display + content light level）
  - `enum class cxxkit::VideoType`（kI420/kNV12/kARGB/kRGB24...）
  - `enum class cxxkit::VideoRotation`（kVideoRotation_0/90/180/270）

- [ ] **Step 1: 创建 media 子库骨架**

`cxxkit/media/CMakeLists.txt`:
```cmake
file(GLOB _cxxkit_headers CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/detail/*.hpp
)
cxxkit_add_library(cxxkit_media
    HEADERS ${_cxxkit_headers}
    SOURCES color_space.cpp hdr_metadata.cpp
    LIBRARIES cxxkit::base cxxkit::memory CxxKitWrapLibyuv::WrapLibyuv
    FOLDER CxxKit/media
)
```

`cxxkit/media/media_global.hpp`:
```cpp
#pragma once
#ifndef CXXKIT_MEDIA_API
#  if defined(CXXKIT_BUILD_SHARED) && defined(CXXKIT_BUILDING_MEDIA_LIB)
#    define CXXKIT_MEDIA_API CXXKIT_EXPORT
#  elif defined(CXXKIT_BUILD_SHARED)
#    define CXXKIT_MEDIA_API CXXKIT_IMPORT
#  else
#    define CXXKIT_MEDIA_API
#  endif
#endif
```
（参考现有 `cxxkit/thread/thread_global.hpp` 导出宏结构，补 CXXKIT_EXPORT/IMPORT 宏）

- [ ] **Step 2: 编写失败测试 `tst_media_color_space.cpp`**

```cpp
#include <cxxkit/media/color_space.hpp>
#include <cxxkit/media/video_types.hpp>
#include <gtest/gtest.h>

TEST(ColorSpace, DefaultIsBt709) {
    cxxkit::ColorSpace cs(cxxkit::ColorSpace::PrimaryID::kBT709,
                          cxxkit::ColorSpace::TransferID::kBT709,
                          cxxkit::ColorSpace::MatrixID::kBT709,
                          cxxkit::ColorSpace::RangeID::kLimited);
    EXPECT_EQ(cs.primaries(), cxxkit::ColorSpace::PrimaryID::kBT709);
    EXPECT_EQ(cs.transfer(), cxxkit::ColorSpace::TransferID::kBT709);
    EXPECT_EQ(cs.matrix(), cxxkit::ColorSpace::MatrixID::kBT709);
    EXPECT_EQ(cs.range(), cxxkit::ColorSpace::RangeID::kLimited);
}

TEST(ColorSpace, ParamCtor) {
    cxxkit::ColorSpace cs(cxxkit::ColorSpace::PrimaryID::kBT2020,
                          cxxkit::ColorSpace::TransferID::kBT709,
                          cxxkit::ColorSpace::MatrixID::kBT2020_NCL,
                          cxxkit::ColorSpace::RangeID::kFull);
    EXPECT_EQ(cs.primaries(), cxxkit::ColorSpace::PrimaryID::kBT2020);
    EXPECT_EQ(cs.matrix(), cxxkit::ColorSpace::MatrixID::kBT2020_NCL);
    EXPECT_EQ(cs.range(), cxxkit::ColorSpace::RangeID::kFull);
}

TEST(VideoType, Sizes) {
    // I420: Y(w*h) + U(w/2*h/2) + V(w/2*h/2) = 1.5*w*h
    EXPECT_EQ(cxxkit::CalcBufferSize(cxxkit::VideoType::kI420, 2, 2), 6u);
    // NV12: Y(w*h) + UV(w/2*h/2*2) = 1.5*w*h
    EXPECT_EQ(cxxkit::CalcBufferSize(cxxkit::VideoType::kNV12, 2, 2), 6u);
}
```

- [ ] **Step 3: 运行测试确认失败**

Run: `cmake --build build --target cxxkit_tst_media_color_space 2>&1 | tail -5`
Expected: FAIL（`media/color_space.hpp` 不存在）

- [ ] **Step 4: 移植 color_space.hpp/.cpp**

从 OpenCTK `src/libs/media/source/video/color_space.hpp/.cpp` 复制，适配：
- `OCTK_BEGIN_NAMESPACE`→`namespace cxxkit {`
- `OCTK_MEDIA_API`→`CXXKIT_MEDIA_API`
- include `<openctk/media/...>`→`<cxxkit/media/...>`
- 保留 ColorSpace 枚举（**PrimaryID/TransferID/MatrixID/RangeID**）+ 私有成员 + `primaries()/transfer()/matrix()/range()` 访问器 + 参数构造器（贴合 OpenCTK，不发明 public member API）

- [ ] **Step 5: 移植 hdr_metadata.hpp/.cpp**

同 Step 4，从 OpenCTK `src/libs/media/source/video/hdr_metadata.hpp/.cpp` 复制适配。

- [ ] **Step 6: 移植 video_types.hpp + video_rotation.hpp**

```cpp
#pragma once
// video_types.hpp — pixel format enum
namespace cxxkit {
enum class VideoType {
    kUnknown, kI420, kIYUV, kRGB24, kABGR, kARGB, kARGB4444,
    kRGB565, kARGB1555, kYUY2, kYV12, kUYVY, kMJPEG, kNV12,
    kNV21, kBGRA, kI010, kI210, kI410, kI422, kI444,
};

// CalcBufferSize: 单一归属（inline 定义），Task 7 webrtc_libyuv 复用不重定义
inline size_t CalcBufferSize(VideoType type, int width, int height) {
    if (width <= 0 || height <= 0) return 0;
    switch (type) {
        case VideoType::kI420: case VideoType::kIYUV: case VideoType::kYV12:
        case VideoType::kNV12: case VideoType::kNV21:
            return static_cast<size_t>(width * height * 3 / 2);
        case VideoType::kRGB24:
            return static_cast<size_t>(width * height * 3);
        case VideoType::kABGR: case VideoType::kARGB: case VideoType::kBGRA:
            return static_cast<size_t>(width * height * 4);
        case VideoType::kI422: case VideoType::kUYVY: case VideoType::kYUY2:
            return static_cast<size_t>(width * height * 2);
        case VideoType::kI444: case VideoType::kI010:
            return static_cast<size_t>(width * height * 3);
        case VideoType::kMJPEG: case VideoType::kUnknown:
        case VideoType::kARGB4444: case VideoType::kARGB1555:
        case VideoType::kRGB565: case VideoType::kI210: case VideoType::kI410:
            return 0;  // 未支持格式
    }
    return 0;
}
}

#pragma once
// video_rotation.hpp
namespace cxxkit {
enum class VideoRotation { kVideoRotation_0 = 0, kVideoRotation_90 = 90,
                           kVideoRotation_180 = 180, kVideoRotation_270 = 270 };
}
```

- [ ] **Step 7: 注册测试 + 运行**

```cmake
# tests/CMakeLists.txt
cxxkit_add_test(cxxkit_tst_media_color_space
    SOURCES tst_media_color_space.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} cxxkit::media)
```
```bash
cmake --build build && ctest --test-dir build -R tst_media_color_space --output-on-failure
```
Expected: PASS

- [ ] **Step 8: 提交**

```bash
git add cxxkit/media/ tests/tst_media_color_space.cpp tests/CMakeLists.txt
git commit -m "feat(media): add media sublib skeleton, VideoType, ColorSpace, HdrMetadata

Port from OpenCTK media. Add ColorSpace (BT.601/709/2020 primaries,
transfer, matrix, range) + HdrMetadata (mastering display, content light).
Add VideoType pixel-format enum + CalcBufferSize. Media sublib target
(compiled) links cxxkit::base, cxxkit::memory, WrapLibyuv.
"
```

---

## Task 3: VideoFrameBuffer 接口家族（P0#1）

**Files:**
- Create: `cxxkit/media/video_frame_buffer.hpp`（header-only，接口 + Wrap 工厂声明）
- Create: `cxxkit/media/detail/video_frame_buffer_p.hpp`（内联实现）
- Create: `tests/tst_media_video_frame_buffer.cpp`

**Interfaces:**
- Consumes: `cxxkit::RefPtr`（memory/ref_count）、`cxxkit::ArrayView`（containers/array_view）、`cxxkit::ColorSpace`（Task 2）
- Produces:
  - `class VideoFrameBuffer`（抽象基类：width/height/type()/CropAndScale/Scale/ToI420...）
  - `class PlanarYuvBuffer`（10bit 分支接口）
  - `class PlanarYuv8Buffer`（GetDataY/GetDataU/GetDataV + Stride）
  - `class PlanarYuv16BBuffer`
  - `class BiplanarYuvBuffer`（GetDataY/GetDataUV）
  - `class I420BufferInterface` / `class NV12BufferInterface`（具体格式接口）
  - Wrap 工厂：`WrapI420Buffer(w,h,y,ys,u,us,v,vs,no_longer_used_fn)`

- [ ] **Step 1: 编写失败测试（本地 test double，不依赖 Task 4）**

```cpp
#include <cxxkit/media/video_frame_buffer.hpp>
#include <gtest/gtest.h>
#include <vector>

// 本地 test double：实现 I420BufferInterface（Task 4 才移植真实 I420Buffer）
class NativeI420Buffer final : public cxxkit::I420BufferInterface {
public:
    NativeI420Buffer(int w, int h)
        : data_(static_cast<size_t>(w) * h * 3 / 2),
          w_(w), h_(h), sy_(w), su_(w / 2), sv_(w / 2) {}
    int width() const override { return w_; }
    int height() const override { return h_; }
    const uint8_t* GetDataY() const override { return data_.data(); }
    const uint8_t* GetDataU() const override { return data_.data() + w_ * h_; }
    const uint8_t* GetDataV() const override { return data_.data() + w_ * h_ * 5 / 4; }
    int StrideY() const override { return sy_; }
    int StrideU() const override { return su_; }
    int StrideV() const override { return sv_; }
    cxxkit::VideoType type() const override { return cxxkit::VideoType::kI420; }

private:
    std::vector<uint8_t> data_;
    int w_, h_, sy_, su_, sv_;
};

TEST(VideoFrameBuffer, InterfaceType) {
    NativeI420Buffer buf(2, 2);
    EXPECT_EQ(buf.width(), 2);
    EXPECT_EQ(buf.height(), 2);
    EXPECT_EQ(buf.type(), cxxkit::VideoType::kI420);
    EXPECT_EQ(buf.StrideY(), 2);
    EXPECT_EQ(buf.ChromaWidth(), 1);
    EXPECT_EQ(buf.ChromaHeight(), 1);
}

TEST(VideoFrameBuffer, WrapI420Buffer) {
    std::vector<uint8_t> mem(6);
    bool released = false;
    auto wrapped = cxxkit::WrapI420Buffer(
        2, 2, mem.data(), 2, mem.data() + 4, 1, mem.data() + 5, 1,
        [&released]() { released = true; });
    EXPECT_TRUE(wrapped);
    EXPECT_EQ(wrapped->width(), 2);
    EXPECT_EQ(wrapped->type(), cxxkit::VideoType::kI420);
}
```

- [ ] **Step 2: 运行确认失败**

Run: `cmake --build build --target cxxkit_tst_media_video_frame_buffer 2>&1 | tail -5`
Expected: FAIL（`video_frame_buffer.hpp` 不存在——tst 编译错误）

- [ ] **Step 3: 移植接口类（header-only）**

从 libwebrtc `api/video/video_frame_buffer.h` + `common_video/include/video_frame_buffer.h` 复制接口，适配：
- `rtc::scoped_refptr`→`cxxkit::RefPtr`
- `RTC_DCHECK`→`CXXKIT_DCHECK`、`RTC_CHECK`→`CXXKIT_CHECK`
- `RTC_EXPORT`→`CXXKIT_MEDIA_API`
- `.cc` 中的 CropAndScale 默认实现 + type()/ChromaWidth 小函数 → 内联到头文件（保 header-only）
- 只保留 `VideoType::kI420` + `kNV12` + `kI422` + `kI444`（P1 范围），裁剪 `kI010/kI210/kI410`（和 I420Buffer 数据布局一起，见 Task 4）

- [ ] **Step 4: 注册测试 + 运行**

```bash
cmake --build build && ctest --test-dir build -R tst_media_video_frame_buffer --output-on-failure
```

- [ ] **Step 5: 提交**

```bash
git add cxxkit/media/video_frame_buffer.hpp cxxkit/media/detail/video_frame_buffer_p.hpp \
        tests/tst_media_video_frame_buffer.cpp
git commit -m "feat(media): add VideoFrameBuffer interface family (header-only)

Port from libwebrtc api/video/video_frame_buffer.h. Abstract buffer
interface: VideoFrameBuffer, PlanarYuvBuffer (8/16bit), BiplanarYuvBuffer,
I420BufferInterface, NV12BufferInterface. Header-only (inline .cc default
impls). WrapI420Buffer etc factory for external memory with std::function
release callback. C++11, cxxkit:: namespace.
"
```

---

## Task 4: I420Buffer（P0#2，libyuv 后端）

**Files:**
- Create: `cxxkit/media/i420_buffer.hpp` + `cxxkit/media/i420_buffer.cpp`
- Modify: `cxxkit/media/CMakeLists.txt`（加 i420_buffer.cpp）
- Create: `tests/tst_media_i420_buffer.cpp`

**Interfaces:**
- Consumes: `VideoFrameBuffer`（Task 3）、`libyuv`（Task 1）、`cxxkit::AlignedMalloc`（memory）
- Produces:
  - `class I420Buffer : public I420BufferInterface`（ref-counted）
    - `static RefPtr<I420Buffer> Create(int w, int h)` / `Create(int w, int h, int sy, int su, int sv)`
    - `static RefPtr<I420Buffer> Copy(const I420BufferInterface&)`
    - `static RefPtr<I420Buffer> Rotate(const I420BufferInterface&, VideoRotation)`
    - `SetBlack()`, `InitializeData()`, `CropAndScaleFrom(...)`, `ScaleFrom(...)`

- [ ] **Step 1: 编写失败测试**

```cpp
#include <cxxkit/media/i420_buffer.hpp>
#include <cxxkit/media/video_frame_buffer.hpp>
#include <gtest/gtest.h>

TEST(I420Buffer, Create) {
    auto buf = cxxkit::I420Buffer::Create(16, 16);
    ASSERT_TRUE(buf);
    EXPECT_EQ(buf->width(), 16);
    EXPECT_EQ(buf->height(), 16);
    EXPECT_EQ(buf->type(), cxxkit::VideoType::kI420);
}

TEST(I420Buffer, Copy) {
    auto src = cxxkit::I420Buffer::Create(4, 4);
    src->SetBlack();  // Y=0, U=128, V=128 for black
    auto dst = cxxkit::I420Buffer::Copy(*src);
    ASSERT_TRUE(dst);
    EXPECT_EQ(dst->width(), 4);
    EXPECT_EQ(dst->height(), 4);
    // 校验 Y 平面为 0
    const auto* y = dst->DataY();
    for (int i = 0; i < 4 * 4; ++i) EXPECT_EQ(y[i], 0);
}

TEST(I420Buffer, SetBlack) {
    auto buf = cxxkit::I420Buffer::Create(4, 4);
    buf->SetBlack();
    const auto* y = buf->DataY();
    for (int i = 0; i < 16; ++i) EXPECT_EQ(y[i], 0);  // Y=0
}

TEST(I420Buffer, DataLayout) {
    auto buf = cxxkit::I420Buffer::Create(4, 4);
    // I420: Y 平面 w*h, U/V 平面 w/2*h/2，各 stride
    EXPECT_EQ(buf->StrideY(), 4);
    EXPECT_EQ(buf->StrideU(), 2);
    EXPECT_EQ(buf->StrideV(), 2);
}
```

- [ ] **Step 2: 运行确认失败**

Run: `cmake --build build --target cxxkit_tst_media_i420_buffer 2>&1 | tail -5`
Expected: FAIL（I420Buffer 未定义）

- [ ] **Step 3: 移植 I420Buffer 数据布局（i420_buffer.hpp）**

从 OpenCTK `src/libs/media/source/video/i420_buffer.hpp/.cpp`（已是 webrtc I420Buffer 的 C++11 移植）复制，适配：
- **引用计数迁移（M3）**: OpenCTK 是 `std::shared_ptr` 体系（`OCTK_DEFINE_SHARED_PTR`、`toI420()` 返回 shared_ptr）；需改为挂 `cxxkit::RefCounted`（`memory/ref_counted_object.hpp` 已有）+ `cxxkit::RefPtr`——**结构性重写非命名空间适配**。接口家族所有返回类型 shared_ptr→RefPtr：`ToI420()`、`Create()`、`Copy()`、`Rotate()`、`CropAndScaleFrom()`、`ScaleFrom()`
- **命名规范（M2）**: 统一 webrtc 大写风 `GetDataY()/GetDataU()/GetDataV()/StrideY()/StrideU()/StrideV()`（OpenCTK 小写 `dataY()` 是 webrtc 旧版命名，作废）
- 核心结构：
- `I420BufferInterface` 继承 `PlanarYuv8Buffer` + `VideoFrameBuffer`
- `I420Buffer` 继承 `I420BufferInterface`，持有 `std::unique_ptr<uint8_t[]>`（aligned_malloc）
- `Create`/`Copy`/`Rotate`/`SetBlack`/`InitializeData` 工厂

- [ ] **Step 4: 移植 libyuv 后端（i420_buffer.cpp）**

从 OpenCTK `src/libs/media/source/video/video_frame_buffer.cpp` 提取 I420 相关：
- `I420Copy` → `libyuv::I420Copy`（Copy）
- `SetBlack` → 初始化 Y=0, U=128, V=128（或 libyuv::I420Rect）
- `Rotate` → `libyuv::I420Rotate`（kRotate0/90/180/270）
- `CropAndScaleFrom` → `libyuv::I420Scale` + `libyuv::ScalePlane`

```cpp
RefPtr<I420Buffer> I420Buffer::Rotate(const I420BufferInterface& src, VideoRotation rotation) {
    int dst_w = (rotation == kVideoRotation_90 || rotation == kVideoRotation_270)
                    ? src.height() : src.width();
    int dst_h = (rotation == kVideoRotation_90 || rotation == kVideoRotation_270)
                    ? src.width() : src.height();
    auto dst = I420Buffer::Create(dst_w, dst_h);
    libyuv::I420Rotate(src.GetDataY(), src.StrideY(), src.GetDataU(), src.StrideU(),
                       src.GetDataV(), src.StrideV(), dst->MutableDataY(),
                       dst->StrideY(), dst->MutableDataU(), dst->StrideU(),
                       dst->MutableDataV(), dst->StrideV(), src.width(),
                       src.height(), ToLibyuvRotation(rotation));
    return dst;
}
```

- [ ] **Step 5: 运行测试确认通过**

```bash
cmake --build build --target cxxkit_tst_media_i420_buffer
./build/tests/cxxkit_tst_media_i420_buffer
```
Expected: 全部 PASS

- [ ] **Step 6: 更新 Task 3 的接口测试（补全 I420BufferInterface 断言）**

在 `tst_media_video_frame_buffer.cpp` 补 I420Buffer 相关接口测试（此前 I420Buffer 未实现跳过）。

- [ ] **Step 7: 提交**

```bash
git add cxxkit/media/i420_buffer.hpp cxxkit/media/i420_buffer.cpp \
        cxxkit/media/CMakeLists.txt tests/tst_media_i420_buffer.cpp
git commit -m "feat(media): add I420Buffer with libyuv backend

Port from OpenCTK i420_buffer. I420Buffer: Create(width,height[,strides]),
Copy, Rotate (90/180/270), SetBlack, InitializeData, CropAndScaleFrom,
ScaleFrom. 64-byte aligned via AlignedMalloc. libyuv backend for
I420Copy/I420Rotate/I420Scale. Ref-counted via RefCountedObject pattern.
"
```

---

## Task 5: VideoFrame（裁剪版，P1#4）

**Files:**
- Create: `cxxkit/media/video_frame.hpp` + `cxxkit/media/video_frame.cpp`
- Modify: `cxxkit/media/CMakeLists.txt`
- Create: `tests/tst_media_video_frame.cpp`

**Interfaces:**
- Consumes: `I420BufferInterface`（Task 4）、`ColorSpace`（Task 2）、`units::TimeDelta`/`Timestamp`（cxxkit units）
- Produces:
  - `class VideoFrame`
    - `VideoFrame::Builder`（set_video_frame_buffer/set_timestamp_rtp/set_timestamp_us/set_rotation/set_color_space/build）
    - `video_frame_buffer()`, `timestamp_rtp()`, `timestamp_us()`, `rotation()`, `color_space()`
    - `UpdateRect` 嵌套类（`Union`/`Intersect`/`ScaleWithFrame` 面积算法）
  - 裁剪：不移植 rtp_packet_infos/packet_infos/render_parameters

- [ ] **Step 1: 编写失败测试**

```cpp
#include <cxxkit/media/video_frame.hpp>
#include <cxxkit/media/i420_buffer.hpp>
#include <gtest/gtest.h>

TEST(VideoFrame, Builder) {
    auto buf = cxxkit::I420Buffer::Create(16, 16);
    auto frame = cxxkit::VideoFrame::Builder()
        .set_video_frame_buffer(buf)
        .set_timestamp_us(1000)
        .build();
    EXPECT_EQ(frame.video_frame_buffer(), buf);
    EXPECT_EQ(frame.timestamp_us(), 1000);
}

TEST(VideoFrame, UpdateRectUnion) {
    cxxkit::VideoFrame::UpdateRect a{0, 0, 10, 10};
    cxxkit::VideoFrame::UpdateRect b{5, 5, 10, 10};
    auto u = a.Union(b);
    EXPECT_EQ(u.x, 0); EXPECT_EQ(u.y, 0);
    EXPECT_EQ(u.width, 15); EXPECT_EQ(u.height, 15);
}
```

- [ ] **Step 2: 运行确认失败**

Run: `cmake --build build --target cxxkit_tst_media_video_frame 2>&1 | tail -5`
Expected: FAIL

- [ ] **Step 3: 移植 VideoFrame（裁剪 RTP）**

从 libwebrtc `api/video/video_frame.h/.cc` + OpenCTK `video_frame.hpp/.cpp` 裁剪：
- 保留：Builder、id、timestamp_rtp/timestamp_us、rotation、color_space、update_rect（Union/Intersect/ScaleWithFrame 面积算法 ~100 行）
- 裁剪：`rtp_packet_infos()`、`packet_infos_` 字段、`render_parameters`、`processing_time`
- 命名：`units::TimeDelta`/`units::Timestamp`（CxxKit units），不用 webrtc `webrtc::TimeDelta`

- [ ] **Step 4: 运行测试确认通过**

```bash
cmake --build build --target cxxkit_tst_media_video_frame && ./build/tests/cxxkit_tst_media_video_frame
```

- [ ] **Step 5: 提交**

```bash
git add cxxkit/media/video_frame.hpp cxxkit/media/video_frame.cpp \
        cxxkit/media/CMakeLists.txt tests/tst_media_video_frame.cpp
git commit -m "feat(media): add VideoFrame (trimmed RTP-free metadata container)

Port from libwebrtc/OpenCTK. VideoFrame: Builder + metadata (id,
timestamp_us, timestamp_rtp, rotation, color_space) + UpdateRect
(Union/Intersect/ScaleWithFrame). Trimmed RTP fields (rtp_packet_infos,
packet_infos, render_parameters) — CxxKit has no RTP requirement now.
C++11, units::Timestamp/TimeDelta.
"
```

---

## Task 6: VideoFrameBufferPool（P1#3）

**Files:**
- Create: `cxxkit/media/video_frame_buffer_pool.hpp` + `.cpp`
- Modify: `cxxkit/media/CMakeLists.txt`
- Create: `tests/tst_media_video_frame_buffer_pool.cpp`

**Interfaces:**
- Consumes: `I420Buffer`（Task 4）
- Produces:
  - `class VideoFrameBufferPool`
    - `RefPtr<I420BufferInterface> CreateI420Buffer(int w, int h)`
    - `bool Resize(int max_size)`, `void Release()`
    - 内部：空闲列表按 (w,h,Type) 复用（refcount==1 回池），race_checker 守卫

- [ ] **Step 1: 编写失败测试**

```cpp
#include <cxxkit/media/video_frame_buffer_pool.hpp>
#include <gtest/gtest.h>

TEST(VideoFrameBufferPool, ReusesBuffer) {
    cxxkit::VideoFrameBufferPool pool;
    {
        auto b1 = pool.CreateI420Buffer(16, 16);
        EXPECT_TRUE(b1);
    }  // b1 析构，refcount==0 → 回池
    auto b2 = pool.CreateI420Buffer(16, 16);
    auto b3 = pool.CreateI420Buffer(16, 16);
    EXPECT_TRUE(b2); EXPECT_TRUE(b3);
}

TEST(VideoFrameBufferPool, DifferentSizeNoReuse) {
    cxxkit::VideoFrameBufferPool pool;
    void* first;
    {
        auto b1 = pool.CreateI420Buffer(16, 16);
        first = b1->MutableDataY();
    }
    auto b2 = pool.CreateI420Buffer(32, 32);  // 不同尺寸 → 不复用
    EXPECT_NE(b2->MutableDataY(), first);
}
```

- [ ] **Step 2: 运行确认失败**

Run: `cmake --build build --target cxxkit_tst_media_video_frame_buffer_pool 2>&1 | tail -5`
Expected: FAIL

- [ ] **Step 3: 移植 VideoFrameBufferPool（I420-only，M4）**

从 OpenCTK `src/libs/media/source/video/video_frame_buffer_pool.hpp/.cpp` 复制，**只留 I420**（NV12Buffer 是可选 Task 11 才移植，Task 6 不引用）。砍 i010/i210/i410/i422/i444/nv12。核心逻辑：`list<int index>` 空槽、`FreeTuple`（(w,h,type) 判等）、`Recycle`（refcount==1 时回池）。

- [ ] **Step 4: 运行 + 提交**

```bash
cmake --build build && ctest --test-dir build -R tst_media_video_frame_buffer_pool --output-on-failure
git add cxxkit/media/video_frame_buffer_pool.* tests/tst_media_video_frame_buffer_pool.cpp
git commit -m "feat(media): add VideoFrameBufferPool (I420+NV12 trimmed)

Port from OpenCTK. Reuse frame buffers by (w,h,type) when refcount==1,
avoid per-frame malloc. Resize(max), Release(), race_checker guard.
"
```

---

## Task 7: webrtc_libyuv 转换层（P1#5）

**Files:**
- Create: `cxxkit/media/webrtc_libyuv.hpp` + `cxxkit/media/webrtc_libyuv.cpp`
- Modify: `cxxkit/media/CMakeLists.txt`
- Create: `tests/tst_media_webrtc_libyuv.cpp`

**Interfaces:**
- Consumes: `I420BufferInterface`（Task 4）、`libyuv`
- Produces:
  - `CalcBufferSize(VideoType, w, h)`
  - `ExtractBuffer(const I420BufferInterface&, size_t, uint8_t*)`
  - `ConvertFromI420(const VideoFrame&, VideoType dst, int dst_w, int dst_h, uint8_t* dst...)`
  - `ScaleVideoFrameBuffer(const I420BufferInterface&, int w, int h)`
  - `I420Psnr(const I420BufferInterface&, const I420BufferInterface&)`
  - `I420Ssim(const I420BufferInterface&, const I420BufferInterface&)`

- [ ] **Step 1: 编写失败测试**

```cpp
#include <cxxkit/media/webrtc_libyuv.hpp>
#include <cxxkit/media/i420_buffer.hpp>
#include <gtest/gtest.h>

TEST(WebRtcLibyuv, CalcBufferSize) {
    EXPECT_EQ(cxxkit::CalcBufferSize(cxxkit::VideoType::kI420, 2, 2), 6u);
    EXPECT_EQ(cxxkit::CalcBufferSize(cxxkit::VideoType::kNV12, 2, 2), 6u);
    EXPECT_EQ(cxxkit::CalcBufferSize(cxxkit::VideoType::kARGB, 2, 2), 16u);
}

TEST(WebRtcLibyuv, ExtractBuffer) {
    auto src = cxxkit::I420Buffer::Create(4, 4);
    src->SetBlack();
    size_t size = cxxkit::CalcBufferSize(cxxkit::VideoType::kI420, 4, 4);
    std::vector<uint8_t> buf(size);
    int ret = cxxkit::ExtractBuffer(*src, size, buf.data());
    EXPECT_EQ(ret, static_cast<int>(size));
}

TEST(WebRtcLibyuv, PSNRSameFrame) {
    auto a = cxxkit::I420Buffer::Create(8, 8);
    a->SetBlack();
    double psnr = cxxkit::I420Psnr(*a, *a);
    EXPECT_GT(psnr, 60.0);  // 相同帧 PSNR 很高
}
```

- [ ] **Step 2: 运行确认失败**

Run: `cmake --build build --target cxxkit_tst_media_webrtc_libyuv 2>&1 | tail -5`
Expected: FAIL

- [ ] **Step 3: 移植 webrtc_libyuv（libyuv 封装）**

从 libwebrtc `common_video/libyuv/webrtc_libyuv.cc` + OpenCTK `video/yuv.cpp` 移植，适配命名空间 + 裁剪。核心封装 libyuv API：
- `CalcBufferSize` → 格式字节数表
- `ExtractBuffer` → `libyuv::I420Copy` 到连续缓冲
- `ConvertFromI420` → 按目标格式调 `libyuv::ConvertFromI420`（NV12/ARGB/RGB24/RGB565）
- `ScaleVideoFrameBuffer` → `libyuv::I420Scale` + `ScalePlane`
- `I420Psnr`/`I420Ssim` → `libyuv::I420Psnr`/`libyuv::I420Ssim`

- [ ] **Step 4: 运行 + 提交**

```bash
cmake --build build && ctest --test-dir build -R tst_media_webrtc_libyuv --output-on-failure
git add cxxkit/media/webrtc_libyuv.* tests/tst_media_webrtc_libyuv.cpp
git commit -m "feat(media): add webrtc_libyuv conversion layer (I420/NV12/PSNR/SSIM)

Port from libwebrtc/OpenCTK. Wrapper over vendored libyuv:
CalcBufferSize, ExtractBuffer, ConvertFromI420 (NV12/ARGB/RGB24/RGB565),
ScaleVideoFrameBuffer, I420Psnr, I420Ssim. NV12 hardware-encoder input,
PSNR/SSIM for quality testing.
"
```

---

## Task 8: framerate_controller（P0#6）

**Files:**
- Create: `cxxkit/media/framerate_controller.hpp` + `.cpp`
- Modify: `cxxkit/media/CMakeLists.txt`
- Create: `tests/tst_media_framerate_controller.cpp`

**Interfaces:**
- Consumes: `cxxkit::units::TimeDelta`/`Timestamp`
- Produces:
  - `class FramerateController`
    - `void SetFrameRate(double fps)` / `double GetFrameRate() const`
    - `bool ShouldDropFrame(int64_t inTimestampNSecs)`（OpenCTK 实际 API；判断该时间戳是否需丢帧以保持目标帧率）

- [ ] **Step 1: 编写失败测试**

```cpp
#include <cxxkit/media/framerate_controller.hpp>
#include <gtest/gtest.h>

TEST(FramerateController, SetGet) {
    cxxkit::FramerateController fc;
    fc.SetFrameRate(30.0);
    EXPECT_DOUBLE_EQ(fc.GetFrameRate(), 30.0);
}

TEST(FramerateController, ShouldDropFrame) {
    cxxkit::FramerateController fc(30.0);  // 30fps → 帧间隔 ~33.3ms
    // 同一时间戳连续查询：第二帧必然丢帧（未到间隔）
    EXPECT_FALSE(fc.ShouldDropFrame(0));
    EXPECT_TRUE(fc.ShouldDropFrame(0));       // 距上帧 0ms < 33ms → 丢
    // 时间推进到下一帧间隔后：不丢
    EXPECT_FALSE(fc.ShouldDropFrame(40000000));  // +40ms > 33.3ms → 保留
}
```

- [ ] **Step 2: 运行确认失败**

Run: `cmake --build build --target cxxkit_tst_media_framerate_controller 2>&1 | tail -5`
Expected: FAIL

- [ ] **Step 3: 移植 framerate_controller**

从 OpenCTK `src/libs/media/source/video/framerate_controller.hpp/.cpp` 复制，适配命名空间。核心：`ShouldDropFrame(int64_t inTimestampNSecs)` 用目标帧间隔 + 时间戳判断是否丢帧（OpenCTK 原 API，不用 webrtc AddFrame/DropFrame）。

- [ ] **Step 4: 运行 + 提交**

```bash
cmake --build build && ctest --test-dir build -R tst_media_framerate_controller --output-on-failure
git add cxxkit/media/framerate_controller.* tests/tst_media_framerate_controller.cpp
git commit -m "feat(media): add FramerateController

Port from OpenCTK. SetFrameRate/GetFrameRate/AddFrame/DropFrame.
Throttle frame emission to target fps using units::Timestamp.
"
```

---

## Task 9: frame_generator + frame_generator_capturer（P0#8）

**Files:**
- Create: `cxxkit/media/frame_generator.hpp` + `frame_generator.cpp`
- Create: `cxxkit/media/frame_generator_capturer.hpp` + `frame_generator_capturer.cpp`
- Modify: `cxxkit/media/CMakeLists.txt`
- Create: `tests/tst_media_frame_generator.cpp`

**Interfaces:**
- Consumes: `VideoFrame`（Task 5）、`framerate_controller`（Task 8）、`webrtc_libyuv`（Task 7）
- Produces:
  - `enum class FrameGenerator::OutputType`（kI420）
  - `class FrameGenerator`（抽象：`GetNextFrame()` → `RefPtr<VideoFrameBuffer>`）
  - `class FrameGeneratorCapturer`（自包含发射循环 + `SetFrameCallback` 回调；`CreateFrameGeneratorCapturer` 工厂）——**不继承 VideoTrackSource**（该框架类未移植）

- [ ] **Step 1: 编写失败测试**

```cpp
#include <cxxkit/media/frame_generator.hpp>
#include <gtest/gtest.h>

TEST(FrameGenerator, SlideShow) {
    auto gen = cxxkit::FrameGenerator::CreateSlideShow(
        std::vector<std::string>{}, cxxkit::FrameGenerator::OutputType::kI420,
        640, 480, 1);
    auto frame = gen->GetNextFrame();
    EXPECT_TRUE(frame);
    EXPECT_EQ(frame->width(), 640);
    EXPECT_EQ(frame->height(), 480);
}
```

- [ ] **Step 2: 运行确认失败**

Run: `cmake --build build --target cxxkit_tst_media_frame_generator 2>&1 | tail -5`
Expected: FAIL

- [ ] **Step 3: 移植 FrameGenerator + Capturer（自包含裁剪，M5）**

**关键裁剪声明**: OpenCTK `FrameGeneratorCapturer` 继承 `CustomVideoCapturer`，且依赖 `video_track_source/video_broadcaster/video_adapter/repeating_task/task_queue_thread` 等 6+ 框架类（CxxKit 未移植）。**本计划不移植这些框架**——改为：
- `frame_generator.cpp`（FrameGenerator 抽象 + SlideShow 实现，I420 输出，砍 I420A）
- `create_frame_generator.cpp`（工厂）
- `frame_generator_capturer.cpp`：裁剪为**自包含发射循环**——不用 VideoTrackSource/broadcaster/adapter 继承，提供简单回调接口 `SetFrameCallback(std::function<void(const VideoFrame&)>)` + 内部用 framerate_controller 节流的轮询/触发出帧
- 只留 I420 输出

- [ ] **Step 4: 运行 + 提交**

```bash
cmake --build build && ctest --test-dir build -R tst_media_frame_generator --output-on-failure
git add cxxkit/media/frame_generator.* cxxkit/media/frame_generator_capturer.* \
        tests/tst_media_frame_generator.cpp
git commit -m "feat(media): add FrameGenerator + FrameGeneratorCapturer

Port from OpenCTK capture/custom. FrameGenerator (abstract + SlideShow
impl, I420 output), CreateFrameGeneratorCapturer factory. Drive frame
emission via FramerateController. For testing/placeholder/demo.
"
```

---

## 可选 Task 10-12（P1#9 v4l2；后续增强）

### Task 10: v4l2 camera capture（P1#9，Linux only）
- 从 OpenCTK `src/libs/media/source/capture/camera/camera_capture_v4l2*` 移植
- 平台门控：`CXXKIT_ENABLE_LIB_MEDIA_CAMERA` option + `if(UNIX AND NOT APPLE)`
- 零第三方依赖（Linux V4L2 内核 API）
- 依赖 camera_device_info_v4l2 + camera_capture 框架

### Task 11: NV12Buffer（缓冲池/转换需要的 NV12 支持）
- 从 OpenCTK `nv12_buffer.hpp/.cpp` + libwebrtc `nv12_buffer.cc` 移植
- 供 Task 6 缓冲池 + Task 7 NV12 转换使用

### Task 12: 缓冲池补全 + doxygen + 覆盖率
- 补注释、文档、集成到 BuildAll/BuildInstall

---

## 实施顺序与依赖

```
Task 1: libyuv vendored ──────────────────────────────┐ (L1, 基础)
Task 2: media 骨架 + ColorSpace/VideoType ────────────┤ (P0 基础)
Task 3: VideoFrameBuffer 接口家族 <──(Task2 ColorSpace)┤
Task 4: I420Buffer <──(Task3 接口 + Task1 libyuv) ────┤
Task 5: VideoFrame <──(Task4 + Task2) ────────────────┤
Task 6: VideoFrameBufferPool <──(Task4) ──────────────┤
Task 7: webrtc_libyuv <──(Task4 + Task1) ─────────────┤
Task 8: framerate_controller ─────────────────────────┘ (独立)

Task 9: FrameGenerator <──(Task 4/5/7/8)
Task 10: v4l2 camera (可选, Linux)
Task 11: NV12Buffer (可选)
```

**核心依赖链**: Task 1→2→3→4→5/6/7→9。Task 8 独立可并行。

**P0 范围**: Task 1-5 + 8（libyuv + 骨架 + 接口 + I420 + VideoFrame + framerate）
**P1 可选**: Task 6 (pool) + 7 (webrtc_libyuv) + 9 (frame_generator) + 11 (NV12)
**P2 门控**: Task 10 (v4l2 camera, Linux)

---

## 验收门禁

**P0 完成**（Task 1-5 + 8）:
1. `cmake --build build` 0 error
2. `ctest --test-dir build -R "tst_media" --output-on-failure` 全 PASS
3. `clang-format` 通过
4. libyuv vendored 构建成功（stamp 缓存）

**全量完成**（Task 1-11）:
- `ctest --test-dir build --output-on-failure`（~60+ 套件全过，新增 8 个 media 测试）
- coverage 保持 ≥80%（新增 media 编译 .cpp 需配套测试）
- `bash scripts/check.sh` 本地门禁通过
