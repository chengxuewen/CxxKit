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
if(TARGET CXXKitWrapJson::WrapJson)
    set(CXXKitWrapJson_FOUND ON)
    return()
endif()

set(CXXKitWrapJson_NAME "json-3.12.0")
set(CXXKitWrapJson_PKG_NAME "${CXXKitWrapJson_NAME}.7z")
set(CXXKitWrapJson_DIR_NAME "${CXXKitWrapJson_NAME}-${CXXKIT_LOWER_BUILD_TYPE}")
set(CXXKitWrapJson_URL_PATH "${PROJECT_SOURCE_DIR}/3rdparty/${CXXKitWrapJson_PKG_NAME}")
set(CXXKitWrapJson_ROOT_DIR "${PROJECT_BINARY_DIR}/3rdparty/${CXXKitWrapJson_DIR_NAME}")
set(CXXKitWrapJson_BUILD_DIR "${CXXKitWrapJson_ROOT_DIR}/build" CACHE INTERNAL "" FORCE)
set(CXXKitWrapJson_SOURCE_DIR "${CXXKitWrapJson_ROOT_DIR}/source" CACHE INTERNAL "" FORCE)
set(CXXKitWrapJson_INSTALL_DIR "${CXXKitWrapJson_ROOT_DIR}/install" CACHE INTERNAL "" FORCE)
cxxkit_stamp_file_info(CXXKitWrapJson OUTPUT_DIR "${CXXKitWrapJson_ROOT_DIR}")
cxxkit_fetch_3rdparty(CXXKitWrapJson URL "${CXXKitWrapJson_URL_PATH}" OUTPUT_NAME "${CXXKitWrapJson_DIR_NAME}")
if(NOT EXISTS "${CXXKitWrapJson_STAMP_FILE_PATH}")
    if(NOT EXISTS ${CXXKitWrapJson_SOURCE_DIR})
        message(FATAL_ERROR "${CXXKitWrapJson_DIR_NAME} FetchContent failed.")
    endif()
    cxxkit_reset_dir(${CXXKitWrapJson_BUILD_DIR})

    message(STATUS "Configure ${CXXKitWrapJson_DIR_NAME} lib...")
    execute_process(
        COMMAND ${CMAKE_COMMAND}
        -Wno-deprecated
        --no-warn-unused-cli
        -G ${CMAKE_GENERATOR}
        -DJSON_BuildTests=OFF
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX=${CXXKitWrapJson_INSTALL_DIR}
        ${CXXKitWrapJson_SOURCE_DIR}
        WORKING_DIRECTORY "${CXXKitWrapJson_BUILD_DIR}"
        RESULT_VARIABLE CONFIGURE_RESULT)
    if(NOT CONFIGURE_RESULT MATCHES 0)
        message(FATAL_ERROR "${CXXKitWrapJson_DIR_NAME} configure failed.")
    endif()
    message(STATUS "${CXXKitWrapJson_DIR_NAME} configure success")
    
    execute_process(
        COMMAND ${CMAKE_COMMAND} --build ./ --parallel ${CXXKIT_NUMBER_OF_ASYNC_JOBS} --config 
        ${CMAKE_BUILD_TYPE} --target install
        WORKING_DIRECTORY "${CXXKitWrapJson_BUILD_DIR}"
        RESULT_VARIABLE BUILD_RESULT)
    if(NOT BUILD_RESULT MATCHES 0)
        message(FATAL_ERROR "${CXXKitWrapJson_DIR_NAME} build failed.")
    endif()
    message(STATUS "${CXXKitWrapJson_DIR_NAME} build success")
    
    execute_process(
        COMMAND ${CMAKE_COMMAND} --install ./ --config ${CMAKE_BUILD_TYPE}
        WORKING_DIRECTORY "${CXXKitWrapJson_BUILD_DIR}"
        RESULT_VARIABLE INSTALL_RESULT)
    if(NOT INSTALL_RESULT MATCHES 0)
        message(FATAL_ERROR "${CXXKitWrapJson_DIR_NAME} install failed.")
    endif()        
    message(STATUS "${CXXKitWrapJson_DIR_NAME} install success")
    cxxkit_make_stamp_file("${CXXKitWrapJson_STAMP_FILE_PATH}")
endif()
# wrap lib
add_library(CXXKitWrapJson::WrapJson INTERFACE IMPORTED)
set(nlohmann_json_DIR "${CXXKitWrapJson_INSTALL_DIR}/share/cmake/nlohmann_json")
find_package(nlohmann_json PATHS "${CXXKitWrapJson_INSTALL_DIR}" NO_DEFAULT_PATH REQUIRED)
target_link_libraries(CXXKitWrapJson::WrapJson INTERFACE nlohmann_json::nlohmann_json)
set(CXXKitWrapJson_FOUND ON)