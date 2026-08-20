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
if(TARGET CxxKitWrapJson::WrapJson)
    set(CxxKitWrapJson_FOUND ON)
    return()
endif()

set(CxxKitWrapJson_NAME "json-3.12.0")
set(CxxKitWrapJson_PKG_NAME "${CxxKitWrapJson_NAME}.7z")
set(CxxKitWrapJson_DIR_NAME "${CxxKitWrapJson_NAME}-${CXXKIT_LOWER_BUILD_TYPE}")
set(CxxKitWrapJson_URL_PATH "${PROJECT_SOURCE_DIR}/3rdparty/${CxxKitWrapJson_PKG_NAME}")
set(CxxKitWrapJson_ROOT_DIR "${PROJECT_BINARY_DIR}/3rdparty/${CxxKitWrapJson_DIR_NAME}")
set(CxxKitWrapJson_BUILD_DIR "${CxxKitWrapJson_ROOT_DIR}/build" CACHE INTERNAL "" FORCE)
set(CxxKitWrapJson_SOURCE_DIR "${CxxKitWrapJson_ROOT_DIR}/source" CACHE INTERNAL "" FORCE)
set(CxxKitWrapJson_INSTALL_DIR "${CxxKitWrapJson_ROOT_DIR}/install" CACHE INTERNAL "" FORCE)
cxxkit_stamp_file_info(CxxKitWrapJson OUTPUT_DIR "${CxxKitWrapJson_ROOT_DIR}")
cxxkit_fetch_3rdparty(CxxKitWrapJson URL "${CxxKitWrapJson_URL_PATH}" OUTPUT_NAME "${CxxKitWrapJson_DIR_NAME}")
if(NOT EXISTS "${CxxKitWrapJson_STAMP_FILE_PATH}")
    if(NOT EXISTS ${CxxKitWrapJson_SOURCE_DIR})
        message(FATAL_ERROR "${CxxKitWrapJson_DIR_NAME} FetchContent failed.")
    endif()
    cxxkit_reset_dir(${CxxKitWrapJson_BUILD_DIR})

    message(STATUS "Configure ${CxxKitWrapJson_DIR_NAME} lib...")
    execute_process(
        COMMAND ${CMAKE_COMMAND}
        -Wno-deprecated
        --no-warn-unused-cli
        -G ${CMAKE_GENERATOR}
        -DJSON_BuildTests=OFF
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX=${CxxKitWrapJson_INSTALL_DIR}
        ${CxxKitWrapJson_SOURCE_DIR}
        WORKING_DIRECTORY "${CxxKitWrapJson_BUILD_DIR}"
        RESULT_VARIABLE CONFIGURE_RESULT)
    if(NOT CONFIGURE_RESULT MATCHES 0)
        message(FATAL_ERROR "${CxxKitWrapJson_DIR_NAME} configure failed.")
    endif()
    message(STATUS "${CxxKitWrapJson_DIR_NAME} configure success")
    
    execute_process(
        COMMAND ${CMAKE_COMMAND} --build ./ --parallel ${CXXKIT_NUMBER_OF_ASYNC_JOBS} --config 
        ${CMAKE_BUILD_TYPE} --target install
        WORKING_DIRECTORY "${CxxKitWrapJson_BUILD_DIR}"
        RESULT_VARIABLE BUILD_RESULT)
    if(NOT BUILD_RESULT MATCHES 0)
        message(FATAL_ERROR "${CxxKitWrapJson_DIR_NAME} build failed.")
    endif()
    message(STATUS "${CxxKitWrapJson_DIR_NAME} build success")
    
    execute_process(
        COMMAND ${CMAKE_COMMAND} --install ./ --config ${CMAKE_BUILD_TYPE}
        WORKING_DIRECTORY "${CxxKitWrapJson_BUILD_DIR}"
        RESULT_VARIABLE INSTALL_RESULT)
    if(NOT INSTALL_RESULT MATCHES 0)
        message(FATAL_ERROR "${CxxKitWrapJson_DIR_NAME} install failed.")
    endif()        
    message(STATUS "${CxxKitWrapJson_DIR_NAME} install success")
    cxxkit_make_stamp_file("${CxxKitWrapJson_STAMP_FILE_PATH}")
endif()
# wrap lib
add_library(CxxKitWrapJson::WrapJson INTERFACE IMPORTED)
set(nlohmann_json_DIR "${CxxKitWrapJson_INSTALL_DIR}/share/cmake/nlohmann_json")
find_package(nlohmann_json PATHS "${CxxKitWrapJson_INSTALL_DIR}" NO_DEFAULT_PATH REQUIRED)
target_link_libraries(CxxKitWrapJson::WrapJson INTERFACE nlohmann_json::nlohmann_json)
set(CxxKitWrapJson_FOUND ON)