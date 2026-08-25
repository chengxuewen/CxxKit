########################################################################################################################
#
# Library: CxxKit
#
# Copyright (C) 2026~Present ChengXueWen.
#
# License: MIT License
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
# documentation files (the "Software"), to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
# to permit persons to whom the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies or substantial portions
# of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
# THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
# OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
########################################################################################################################

cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

cxxkit_configure_library_begin(CxxKitMedia)

set(CXXKIT_MEDIA_SOURCES
    color_space.cpp
    hdr_metadata.cpp
    i420_buffer.cpp
    video_frame.cpp
    framerate_controller.cpp
    video_frame_buffer_pool.cpp
    webrtc_libyuv.cpp)

cxxkit_find_package(Libyuv PROVIDED_TARGETS CxxKitWrapLibyuv::WrapLibyuv)
cxxkit_add_library(cxxkit_media
    SOURCES ${CXXKIT_MEDIA_SOURCES}
    PUBLIC_LIBRARIES cxxkit::base cxxkit::thread cxxkit::tools CxxKitWrapLibyuv::WrapLibyuv
    EXPORT_NAME media
    FOLDER "CxxKit/libs")
cxxkit_generate_pkg_config(cxxkit_media media)
cxxkit_configure_library_end(CxxKitMedia)
