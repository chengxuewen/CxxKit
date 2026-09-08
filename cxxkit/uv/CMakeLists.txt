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
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
# THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
# CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
########################################################################################################################

# cxxkit::uv — event-loop driver sublibrary over vendored libuv (opt-in CXXKIT_ENABLE_LIB_UV).

cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

cxxkit_configure_library_begin(CxxKitUv)

cxxkit_find_package(Libuv PROVIDED_TARGETS CxxKitWrapLibuv::WrapLibuv)
cxxkit_add_library(cxxkit_uv
    SOURCES uv_event_dispatcher.cpp dispatcher_factory.cpp tcp_socket.cpp
    PUBLIC_LIBRARIES cxxkit::kernel cxxkit::tools CxxKitWrapLibuv::WrapLibuv
    EXPORT_NAME uv
    FOLDER "CxxKit/libs")
target_link_libraries(cxxkit_uv PUBLIC cxxkit::base)
cxxkit_generate_pkg_config(cxxkit_uv uv)
cxxkit_configure_library_end(CxxKitUv)

# Vendored libuv install (four-piece, D6 namespace path <cxxkit/3rdparty/libuv/uv.h>).
# NOT cxxkit_install_public_wrap_headers: B1 stages the D6 tree inside the wrap install at
# include/cxxkit/3rdparty/libuv/ — the generic helper would copy wrap-include CONTENTS into
# include/cxxkit/3rdparty/, double-nesting to include/cxxkit/3rdparty/cxxkit/3rdparty/libuv
# and leaking the un-namespaced uv.h/uv/ (fmt-style flat layout assumption does not hold for
# libuv whose upstream install root is uv.h + uv/). Install the D6 subtree + static lib +
# libuv's own Config (find_dependency chain) explicitly instead.
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

if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_uv EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/uv/
        DESTINATION include/cxxkit/uv
        FILES_MATCHING PATTERN "*.hpp")
endif()
