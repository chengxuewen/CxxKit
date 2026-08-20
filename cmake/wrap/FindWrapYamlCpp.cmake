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
if(TARGET CxxKitWrapYamlCpp::WrapYamlCpp)
    set(CxxKitWrapYamlCpp_FOUND ON)
    return()
endif()

set(CxxKitWrapYamlCpp_NAME "yaml-cpp-0.9.0")
set(CxxKitWrapYamlCpp_PKG_NAME "${CxxKitWrapYamlCpp_NAME}.zip")
set(CxxKitWrapYamlCpp_DIR_NAME "${CxxKitWrapYamlCpp_NAME}-${CXXKIT_LOWER_BUILD_TYPE}")
set(CxxKitWrapYamlCpp_URL_PATH "${PROJECT_SOURCE_DIR}/3rdparty/${CxxKitWrapYamlCpp_PKG_NAME}")
set(CxxKitWrapYamlCpp_ROOT_DIR "${PROJECT_BINARY_DIR}/3rdparty/${CxxKitWrapYamlCpp_DIR_NAME}")
set(CxxKitWrapYamlCpp_BUILD_DIR "${CxxKitWrapYamlCpp_ROOT_DIR}/build" CACHE INTERNAL "" FORCE)
set(CxxKitWrapYamlCpp_SOURCE_DIR "${CxxKitWrapYamlCpp_ROOT_DIR}/source" CACHE INTERNAL "" FORCE)
set(CxxKitWrapYamlCpp_INSTALL_DIR "${CxxKitWrapYamlCpp_ROOT_DIR}/install" CACHE INTERNAL "" FORCE)
cxxkit_stamp_file_info(CxxKitWrapYamlCpp OUTPUT_DIR "${CxxKitWrapYamlCpp_ROOT_DIR}")
cxxkit_fetch_3rdparty(CxxKitWrapYamlCpp URL "${CxxKitWrapYamlCpp_URL_PATH}" OUTPUT_NAME "${CxxKitWrapYamlCpp_DIR_NAME}")
if(NOT EXISTS "${CxxKitWrapYamlCpp_STAMP_FILE_PATH}")
    if(NOT EXISTS ${CxxKitWrapYamlCpp_SOURCE_DIR})
        message(FATAL_ERROR "${CxxKitWrapYamlCpp_NAME} FetchContent failed.")
    endif()
    cxxkit_reset_dir(${CxxKitWrapYamlCpp_BUILD_DIR})

    message(STATUS "Configure ${CxxKitWrapYamlCpp_NAME} lib...")
    execute_process(
        COMMAND ${CMAKE_COMMAND}
        -Wno-deprecated
        --no-warn-unused-cli
        -G ${CMAKE_GENERATOR}
        -DYAML_BUILD_SHARED_LIBS=OFF
        -DYAML_CPP_BUILD_TESTS=OFF
        -DYAML_CPP_INSTALL=ON
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DCMAKE_INSTALL_PREFIX=${CxxKitWrapYamlCpp_INSTALL_DIR}
        ${CxxKitWrapYamlCpp_SOURCE_DIR}
        WORKING_DIRECTORY "${CxxKitWrapYamlCpp_BUILD_DIR}"
        RESULT_VARIABLE CONFIGURE_RESULT)
    if(NOT CONFIGURE_RESULT MATCHES 0)
        message(FATAL_ERROR "${CxxKitWrapYamlCpp_NAME} configure failed.")
    endif()
    message(STATUS "${CxxKitWrapYamlCpp_NAME} configure success")

    execute_process(
        COMMAND ${CMAKE_COMMAND} --build ./ --parallel ${CXXKIT_NUMBER_OF_ASYNC_JOBS} --config Release --target install
        WORKING_DIRECTORY "${CxxKitWrapYamlCpp_BUILD_DIR}"
        RESULT_VARIABLE BUILD_RESULT)
    if(NOT BUILD_RESULT MATCHES 0)
        message(FATAL_ERROR "${CxxKitWrapYamlCpp_NAME} build failed.")
    endif()
    message(STATUS "${CxxKitWrapYamlCpp_NAME} build success")

    execute_process(
        COMMAND ${CMAKE_COMMAND} --install ./
        WORKING_DIRECTORY "${CxxKitWrapYamlCpp_BUILD_DIR}"
        RESULT_VARIABLE INSTALL_RESULT)
    if(NOT INSTALL_RESULT MATCHES 0)
        message(FATAL_ERROR "${CxxKitWrapYamlCpp_NAME} install failed.")
    endif()
    message(STATUS "${CxxKitWrapYamlCpp_NAME} install success")
    cxxkit_make_stamp_file("${CxxKitWrapYamlCpp_STAMP_FILE_PATH}")
endif()
# wrap lib
add_library(CxxKitWrapYamlCpp::WrapYamlCpp INTERFACE IMPORTED)
find_package(yaml-cpp PATHS ${CxxKitWrapYamlCpp_INSTALL_DIR} NO_DEFAULT_PATH REQUIRED)
target_link_libraries(CxxKitWrapYamlCpp::WrapYamlCpp INTERFACE yaml-cpp::yaml-cpp)
set(CxxKitWrapYamlCpp_FOUND ON)