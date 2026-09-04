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
# the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies or substantial portions
# of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
# TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
# CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
########################################################################################################################

# Complete wrap: SDL3 has an official CMakeLists, so we configure + build + install it into the wrap
# install dir (libSDL3.a + SDL3/ headers), like FindWrapLibyuv. SDL3 static link needs the C library
# runtime bits (m/dl/pthread) — probed at configure time, no speculative windowing/GL deps (T9 later).

# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET CxxKitWrapSDL3::WrapSDL3)
    set(CxxKitWrapSDL3_FOUND ON)
    return()
endif()

set(CxxKitWrapSDL3_NAME "sdl3-release-3.4.16")
set(CxxKitWrapSDL3_PKG_NAME "${CxxKitWrapSDL3_NAME}.7z")
set(CxxKitWrapSDL3_DIR_NAME "${CxxKitWrapSDL3_NAME}-${CXXKIT_LOWER_BUILD_TYPE}")
set(CxxKitWrapSDL3_URL_PATH "${PROJECT_SOURCE_DIR}/3rdparty/${CxxKitWrapSDL3_PKG_NAME}")
set(CxxKitWrapSDL3_ROOT_DIR "${PROJECT_BINARY_DIR}/3rdparty/${CxxKitWrapSDL3_DIR_NAME}")
set(CxxKitWrapSDL3_BUILD_DIR "${CxxKitWrapSDL3_ROOT_DIR}/build" CACHE INTERNAL "" FORCE)
set(CxxKitWrapSDL3_SOURCE_DIR "${CxxKitWrapSDL3_ROOT_DIR}/source" CACHE INTERNAL "" FORCE)
set(CxxKitWrapSDL3_INSTALL_DIR "${CxxKitWrapSDL3_ROOT_DIR}/install" CACHE INTERNAL "" FORCE)
cxxkit_stamp_file_info(CxxKitWrapSDL3 OUTPUT_DIR "${CxxKitWrapSDL3_ROOT_DIR}")
cxxkit_fetch_3rdparty(CxxKitWrapSDL3 URL "${CxxKitWrapSDL3_URL_PATH}" OUTPUT_NAME "${CxxKitWrapSDL3_DIR_NAME}")
if(NOT EXISTS "${CxxKitWrapSDL3_STAMP_FILE_PATH}")
    if(NOT EXISTS ${CxxKitWrapSDL3_SOURCE_DIR})
        message(FATAL_ERROR "${CxxKitWrapSDL3_NAME} FetchContent failed.")
    endif()
    cxxkit_reset_dir(${CxxKitWrapSDL3_BUILD_DIR})

    # SDL3 FATALs at configure when neither X11 nor Wayland dev packages exist (headless build
    # boxes). Official escape hatch: console build (dummy video). Probe once, don't force-disable
    # anything when dev packages ARE present (auto-detect stays on).
    find_package(X11 QUIET)
    find_package(PkgConfig QUIET)
    set(_wl_rc 1)
    if(PKG_CONFIG_FOUND)
        execute_process(COMMAND pkg-config --exists wayland-client RESULT_VARIABLE _wl_rc OUTPUT_QUIET ERROR_QUIET)
    endif()
    set(_sdl3_extra_flags "")
    if(NOT X11_FOUND AND NOT _wl_rc EQUAL 0)
        list(APPEND _sdl3_extra_flags -DSDL_UNIX_CONSOLE_BUILD=ON)
        message(STATUS "${CxxKitWrapSDL3_NAME}: no X11/Wayland dev, using SDL_UNIX_CONSOLE_BUILD (dummy video)")
    endif()
    message(STATUS "Configure ${CxxKitWrapSDL3_NAME} lib...")
    execute_process(
        COMMAND ${CMAKE_COMMAND}
        -Wno-deprecated
        --no-warn-unused-cli
        -G ${CMAKE_GENERATOR}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX=${CxxKitWrapSDL3_INSTALL_DIR}
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5
        -DSDL_SHARED=OFF
        -DSDL_STATIC=ON
        # PIC is mandatory: this static lib gets linked into libcxxkit_imgui_sdl3.so in the
        # shared build (check.sh 5/8) — without it the link dies on R_X86_64_PC32 relocations.
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DSDL_TEST_LIBRARY=OFF
        -DSDL_TESTS=OFF
        ${_sdl3_extra_flags}
        ${CxxKitWrapSDL3_SOURCE_DIR}
        WORKING_DIRECTORY "${CxxKitWrapSDL3_BUILD_DIR}"
        RESULT_VARIABLE CONFIGURE_RESULT)
    if(NOT CONFIGURE_RESULT MATCHES 0)
        message(FATAL_ERROR "${CxxKitWrapSDL3_NAME} configure failed.")
    endif()

    message(STATUS "${CxxKitWrapSDL3_NAME} configure success")
    execute_process(
        COMMAND ${CMAKE_COMMAND} --build ./
        --parallel ${CXXKIT_NUMBER_OF_ASYNC_JOBS}
        --config ${CMAKE_BUILD_TYPE} --target install
        WORKING_DIRECTORY "${CxxKitWrapSDL3_BUILD_DIR}"
        RESULT_VARIABLE BUILD_RESULT)
    if(NOT BUILD_RESULT MATCHES 0)
        message(FATAL_ERROR "${CxxKitWrapSDL3_NAME} build failed.")
    endif()
    message(STATUS "${CxxKitWrapSDL3_NAME} build success")

    cxxkit_make_stamp_file("${CxxKitWrapSDL3_STAMP_FILE_PATH}")
endif()

# wrap lib
add_library(CxxKitWrapSDL3::WrapSDL3 STATIC IMPORTED)
if(WIN32)
    set(CxxKitWrapSDL3_LIBRARY SDL3.lib)
else()
    set(CxxKitWrapSDL3_LIBRARY libSDL3.a)
endif()
set_target_properties(CxxKitWrapSDL3::WrapSDL3 PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES
    ${CxxKitWrapSDL3_INSTALL_DIR}/include
    IMPORTED_LOCATION
    ${CxxKitWrapSDL3_INSTALL_DIR}/lib/${CxxKitWrapSDL3_LIBRARY})
# Static SDL3 needs the C runtime support libs it itself links (m/dl/pthread on glibc Linux).
if(UNIX AND NOT APPLE)
    target_link_libraries(CxxKitWrapSDL3::WrapSDL3 INTERFACE m dl pthread)
endif()
set(CxxKitWrapSDL3_FOUND ON)
