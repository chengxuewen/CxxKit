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
if(TARGET CXXKitWrapMbedTLS::WrapMbedTLS)
    set(CXXKitWrapMbedTLS_FOUND ON)
    return()
endif()

set(CXXKitWrapMbedTLS_DIR_NAME "mbedtls-3.6.2-${CXXKIT_LOWER_BUILD_TYPE}")
set(CXXKitWrapMbedTLS_PKG_NAME "mbedtls-3.6.2.zip")
set(CXXKitWrapMbedTLS_URL_PATH "${PROJECT_SOURCE_DIR}/3rdparty/${CXXKitWrapMbedTLS_PKG_NAME}")
set(CXXKitWrapMbedTLS_ROOT_DIR "${PROJECT_BINARY_DIR}/3rdparty/${CXXKitWrapMbedTLS_DIR_NAME}")
set(CXXKitWrapMbedTLS_BUILD_DIR "${CXXKitWrapMbedTLS_ROOT_DIR}/build" CACHE INTERNAL "" FORCE)
set(CXXKitWrapMbedTLS_SOURCE_DIR "${CXXKitWrapMbedTLS_ROOT_DIR}/source" CACHE INTERNAL "" FORCE)
set(CXXKitWrapMbedTLS_INSTALL_DIR "${CXXKitWrapMbedTLS_ROOT_DIR}/install" CACHE INTERNAL "" FORCE)
cxxkit_stamp_file_info(CXXKitWrapMbedTLS OUTPUT_DIR "${CXXKitWrapMbedTLS_ROOT_DIR}")
cxxkit_fetch_3rdparty(CXXKitWrapMbedTLS URL "${CXXKitWrapMbedTLS_URL_PATH}" OUTPUT_NAME "${CXXKitWrapMbedTLS_DIR_NAME}")
if(NOT EXISTS "${CXXKitWrapMbedTLS_STAMP_FILE_PATH}")
    if(NOT EXISTS ${CXXKitWrapMbedTLS_SOURCE_DIR})
        message(FATAL_ERROR "${CXXKitWrapMbedTLS_DIR_NAME} FetchContent failed.")
    endif()
    cxxkit_reset_dir(${CXXKitWrapMbedTLS_BUILD_DIR})

    message(STATUS "Configure ${CXXKitWrapMbedTLS_DIR_NAME} lib...")
    execute_process(
        COMMAND ${CMAKE_COMMAND}
        -Wno-deprecated
        --no-warn-unused-cli
        -G ${CMAKE_GENERATOR}
        -DENABLE_PROGRAMS=OFF
        -DMBEDTLS_AS_SUBPROJECT=OFF
        -DMBEDTLS_FATAL_WARNINGS=OFF
        -DCMAKE_C_FLAGS="-fPIC"
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX=${CXXKitWrapMbedTLS_INSTALL_DIR}
        ${CXXKitWrapMbedTLS_SOURCE_DIR}
        WORKING_DIRECTORY "${CXXKitWrapMbedTLS_BUILD_DIR}"
        RESULT_VARIABLE CONFIGURE_RESULT)
    if(NOT CONFIGURE_RESULT MATCHES 0)
        message(FATAL_ERROR "${CXXKitWrapMbedTLS_DIR_NAME} configure failed.")
    endif()
    message(STATUS "${CXXKitWrapMbedTLS_DIR_NAME} configure success")

    execute_process(
        COMMAND ${CMAKE_COMMAND} --build ./ --parallel ${CXXKIT_NUMBER_OF_ASYNC_JOBS}
        --config ${CMAKE_BUILD_TYPE} --target install
        WORKING_DIRECTORY "${CXXKitWrapMbedTLS_BUILD_DIR}"
        RESULT_VARIABLE BUILD_RESULT)
    if(NOT BUILD_RESULT MATCHES 0)
        message(FATAL_ERROR "${CXXKitWrapMbedTLS_DIR_NAME} build failed.")
    endif()
    message(STATUS "${CXXKitWrapMbedTLS_DIR_NAME} build success")

    execute_process(
        COMMAND ${CMAKE_COMMAND} --install ./ --config ${CMAKE_BUILD_TYPE}
        WORKING_DIRECTORY "${CXXKitWrapMbedTLS_BUILD_DIR}"
        RESULT_VARIABLE INSTALL_RESULT)
    if(NOT INSTALL_RESULT MATCHES 0)
        message(FATAL_ERROR "${CXXKitWrapMbedTLS_DIR_NAME} install failed.")
    endif()
    message(STATUS "${CXXKitWrapMbedTLS_DIR_NAME} install success")
    cxxkit_make_stamp_file("${CXXKitWrapMbedTLS_STAMP_FILE_PATH}")
endif()
# wrap lib
add_library(CXXKitWrapMbedTLS::WrapMbedTLS INTERFACE IMPORTED)
find_package(MbedTLS PATHS ${CXXKitWrapMbedTLS_INSTALL_DIR} NO_DEFAULT_PATH REQUIRED)
target_link_libraries(CXXKitWrapMbedTLS::WrapMbedTLS INTERFACE MbedTLS::mbedtls)
set(CXXKitWrapMbedTLS_FOUND ON)
