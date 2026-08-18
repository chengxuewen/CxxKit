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
if(TARGET CXXKitWrapFunction2::WrapFunction2)
    set(CXXKitWrapFunction2_FOUND ON)
    return()
endif()

set(CXXKitWrapFunction2_NAME "function2-4.2.5")
set(CXXKitWrapFunction2_PKG_NAME "${CXXKitWrapFunction2_NAME}.tar.gz")
set(CXXKitWrapFunction2_DIR_NAME "${CXXKitWrapFunction2_NAME}-${CXXKIT_LOWER_BUILD_TYPE}")
set(CXXKitWrapFunction2_URL_PATH "${PROJECT_SOURCE_DIR}/3rdparty/${CXXKitWrapFunction2_PKG_NAME}")
set(CXXKitWrapFunction2_ROOT_DIR "${PROJECT_BINARY_DIR}/3rdparty/${CXXKitWrapFunction2_DIR_NAME}")
set(CXXKitWrapFunction2_BUILD_DIR "${CXXKitWrapFunction2_ROOT_DIR}/build" CACHE INTERNAL "" FORCE)
set(CXXKitWrapFunction2_SOURCE_DIR "${CXXKitWrapFunction2_ROOT_DIR}/source" CACHE INTERNAL "" FORCE)
set(CXXKitWrapFunction2_INSTALL_DIR "${CXXKitWrapFunction2_ROOT_DIR}/install" CACHE INTERNAL "" FORCE)
cxxkit_stamp_file_info(CXXKitWrapFunction2 OUTPUT_DIR "${CXXKitWrapFunction2_ROOT_DIR}")
cxxkit_fetch_3rdparty(CXXKitWrapFunction2 URL "${CXXKitWrapFunction2_URL_PATH}" OUTPUT_NAME "${CXXKitWrapFunction2_DIR_NAME}")
if(NOT EXISTS "${CXXKitWrapFunction2_STAMP_FILE_PATH}")
    if(NOT EXISTS ${CXXKitWrapFunction2_SOURCE_DIR})
        message(FATAL_ERROR "${CXXKitWrapFunction2_NAME} FetchContent failed.")
    endif()
    cxxkit_reset_dir(${CXXKitWrapFunction2_BUILD_DIR})

    message(STATUS "Configure ${CXXKitWrapFunction2_NAME} lib...")
    execute_process(
        COMMAND ${CMAKE_COMMAND}
        -Wno-deprecated
        --no-warn-unused-cli
        -G ${CMAKE_GENERATOR}
        -DBUILD_TESTING=OFF
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX=${CXXKitWrapFunction2_INSTALL_DIR}
        ${CXXKitWrapFunction2_SOURCE_DIR}
        WORKING_DIRECTORY "${CXXKitWrapFunction2_BUILD_DIR}"
        RESULT_VARIABLE CONFIGURE_RESULT)
    if(NOT CONFIGURE_RESULT MATCHES 0)
        message(FATAL_ERROR "${CXXKitWrapFunction2_NAME} configure failed.")
    endif()
    message(STATUS "${CXXKitWrapFunction2_NAME} configure success")

    execute_process(
        COMMAND ${CMAKE_COMMAND} --build ./ --parallel ${CXXKIT_NUMBER_OF_ASYNC_JOBS} --config
        ${CMAKE_BUILD_TYPE} --target install
        WORKING_DIRECTORY "${CXXKitWrapFunction2_BUILD_DIR}"
        RESULT_VARIABLE BUILD_RESULT)
    if(NOT BUILD_RESULT MATCHES 0)
        message(FATAL_ERROR "${CXXKitWrapFunction2_NAME} build failed.")
    endif()
    message(STATUS "${CXXKitWrapFunction2_NAME} build success")

    execute_process(
        COMMAND ${CMAKE_COMMAND} --install ./ --config ${CMAKE_BUILD_TYPE}
        WORKING_DIRECTORY "${CXXKitWrapFunction2_BUILD_DIR}"
        RESULT_VARIABLE INSTALL_RESULT)
    if(NOT INSTALL_RESULT MATCHES 0)
        message(FATAL_ERROR "${CXXKitWrapFunction2_NAME} install failed.")
    endif()
    message(STATUS "${CXXKitWrapFunction2_NAME} install success")
    cxxkit_make_stamp_file("${CXXKitWrapFunction2_STAMP_FILE_PATH}")
endif()
# wrap lib
add_library(CXXKitWrapFunction2::WrapFunction2 INTERFACE IMPORTED)
set(function2_DIR "${CXXKitWrapFunction2_INSTALL_DIR}/lib/cmake/function2")
find_package(function2 PATHS "${CXXKitWrapFunction2_INSTALL_DIR}/lib" NO_DEFAULT_PATH REQUIRED)
target_link_libraries(CXXKitWrapFunction2::WrapFunction2 INTERFACE function2::function2)
set(CXXKitWrapFunction2_FOUND ON)
