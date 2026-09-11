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
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
# OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
########################################################################################################################

cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

cxxkit_configure_library_begin(CxxKitKernel)

cxxkit_find_package(Fmt PROVIDED_TARGETS CxxKitWrapFmt::WrapFmt)
# Default loop backend (D38): the kernel carries the libuv engine privately (cxxkit/kernel/uv/detail/,
# QtCore model — Core links glib but its public API says nothing about it). Conditional on the switch;
# when OFF, make_default_dispatcher() takes the fatal no-backend branch (default_dispatcher.cpp always compiles).
set(_cxxkit_kernel_uv_sources "")
set(_cxxkit_kernel_uv_libs "")
if(CXXKIT_ENABLE_LOOP_BACKEND_UV)
    cxxkit_find_package(Libuv PROVIDED_TARGETS CxxKitWrapLibuv::WrapLibuv)
    list(APPEND _cxxkit_kernel_uv_sources uv/detail/uv_event_dispatcher.cpp)
    list(APPEND _cxxkit_kernel_uv_libs CxxKitWrapLibuv::WrapLibuv)
endif()
cxxkit_add_library(cxxkit_kernel
    SOURCES application.cpp event.cpp event_loop.cpp object.cpp signals.cpp default_dispatcher.cpp ${_cxxkit_kernel_uv_sources}
    PUBLIC_LIBRARIES cxxkit::base cxxkit::memory cxxkit::tools CxxKitWrapFmt::WrapFmt ${_cxxkit_kernel_uv_libs}
    EXPORT_NAME kernel
    FOLDER "CxxKit/libs")
# The switch must reach the preprocessor: default_dispatcher.cpp's #if branch and the uv/detail
# engine include depend on it (core_config.hpp does not carry module switches).
if(CXXKIT_ENABLE_LOOP_BACKEND_UV)
    target_compile_definitions(cxxkit_kernel PUBLIC CXXKIT_ENABLE_LOOP_BACKEND_UV=1)
endif()
cxxkit_generate_pkg_config(cxxkit_kernel kernel)
cxxkit_configure_library_end(CxxKitKernel)

if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_kernel EXPORT cxxkitTargets)
    # Private engine headers stay out of the install tree (Momus F-1): kernel/uv/detail/ holds the
    # UvEventDispatcherPrivate pimpl and the engine's own header — public consumers go through
    # default_dispatcher.hpp / EventLoop only.
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/kernel/
        DESTINATION include/cxxkit/kernel
        FILES_MATCHING PATTERN "*.hpp"
        PATTERN "uv/detail" EXCLUDE)
endif()

# Vendored libuv install (four-piece, D6 namespace path <cxxkit/3rdparty/libuv/uv.h>) — moved from the
# dissolved cxxkit_uv sublibrary (D38). Kept here so the install-tree layout is IDENTICAL to the pre-move
# layout (consumers unchanged). Condition covers both engine carriers: kernel builds the engine when the
# backend switch is ON; network needs the static lib + headers whenever it ships TcpSocket/TcpServer
# (Momus F-2: NETWORK=ON + backend=OFF must still install libuv).
# NOT cxxkit_install_public_wrap_headers: B1 stages the D6 tree inside the wrap install at
# include/cxxkit/3rdparty/libuv/ — the generic helper would copy wrap-include CONTENTS into
# include/cxxkit/3rdparty/, double-nesting to include/cxxkit/3rdparty/cxxkit/3rdparty/libuv
# and leaking the un-namespaced uv.h/uv/ (fmt-style flat layout assumption does not hold for
# libuv whose upstream install root is uv.h + uv/). Install the D6 subtree + static lib +
# libuv's own Config (find_dependency chain) explicitly instead.
if(CXXKIT_ENABLE_LOOP_BACKEND_UV OR CXXKIT_ENABLE_LIB_NETWORK)
    cxxkit_find_package(Libuv PROVIDED_TARGETS CxxKitWrapLibuv::WrapLibuv)
    if(CXXKIT_BUILD_INSTALL)
        # Build-tree staging (consumers' <cxxkit/3rdparty/libuv/uv.h> also resolves via
        # ${PROJECT_BINARY_DIR}/include on the wrap target, keep both faces identical):
        file(COPY "${CxxKitWrapLibuv_INSTALL_DIR}/include/cxxkit/3rdparty/libuv/"
            DESTINATION "${PROJECT_BINARY_DIR}/include/cxxkit/3rdparty/libuv")
        # Install-tree D6 headers:
        install(DIRECTORY "${CxxKitWrapLibuv_INSTALL_DIR}/include/cxxkit/3rdparty/libuv/"
            DESTINATION "include/cxxkit/3rdparty/libuv"
            FILES_MATCHING
            PATTERN "*.h")
        # Static lib ships with the package (M3):
        install(FILES "${CxxKitWrapLibuv_INSTALL_DIR}/lib/libuv.a"
            DESTINATION lib)
        # vcpkg-style: libuv's own Config.cmake for find_dependency(libuv) (D11):
        install(DIRECTORY "${CxxKitWrapLibuv_INSTALL_DIR}/lib/cmake/libuv/"
            DESTINATION "lib/cmake/libuv")
        # libuv's .pc (vendored absolute prefix — same caveat as every other vendored .pc):
        install(FILES "${CxxKitWrapLibuv_INSTALL_DIR}/lib/pkgconfig/libuv-static.pc"
            DESTINATION "lib/pkgconfig")
    endif()
endif()
