########################################################################################################################
#
# Library: CxxKit
#
# Copyright (C) 2025~Present ChengXueWen.
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
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
# OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
########################################################################################################################

# FindWrapBackward — backward-cpp stack printer via cxxkit_vcpkg_install_package (QExt mechanism, one wrap per package).
#
# Consumes the standard backward-cpp-<triplet>.7z: cache present → unpack + find_package(Backward); missing →
# OpenCTK-style auto fallback (bootstrap vcpkg, install, export, pack). Header-only library — no PIC/link smoke.
#
# On Linux, backward dlopens "libdw.so.1" at runtime, so elfutils + libunwind are installed as runtime payloads
# (NOT_IMPORT, .so shipped by the consumer's install rules; the crash sublibrary copies them next to itself via
# POST_BUILD). Their unpack roots are exposed as CXXKitWrapBackward_ElfUtils_INSTALL_DIR /
# CXXKitWrapBackward_LibUnwind_INSTALL_DIR for the consumer's install/copy rules.
#
# Provides: CXXKitWrapBackward::WrapBackward → Backward::Interface
# Gate: relocatability (resolved -I/-L inside the unpack root).

# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET CXXKitWrapBackward::WrapBackward)
    set(CXXKitWrapBackward_FOUND ON)
    return()
endif()

cxxkit_vcpkg_install_package(backward-cpp
    TARGET CXXKitWrapBackward::WrapBackward   # helper creates the empty INTERFACE IMPORTED target (QExt style)
    PREFIX CXXKitWrapBackward
    PACK_NAME backward-cpp
    NOT_IMPORT)

set(_cxxkit_wrap_backward_install_dir "${CXXKitWrapBackward_INSTALL_DIR}")
    TARGET CXXKitWrapBackward_Unpack
    PREFIX CXXKitWrapBackward
    PACK_NAME backward-cpp
    NOT_IMPORT)

set(_cxxkit_wrap_backward_install_dir "${CXXKitWrapBackward_INSTALL_DIR}")

# Runtime payloads for backward's dlopen("libdw.so.1") on Linux.
if(NOT APPLE AND NOT WIN32)
    cxxkit_vcpkg_install_package(elfutils
        TARGET CXXKitWrapBackward_ElfUtilsUnpack
        PREFIX CXXKitWrapBackward_ElfUtils
        PACK_NAME elfutils
        NOT_IMPORT)
    cxxkit_vcpkg_install_package(libunwind
        TARGET CXXKitWrapBackward_LibUnwindUnpack
        PREFIX CXXKitWrapBackward_LibUnwind
        PACK_NAME libunwind
        NOT_IMPORT)
endif()

find_package(Backward PATHS "${_cxxkit_wrap_backward_install_dir}/share/Backward"
    NO_DEFAULT_PATH REQUIRED)

target_link_libraries(CXXKitWrapBackward::WrapBackward INTERFACE Backward::Interface)
target_include_directories(CXXKitWrapBackward::WrapBackward INTERFACE "${_cxxkit_wrap_backward_install_dir}/include")
set_target_properties(CXXKitWrapBackward::WrapBackward PROPERTIES FOLDER "CxxKit/3rdparty")

# ---- relocatability gate -----------------------------------------------------------------------------------------
get_target_property(_cxxkit_wrap_backward_includes CXXKitWrapBackward::WrapBackward INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(_cxxkit_wrap_backward_links    CXXKitWrapBackward::WrapBackward INTERFACE_LINK_LIBRARIES)
foreach(_cxxkit_wrap_backward_inc IN LISTS _cxxkit_wrap_backward_includes)
    if(NOT _cxxkit_wrap_backward_inc STREQUAL "" AND NOT "${_cxxkit_wrap_backward_inc}" MATCHES "^${CXXKitWrapBackward_ROOT_DIR}")
        message(FATAL_ERROR "crash-deps relocatability gate failed: include '${_cxxkit_wrap_backward_inc}' "
            "lies outside the unpack root '${CXXKitWrapBackward_ROOT_DIR}'.")
    endif()
endforeach()
foreach(_cxxkit_wrap_backward_link IN LISTS _cxxkit_wrap_backward_links)
    if("${_cxxkit_wrap_backward_link}" MATCHES "^(/|\\\\)" AND NOT "${_cxxkit_wrap_backward_link}" MATCHES "^${CXXKitWrapBackward_ROOT_DIR}")
        message(FATAL_ERROR "crash-deps relocatability gate failed: absolute link path "
            "'${_cxxkit_wrap_backward_link}' lies outside the unpack root '${CXXKitWrapBackward_ROOT_DIR}'.")
    endif()
endforeach()
message(STATUS "crash-deps relocatability gate passed (backward-cpp)")

set(CXXKitWrapBackward_FOUND ON)