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
if(TARGET CXXKitWrapYamlCpp::WrapYamlCpp)
    set(CXXKitWrapYamlCpp_FOUND ON)
    return()
endif()

set(CXXKitWrapYamlCpp_NAME "yaml-cpp-0.9.0")
set(CXXKitWrapYamlCpp_PKG_NAME "${CXXKitWrapYamlCpp_NAME}.zip")
set(CXXKitWrapYamlCpp_DIR_NAME "${CXXKitWrapYamlCpp_NAME}-${CXXKIT_LOWER_BUILD_TYPE}")
set(CXXKitWrapYamlCpp_URL_PATH "${PROJECT_SOURCE_DIR}/3rdparty/${CXXKitWrapYamlCpp_PKG_NAME}")
set(CXXKitWrapYamlCpp_ROOT_DIR "${PROJECT_BINARY_DIR}/3rdparty/${CXXKitWrapYamlCpp_DIR_NAME}")
set(CXXKitWrapYamlCpp_BUILD_DIR "${CXXKitWrapYamlCpp_ROOT_DIR}/build" CACHE INTERNAL "" FORCE)
set(CXXKitWrapYamlCpp_SOURCE_DIR "${CXXKitWrapYamlCpp_ROOT_DIR}/source" CACHE INTERNAL "" FORCE)
set(CXXKitWrapYamlCpp_INSTALL_DIR "${CXXKitWrapYamlCpp_ROOT_DIR}/install" CACHE INTERNAL "" FORCE)
cxxkit_stamp_file_info(CXXKitWrapYamlCpp OUTPUT_DIR "${CXXKitWrapYamlCpp_ROOT_DIR}")
cxxkit_fetch_3rdparty(CXXKitWrapYamlCpp URL "${CXXKitWrapYamlCpp_URL_PATH}" OUTPUT_NAME "${CXXKitWrapYamlCpp_DIR_NAME}")
if(NOT EXISTS "${CXXKitWrapYamlCpp_STAMP_FILE_PATH}")
    if(NOT EXISTS ${CXXKitWrapYamlCpp_SOURCE_DIR})
        message(FATAL_ERROR "${CXXKitWrapYamlCpp_NAME} FetchContent failed.")
    endif()
    cxxkit_reset_dir(${CXXKitWrapYamlCpp_BUILD_DIR})

    message(STATUS "Configure ${CXXKitWrapYamlCpp_NAME} lib...")
    execute_process(
        COMMAND ${CMAKE_COMMAND}
        -Wno-deprecated
        --no-warn-unused-cli
        -G ${CMAKE_GENERATOR}
        -DYAML_BUILD_SHARED_LIBS=OFF
        -DYAML_CPP_BUILD_TESTS=OFF
        -DYAML_CPP_INSTALL=ON
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DCMAKE_INSTALL_PREFIX=${CXXKitWrapYamlCpp_INSTALL_DIR}
        ${CXXKitWrapYamlCpp_SOURCE_DIR}
        WORKING_DIRECTORY "${CXXKitWrapYamlCpp_BUILD_DIR}"
        RESULT_VARIABLE CONFIGURE_RESULT)
    if(NOT CONFIGURE_RESULT MATCHES 0)
        message(FATAL_ERROR "${CXXKitWrapYamlCpp_NAME} configure failed.")
    endif()
    message(STATUS "${CXXKitWrapYamlCpp_NAME} configure success")

    execute_process(
        COMMAND ${CMAKE_COMMAND} --build ./ --parallel ${CXXKIT_NUMBER_OF_ASYNC_JOBS} --config Release --target install
        WORKING_DIRECTORY "${CXXKitWrapYamlCpp_BUILD_DIR}"
        RESULT_VARIABLE BUILD_RESULT)
    if(NOT BUILD_RESULT MATCHES 0)
        message(FATAL_ERROR "${CXXKitWrapYamlCpp_NAME} build failed.")
    endif()
    message(STATUS "${CXXKitWrapYamlCpp_NAME} build success")

    execute_process(
        COMMAND ${CMAKE_COMMAND} --install ./
        WORKING_DIRECTORY "${CXXKitWrapYamlCpp_BUILD_DIR}"
        RESULT_VARIABLE INSTALL_RESULT)
    if(NOT INSTALL_RESULT MATCHES 0)
        message(FATAL_ERROR "${CXXKitWrapYamlCpp_NAME} install failed.")
    endif()
    message(STATUS "${CXXKitWrapYamlCpp_NAME} install success")
    cxxkit_make_stamp_file("${CXXKitWrapYamlCpp_STAMP_FILE_PATH}")
endif()
# wrap lib
add_library(CXXKitWrapYamlCpp::WrapYamlCpp INTERFACE IMPORTED)
find_package(yaml-cpp PATHS ${CXXKitWrapYamlCpp_INSTALL_DIR} NO_DEFAULT_PATH REQUIRED)
target_link_libraries(CXXKitWrapYamlCpp::WrapYamlCpp INTERFACE yaml-cpp::yaml-cpp)
set(CXXKitWrapYamlCpp_FOUND ON)