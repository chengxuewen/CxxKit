########################################################################################################################
#
# Library: CXXKIT
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
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  AUTHORS
# OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
########################################################################################################################

# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET CxxKitWrapLibyuv::WrapLibyuv)
    set(CxxKitWrapLibyuv_FOUND ON)
    return()
endif()

# in arm64/aarch64-linux, need gcc10!
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
    if(NOT EXISTS ${CxxKitWrapLibyuv_SOURCE_DIR})
        message(FATAL_ERROR "${CxxKitWrapLibyuv_NAME} FetchContent failed.")
    endif()
    cxxkit_reset_dir(${CxxKitWrapLibyuv_BUILD_DIR})

    message(STATUS "Configure ${CxxKitWrapLibyuv_NAME} lib...")
    execute_process(
        COMMAND ${CMAKE_COMMAND}
        -Wno-deprecated
        --no-warn-unused-cli
        -G ${CMAKE_GENERATOR}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX=${CxxKitWrapLibyuv_INSTALL_DIR}
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5
        ${CxxKitWrapLibyuv_SOURCE_DIR}
        WORKING_DIRECTORY "${CxxKitWrapLibyuv_BUILD_DIR}"
        RESULT_VARIABLE CONFIGURE_RESULT)
    if(NOT CONFIGURE_RESULT MATCHES 0)
        message(FATAL_ERROR "${CxxKitWrapLibyuv_NAME} configure failed.")
    endif()

    message(STATUS "${CxxKitWrapLibyuv_NAME} configure success")
    execute_process(
        COMMAND ${CMAKE_COMMAND} --build ./ 
        --parallel ${CXXKIT_NUMBER_OF_ASYNC_JOBS}
        --config ${CMAKE_BUILD_TYPE} --target install
        WORKING_DIRECTORY "${CxxKitWrapLibyuv_BUILD_DIR}"
        RESULT_VARIABLE BUILD_RESULT)
    if(NOT BUILD_RESULT MATCHES 0)
        message(FATAL_ERROR "${CxxKitWrapLibyuv_NAME} build failed.")
    endif()
    message(STATUS "${CxxKitWrapLibyuv_NAME} build success")

    execute_process(
        COMMAND ${CMAKE_COMMAND} --install ./ --config ${CMAKE_BUILD_TYPE}
        WORKING_DIRECTORY "${CxxKitWrapLibyuv_BUILD_DIR}"
        RESULT_VARIABLE INSTALL_RESULT)
    if(NOT INSTALL_RESULT MATCHES 0)
        message(FATAL_ERROR "${CxxKitWrapLibyuv_NAME} install failed.")
    endif()
    message(STATUS "${CxxKitWrapLibyuv_NAME} install success")
    cxxkit_make_stamp_file("${CxxKitWrapLibyuv_STAMP_FILE_PATH}")
endif()
# wrap lib
add_library(CxxKitWrapLibyuv::WrapLibyuv STATIC IMPORTED)
if(WIN32)
    set(CxxKitWrapLibyuv_LIBRARY yuv.lib)
else()
    set(CxxKitWrapLibyuv_LIBRARY libyuv.a)
endif()
set_target_properties(CxxKitWrapLibyuv::WrapLibyuv PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES
    ${CxxKitWrapLibyuv_INSTALL_DIR}/include
    IMPORTED_LOCATION
    ${CxxKitWrapLibyuv_INSTALL_DIR}/lib/${CxxKitWrapLibyuv_LIBRARY})
find_package(JPEG)
if(JPEG_FOUND)
    target_link_libraries(CxxKitWrapLibyuv::WrapLibyuv INTERFACE ${JPEG_LIBRARY})
endif()
if(NOT EXISTS "${CXXKIT_BUILD_DIR}/third_party/libyuv/include/libyuv")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CxxKitWrapLibyuv_INSTALL_DIR}/include"
        "${CXXKIT_BUILD_DIR}/third_party/libyuv/include"
        WORKING_DIRECTORY "${CxxKitWrapLibyuv_ROOT_DIR}"
        ERROR_QUIET)
endif()
set(CxxKitWrapLibyuv_FOUND ON)