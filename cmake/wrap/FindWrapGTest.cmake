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
if(TARGET CxxKitWrapGTest::WrapGTest)
    set(CxxKitWrapGTest_FOUND ON)
    return()
endif()

set(CxxKitWrapGTest_NAME "googletest-release-1.12.1")
set(CxxKitWrapGTest_PKG_NAME "${CxxKitWrapGTest_NAME}.tar.gz")
set(CxxKitWrapGTest_DIR_NAME "${CxxKitWrapGTest_NAME}-${CXXKIT_LOWER_BUILD_TYPE}")
set(CxxKitWrapGTest_URL_PATH "${PROJECT_SOURCE_DIR}/3rdparty/${CxxKitWrapGTest_PKG_NAME}")
set(CxxKitWrapGTest_ROOT_DIR "${PROJECT_BINARY_DIR}/3rdparty/${CxxKitWrapGTest_DIR_NAME}")
set(CxxKitWrapGTest_BUILD_DIR "${CxxKitWrapGTest_ROOT_DIR}/build" CACHE INTERNAL "" FORCE)
set(CxxKitWrapGTest_SOURCE_DIR "${CxxKitWrapGTest_ROOT_DIR}/source" CACHE INTERNAL "" FORCE)
set(CxxKitWrapGTest_INSTALL_DIR "${CxxKitWrapGTest_ROOT_DIR}/install" CACHE INTERNAL "" FORCE)
cxxkit_stamp_file_info(CxxKitWrapGTest OUTPUT_DIR "${CxxKitWrapGTest_ROOT_DIR}")
cxxkit_fetch_3rdparty(CxxKitWrapGTest URL "${CxxKitWrapGTest_URL_PATH}" OUTPUT_NAME "${CxxKitWrapGTest_DIR_NAME}")
if(NOT EXISTS "${CxxKitWrapGTest_STAMP_FILE_PATH}")
    if(NOT EXISTS ${CxxKitWrapGTest_SOURCE_DIR})
        message(FATAL_ERROR "${CxxKitWrapGTest_DIR_NAME} FetchContent failed.")
    endif()
    cxxkit_reset_dir(${CxxKitWrapGTest_BUILD_DIR})

    message(STATUS "Configure ${CxxKitWrapGTest_DIR_NAME} lib...")
    execute_process(
        COMMAND ${CMAKE_COMMAND}
        -Wno-deprecated
        --no-warn-unused-cli
        -G ${CMAKE_GENERATOR}
        -DBUILD_SHARED_LIBS=OFF
        -Dgtest_force_shared_crt=ON
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX=${CxxKitWrapGTest_INSTALL_DIR}
        ${CxxKitWrapGTest_SOURCE_DIR}
        WORKING_DIRECTORY "${CxxKitWrapGTest_BUILD_DIR}"
        RESULT_VARIABLE CONFIGURE_RESULT)
    if(NOT CONFIGURE_RESULT MATCHES 0)
        message(FATAL_ERROR "${CxxKitWrapGTest_DIR_NAME} configure failed.")
    endif()
    message(STATUS "${CxxKitWrapGTest_DIR_NAME} configure success")

    execute_process(
        COMMAND ${CMAKE_COMMAND} --build ./ --parallel ${CXXKIT_NUMBER_OF_ASYNC_JOBS} --config 
        ${CMAKE_BUILD_TYPE} --target install
        WORKING_DIRECTORY "${CxxKitWrapGTest_BUILD_DIR}"
        RESULT_VARIABLE BUILD_RESULT)
    if(NOT BUILD_RESULT MATCHES 0)
        message(FATAL_ERROR "${CxxKitWrapGTest_DIR_NAME} build failed.")
    endif()
    message(STATUS "${CxxKitWrapGTest_DIR_NAME} build success")

    execute_process(
        COMMAND ${CMAKE_COMMAND} --install ./ --config ${CMAKE_BUILD_TYPE}
        WORKING_DIRECTORY "${CxxKitWrapGTest_BUILD_DIR}"
        RESULT_VARIABLE INSTALL_RESULT)
    if(NOT INSTALL_RESULT MATCHES 0)
        message(FATAL_ERROR "${CxxKitWrapGTest_DIR_NAME} install failed.")
    endif()
    message(STATUS "${CxxKitWrapGTest_DIR_NAME} install success")
    cxxkit_make_stamp_file("${CxxKitWrapGTest_STAMP_FILE_PATH}")
endif()
# wrap lib
add_library(CxxKitWrapGTest::WrapGTest INTERFACE IMPORTED)
find_package(GTest PATHS ${CxxKitWrapGTest_INSTALL_DIR} NO_DEFAULT_PATH REQUIRED)
target_link_libraries(CxxKitWrapGTest::WrapGTest INTERFACE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)
set(CxxKitWrapGTest_FOUND ON)