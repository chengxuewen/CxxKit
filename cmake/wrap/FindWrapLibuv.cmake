########################################################################################################################
#
# Library: CXXKIT
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
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  AUTHORS
# OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
########################################################################################################################

# Vendored libuv 1.49.2 (libyuv-style source chain, NOT the breakpad vcpkg chain):
# unpack the in-repo 7z, build the static lib with libuv's own CMakeLists, stage headers
# under install/include/cxxkit/3rdparty/libuv/ (D6 namespace path), then expose
# CxxKitWrapLibuv::WrapLibuv as a STATIC IMPORTED target.

# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET CxxKitWrapLibuv::WrapLibuv)
    set(CxxKitWrapLibuv_FOUND ON)
    return()
endif()

find_package(Threads REQUIRED)

set(CxxKitWrapLibuv_NAME "libuv")
set(CxxKitWrapLibuv_PKG_NAME "libuv-v1.49.2.7z")
set(CxxKitWrapLibuv_DIR_NAME "libuv-v1.49.2-${CXXKIT_LOWER_BUILD_TYPE}")
set(CxxKitWrapLibuv_URL_PATH "${PROJECT_SOURCE_DIR}/3rdparty/${CxxKitWrapLibuv_PKG_NAME}")
set(CxxKitWrapLibuv_ROOT_DIR "${PROJECT_BINARY_DIR}/3rdparty/${CxxKitWrapLibuv_DIR_NAME}")
set(CxxKitWrapLibuv_BUILD_DIR "${CxxKitWrapLibuv_ROOT_DIR}/build" CACHE INTERNAL "" FORCE)
set(CxxKitWrapLibuv_SOURCE_DIR "${CxxKitWrapLibuv_ROOT_DIR}/source" CACHE INTERNAL "" FORCE)
set(CxxKitWrapLibuv_INSTALL_DIR "${CxxKitWrapLibuv_ROOT_DIR}/install" CACHE INTERNAL "" FORCE)
cxxkit_stamp_file_info(CxxKitWrapLibuv OUTPUT_DIR "${CxxKitWrapLibuv_ROOT_DIR}")
cxxkit_fetch_3rdparty(CxxKitWrapLibuv URL "${CxxKitWrapLibuv_URL_PATH}" OUTPUT_NAME "${CxxKitWrapLibuv_DIR_NAME}")
if(NOT EXISTS "${CxxKitWrapLibuv_STAMP_FILE_PATH}")
    if(NOT EXISTS ${CxxKitWrapLibuv_SOURCE_DIR})
        message(FATAL_ERROR "${CxxKitWrapLibuv_NAME} FetchContent failed.")
    endif()
    cxxkit_reset_dir(${CxxKitWrapLibuv_BUILD_DIR})

    # libuv 1.49.2 option names (verified in its CMakeLists.txt): LIBUV_BUILD_SHARED,
    # LIBUV_BUILD_TESTS/BENCH are cmake_dependent_option gated on BUILD_TESTING AND
    # LIBUV_BUILD_SHARED AND being the root project — all false here, so tests/bench are
    # off automatically. We still pass the flags explicitly for self-documentation.
    message(STATUS "Configure ${CxxKitWrapLibuv_NAME} lib...")
    execute_process(
        COMMAND ${CMAKE_COMMAND}
        -Wno-deprecated
        --no-warn-unused-cli
        -G ${CMAKE_GENERATOR}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX=${CxxKitWrapLibuv_INSTALL_DIR}
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5
        -DBUILD_SHARED_LIBS=OFF
        -DLIBUV_BUILD_SHARED=OFF
        -DLIBUV_BUILD_TESTS=OFF
        -DLIBUV_BUILD_BENCH=OFF
        # PIC is mandatory: this static lib gets linked into libcxxkit_uv.so in the shared
        # build (check.sh 5/8) — without it the link dies on R_X86_64_PC32 relocations.
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        ${CxxKitWrapLibuv_SOURCE_DIR}
        WORKING_DIRECTORY "${CxxKitWrapLibuv_BUILD_DIR}"
        RESULT_VARIABLE CONFIGURE_RESULT)
    if(NOT CONFIGURE_RESULT MATCHES 0)
        message(FATAL_ERROR "${CxxKitWrapLibuv_NAME} configure failed.")
    endif()

    message(STATUS "${CxxKitWrapLibuv_NAME} configure success")
    execute_process(
        COMMAND ${CMAKE_COMMAND} --build ./
        --parallel ${CXXKIT_NUMBER_OF_ASYNC_JOBS}
        --config ${CMAKE_BUILD_TYPE} --target install
        WORKING_DIRECTORY "${CxxKitWrapLibuv_BUILD_DIR}"
        RESULT_VARIABLE BUILD_RESULT)
    if(NOT BUILD_RESULT MATCHES 0)
        message(FATAL_ERROR "${CxxKitWrapLibuv_NAME} build failed.")
    endif()
    message(STATUS "${CxxKitWrapLibuv_NAME} build success")

    # D6 namespace staging: libuv installs include/{uv.h,uv/} — re-stage under
    # <cxxkit/3rdparty/libuv/ so consumers use <cxxkit/3rdparty/libuv/uv.h>.
    file(MAKE_DIRECTORY "${CxxKitWrapLibuv_INSTALL_DIR}/include/cxxkit/3rdparty")
    file(COPY "${CxxKitWrapLibuv_INSTALL_DIR}/include/uv.h" "${CxxKitWrapLibuv_INSTALL_DIR}/include/uv"
        DESTINATION "${CxxKitWrapLibuv_INSTALL_DIR}/include/cxxkit/3rdparty/libuv")

    cxxkit_make_stamp_file("${CxxKitWrapLibuv_STAMP_FILE_PATH}")
