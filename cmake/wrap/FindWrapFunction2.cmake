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
if(TARGET CxxKitWrapFunction2::WrapFunction2)
    set(CxxKitWrapFunction2_FOUND ON)
    return()
endif()

set(CxxKitWrapFunction2_NAME "function2-4.2.5")
set(CxxKitWrapFunction2_PKG_NAME "${CxxKitWrapFunction2_NAME}.tar.gz")
set(CxxKitWrapFunction2_DIR_NAME "${CxxKitWrapFunction2_NAME}-${CXXKIT_LOWER_BUILD_TYPE}")
set(CxxKitWrapFunction2_URL_PATH "${PROJECT_SOURCE_DIR}/3rdparty/${CxxKitWrapFunction2_PKG_NAME}")
set(CxxKitWrapFunction2_ROOT_DIR "${PROJECT_BINARY_DIR}/3rdparty/${CxxKitWrapFunction2_DIR_NAME}")
set(CxxKitWrapFunction2_BUILD_DIR "${CxxKitWrapFunction2_ROOT_DIR}/build" CACHE INTERNAL "" FORCE)
set(CxxKitWrapFunction2_SOURCE_DIR "${CxxKitWrapFunction2_ROOT_DIR}/source" CACHE INTERNAL "" FORCE)
set(CxxKitWrapFunction2_INSTALL_DIR "${CxxKitWrapFunction2_ROOT_DIR}/install" CACHE INTERNAL "" FORCE)
cxxkit_stamp_file_info(CxxKitWrapFunction2 OUTPUT_DIR "${CxxKitWrapFunction2_ROOT_DIR}")
cxxkit_fetch_3rdparty(CxxKitWrapFunction2 URL "${CxxKitWrapFunction2_URL_PATH}" OUTPUT_NAME "${CxxKitWrapFunction2_DIR_NAME}")
if(NOT EXISTS "${CxxKitWrapFunction2_STAMP_FILE_PATH}")
    if(NOT EXISTS ${CxxKitWrapFunction2_SOURCE_DIR})
        message(FATAL_ERROR "${CxxKitWrapFunction2_NAME} FetchContent failed.")
    endif()
    cxxkit_reset_dir(${CxxKitWrapFunction2_BUILD_DIR})

    message(STATUS "Configure ${CxxKitWrapFunction2_NAME} lib...")
    execute_process(
        COMMAND ${CMAKE_COMMAND}
        -Wno-deprecated
        --no-warn-unused-cli
        -G ${CMAKE_GENERATOR}
        -DBUILD_TESTING=OFF
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX=${CxxKitWrapFunction2_INSTALL_DIR}
        ${CxxKitWrapFunction2_SOURCE_DIR}
        WORKING_DIRECTORY "${CxxKitWrapFunction2_BUILD_DIR}"
        RESULT_VARIABLE CONFIGURE_RESULT)
    if(NOT CONFIGURE_RESULT MATCHES 0)
        message(FATAL_ERROR "${CxxKitWrapFunction2_NAME} configure failed.")
    endif()
    message(STATUS "${CxxKitWrapFunction2_NAME} configure success")

    execute_process(
        COMMAND ${CMAKE_COMMAND} --build ./ --parallel ${CXXKIT_NUMBER_OF_ASYNC_JOBS} --config
        ${CMAKE_BUILD_TYPE} --target install
        WORKING_DIRECTORY "${CxxKitWrapFunction2_BUILD_DIR}"
        RESULT_VARIABLE BUILD_RESULT)
    if(NOT BUILD_RESULT MATCHES 0)
        message(FATAL_ERROR "${CxxKitWrapFunction2_NAME} build failed.")
    endif()
    message(STATUS "${CxxKitWrapFunction2_NAME} build success")

    execute_process(
        COMMAND ${CMAKE_COMMAND} --install ./ --config ${CMAKE_BUILD_TYPE}
        WORKING_DIRECTORY "${CxxKitWrapFunction2_BUILD_DIR}"
        RESULT_VARIABLE INSTALL_RESULT)
    if(NOT INSTALL_RESULT MATCHES 0)
        message(FATAL_ERROR "${CxxKitWrapFunction2_NAME} install failed.")
    endif()
    message(STATUS "${CxxKitWrapFunction2_NAME} install success")
    cxxkit_make_stamp_file("${CxxKitWrapFunction2_STAMP_FILE_PATH}")
endif()
# wrap lib
add_library(CxxKitWrapFunction2::WrapFunction2 INTERFACE IMPORTED)
set(function2_DIR "${CxxKitWrapFunction2_INSTALL_DIR}/lib/cmake/function2")
find_package(function2 PATHS "${CxxKitWrapFunction2_INSTALL_DIR}/lib" NO_DEFAULT_PATH REQUIRED)
target_link_libraries(CxxKitWrapFunction2::WrapFunction2 INTERFACE function2::function2)
set(CxxKitWrapFunction2_FOUND ON)
