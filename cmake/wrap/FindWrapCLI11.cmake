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

# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET CXXKitWrapCLI11::WrapCLI11)
    set(CXXKitWrapCLI11_FOUND ON)
    return()
endif()

set(CXXKitWrapCLI11_NAME "CLI11-2.6.2")
set(CXXKitWrapCLI11_PKG_NAME "${CXXKitWrapCLI11_NAME}.tar.gz")
set(CXXKitWrapCLI11_DIR_NAME "${CXXKitWrapCLI11_NAME}-${CXXKIT_LOWER_BUILD_TYPE}")
set(CXXKitWrapCLI11_URL_PATH "${PROJECT_SOURCE_DIR}/3rdparty/${CXXKitWrapCLI11_PKG_NAME}")
set(CXXKitWrapCLI11_ROOT_DIR "${PROJECT_BINARY_DIR}/3rdparty/${CXXKitWrapCLI11_DIR_NAME}")
set(CXXKitWrapCLI11_BUILD_DIR "${CXXKitWrapCLI11_ROOT_DIR}/build" CACHE INTERNAL "" FORCE)
set(CXXKitWrapCLI11_SOURCE_DIR "${CXXKitWrapCLI11_ROOT_DIR}/source" CACHE INTERNAL "" FORCE)
set(CXXKitWrapCLI11_INSTALL_DIR "${CXXKitWrapCLI11_ROOT_DIR}/install" CACHE INTERNAL "" FORCE)
cxxkit_stamp_file_info(CXXKitWrapCLI11 OUTPUT_DIR "${CXXKitWrapCLI11_ROOT_DIR}")
cxxkit_fetch_3rdparty(CXXKitWrapCLI11 URL "${CXXKitWrapCLI11_URL_PATH}" OUTPUT_NAME "${CXXKitWrapCLI11_DIR_NAME}")
if(NOT EXISTS "${CXXKitWrapCLI11_STAMP_FILE_PATH}")
    if(NOT EXISTS ${CXXKitWrapCLI11_SOURCE_DIR})
        message(FATAL_ERROR "${CXXKitWrapCLI11_NAME} FetchContent failed.")
    endif()
    cxxkit_reset_dir(${CXXKitWrapCLI11_BUILD_DIR})

    message(STATUS "Configure ${CXXKitWrapCLI11_NAME} lib...")
    execute_process(
        COMMAND ${CMAKE_COMMAND}
        -Wno-deprecated
        --no-warn-unused-cli
        -G ${CMAKE_GENERATOR}
        -DCLI11_BUILD_DOCS=OFF
        -DCLI11_BUILD_TESTS=OFF
        -DCLI11_BUILD_EXAMPLES=OFF
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DCMAKE_INSTALL_PREFIX=${CXXKitWrapCLI11_INSTALL_DIR}
        ${CXXKitWrapCLI11_SOURCE_DIR}
        WORKING_DIRECTORY "${CXXKitWrapCLI11_BUILD_DIR}"
        RESULT_VARIABLE CONFIGURE_RESULT)
    if(NOT CONFIGURE_RESULT MATCHES 0)
        message(FATAL_ERROR "${CXXKitWrapCLI11_NAME} configure failed.")
    endif()
    message(STATUS "${CXXKitWrapCLI11_NAME} configure success")

    execute_process(
        COMMAND ${CMAKE_COMMAND} --build ./ --parallel ${CXXKIT_NUMBER_OF_ASYNC_JOBS} --config Release --target install
        WORKING_DIRECTORY "${CXXKitWrapCLI11_BUILD_DIR}"
        RESULT_VARIABLE BUILD_RESULT)
    if(NOT BUILD_RESULT MATCHES 0)
        message(FATAL_ERROR "${CXXKitWrapCLI11_NAME} build failed.")
    endif()
    message(STATUS "${CXXKitWrapCLI11_NAME} build success")

    execute_process(
        COMMAND ${CMAKE_COMMAND} --install ./ --config ${CMAKE_BUILD_TYPE}
        WORKING_DIRECTORY "${CXXKitWrapCLI11_BUILD_DIR}"
        RESULT_VARIABLE INSTALL_RESULT)
    if(NOT INSTALL_RESULT MATCHES 0)
        message(FATAL_ERROR "${CXXKitWrapCLI11_NAME} install failed.")
    endif()
    message(STATUS "${CXXKitWrapCLI11_NAME} install success")
    cxxkit_make_stamp_file("${CXXKitWrapCLI11_STAMP_FILE_PATH}")
endif()
# wrap lib
add_library(CXXKitWrapCLI11::WrapCLI11 INTERFACE IMPORTED)
find_package(CLI11 PATHS ${CXXKitWrapCLI11_INSTALL_DIR} NO_DEFAULT_PATH REQUIRED)
target_link_libraries(CXXKitWrapCLI11::WrapCLI11 INTERFACE CLI11::CLI11)
set(CXXKitWrapCLI11_FOUND ON)