endif()

# wrap lib — prefer libuv >=1.44's own libuvConfig.cmake (carries thread/dl transitives);
# fall back to a hand-rolled IMPORTED target with Threads/dl wired in.
find_package(libuv PATHS "${CxxKitWrapLibuv_INSTALL_DIR}/lib/cmake" NO_DEFAULT_PATH QUIET)
# libuv 1.49.2's installed config only exports libuv::uv_a (the libuv::libuv ALIAS is a
# build-tree-only add_library alias, not part of the install export). CMake forbids
# set_target_properties/target_include_directories on an ALIAS, so instead of aliasing we
# clone the config's properties onto our own IMPORTED target — uniform handling for both
# branches (PIT-34: IMPORTED targets are only mutable in their creating scope).
if(NOT TARGET CxxKitWrapLibuv::WrapLibuv)
    add_library(CxxKitWrapLibuv::WrapLibuv STATIC IMPORTED)
endif()
set_target_properties(CxxKitWrapLibuv::WrapLibuv PROPERTIES
    IMPORTED_LOCATION
    "${CxxKitWrapLibuv_INSTALL_DIR}/lib/$<IF:$<PLATFORM_ID:Windows>,uv_a.lib,libuv.a>"
    # D6 include root: headers live at <install>/include/cxxkit/3rdparty/libuv/ — expose the
    # include/ root so <cxxkit/3rdparty/libuv/uv.h> resolves (build + install faces).
    INTERFACE_INCLUDE_DIRECTORIES
    "$<BUILD_INTERFACE:${CxxKitWrapLibuv_INSTALL_DIR}/include>")
if(TARGET libuv::uv_a)
    # Config path: adopt libuv's transitive link libs verbatim (pthread;dl;rt today).
    get_target_property(_libuv_itf_libs libuv::uv_a INTERFACE_LINK_LIBRARIES)
    if(_libuv_itf_libs)
        set_target_properties(CxxKitWrapLibuv::WrapLibuv PROPERTIES
            INTERFACE_LINK_LIBRARIES "${_libuv_itf_libs}")
    endif()
else()
    # Hand-rolled path: wire the always-present transitives so consumers never miss them.
    set(CxxKitWrapLibuv_INTERFACE_LIBS Threads::Threads ${CMAKE_DL_LIBS})
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        find_library(CxxKitWrapLibuv_NSL_LIB nsl)
        find_library(CxxKitWrapLibuv_SOCKET_LIB socket)
        if(CxxKitWrapLibuv_NSL_LIB)
            list(APPEND CxxKitWrapLibuv_INTERFACE_LIBS ${CxxKitWrapLibuv_NSL_LIB})
        endif()
        if(CxxKitWrapLibuv_SOCKET_LIB)
            list(APPEND CxxKitWrapLibuv_INTERFACE_LIBS ${CxxKitWrapLibuv_SOCKET_LIB})
        endif()
    endif()
    set_target_properties(CxxKitWrapLibuv::WrapLibuv PROPERTIES
        INTERFACE_LINK_LIBRARIES "${CxxKitWrapLibuv_INTERFACE_LIBS}")
endif()

set(CxxKitWrapLibuv_FOUND ON)
