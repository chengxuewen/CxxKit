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
if(TARGET CXXKitWrapExpected::WrapExpected)
    set(CXXKitWrapExpected_FOUND ON)
    return()
endif()

set(CXXKitWrapExpected_NAME "expected-1.2.0")
set(CXXKitWrapExpected_PKG_NAME "${CXXKitWrapExpected_NAME}.tar.gz")
set(CXXKitWrapExpected_DIR_NAME "${CXXKitWrapExpected_NAME}-${CXXKIT_LOWER_BUILD_TYPE}")
set(CXXKitWrapExpected_URL_PATH "${PROJECT_SOURCE_DIR}/3rdparty/${CXXKitWrapExpected_PKG_NAME}")
set(CXXKitWrapExpected_ROOT_DIR "${PROJECT_BINARY_DIR}/3rdparty/${CXXKitWrapExpected_DIR_NAME}")
set(CXXKitWrapExpected_BUILD_DIR "${CXXKitWrapExpected_ROOT_DIR}/build" CACHE INTERNAL "" FORCE)
set(CXXKitWrapExpected_SOURCE_DIR "${CXXKitWrapExpected_ROOT_DIR}/source" CACHE INTERNAL "" FORCE)
set(CXXKitWrapExpected_INSTALL_DIR "${CXXKitWrapExpected_ROOT_DIR}/install" CACHE INTERNAL "" FORCE)
cxxkit_stamp_file_info(CXXKitWrapExpected OUTPUT_DIR "${CXXKitWrapExpected_ROOT_DIR}")
cxxkit_fetch_3rdparty(CXXKitWrapExpected URL "${CXXKitWrapExpected_URL_PATH}" OUTPUT_NAME "${CXXKitWrapExpected_DIR_NAME}")
if(NOT EXISTS "${CXXKitWrapExpected_STAMP_FILE_PATH}")
    if(NOT EXISTS ${CXXKitWrapExpected_SOURCE_DIR})
        message(FATAL_ERROR "${CXXKitWrapExpected_NAME} FetchContent failed.")
    endif()
    cxxkit_reset_dir(${CXXKitWrapExpected_BUILD_DIR})

    message(STATUS "Configure ${CXXKitWrapExpected_NAME} lib...")
    execute_process(
        COMMAND ${CMAKE_COMMAND}
        -Wno-deprecated
        --no-warn-unused-cli
        -G ${CMAKE_GENERATOR}
        -DEXPECTED_BUILD_TESTS=OFF
        -DCMAKE_INSTALL_PREFIX=${CXXKitWrapExpected_INSTALL_DIR}
        ${CXXKitWrapExpected_SOURCE_DIR}
        WORKING_DIRECTORY "${CXXKitWrapExpected_BUILD_DIR}"
        RESULT_VARIABLE CONFIGURE_RESULT)
    if(NOT CONFIGURE_RESULT MATCHES 0)
        message(FATAL_ERROR "${CXXKitWrapExpected_NAME} configure failed.")
    endif()
    message(STATUS "${CXXKitWrapExpected_NAME} configure success")

    execute_process(
        COMMAND ${CMAKE_COMMAND} --build ./ --parallel ${CXXKIT_NUMBER_OF_ASYNC_JOBS} --config Release --target install
        WORKING_DIRECTORY "${CXXKitWrapExpected_BUILD_DIR}"
        RESULT_VARIABLE BUILD_RESULT)
    if(NOT BUILD_RESULT MATCHES 0)
        message(FATAL_ERROR "${CXXKitWrapExpected_NAME} build failed.")
    endif()
    message(STATUS "${CXXKitWrapExpected_NAME} build success")

    execute_process(
        COMMAND ${CMAKE_COMMAND} --install ./ --config ${CMAKE_BUILD_TYPE}
        WORKING_DIRECTORY "${CXXKitWrapExpected_BUILD_DIR}"
        RESULT_VARIABLE INSTALL_RESULT)
    if(NOT INSTALL_RESULT MATCHES 0)
        message(FATAL_ERROR "${CXXKitWrapExpected_NAME} install failed.")
    endif()
    message(STATUS "${CXXKitWrapExpected_NAME} install success")
    cxxkit_make_stamp_file("${CXXKitWrapExpected_STAMP_FILE_PATH}")
endif()
# wrap lib
add_library(CXXKitWrapExpected::WrapExpected INTERFACE IMPORTED)
find_package(tl-expected PATHS ${CXXKitWrapExpected_INSTALL_DIR} NO_DEFAULT_PATH REQUIRED)
target_link_libraries(CXXKitWrapExpected::WrapExpected INTERFACE tl::expected)
set(CXXKitWrapExpected_FOUND ON)