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
if(TARGET CxxKitWrapAnyLite::WrapAnyLite)
    set(CxxKitWrapAnyLite_FOUND ON)
    return()
endif()

set(CxxKitWrapAnyLite_NAME "any-lite-master")
set(CxxKitWrapAnyLite_PKG_NAME "${CxxKitWrapAnyLite_NAME}.zip")
set(CxxKitWrapAnyLite_DIR_NAME "${CxxKitWrapAnyLite_NAME}-${CXXKIT_LOWER_BUILD_TYPE}")
set(CxxKitWrapAnyLite_URL_PATH "${PROJECT_SOURCE_DIR}/3rdparty/${CxxKitWrapAnyLite_PKG_NAME}")
set(CxxKitWrapAnyLite_ROOT_DIR "${PROJECT_BINARY_DIR}/3rdparty/${CxxKitWrapAnyLite_DIR_NAME}")
set(CxxKitWrapAnyLite_BUILD_DIR "${CxxKitWrapAnyLite_ROOT_DIR}/build" CACHE INTERNAL "" FORCE)
set(CxxKitWrapAnyLite_SOURCE_DIR "${CxxKitWrapAnyLite_ROOT_DIR}/source" CACHE INTERNAL "" FORCE)
set(CxxKitWrapAnyLite_INSTALL_DIR "${CxxKitWrapAnyLite_ROOT_DIR}/install" CACHE INTERNAL "" FORCE)
cxxkit_stamp_file_info(CxxKitWrapAnyLite OUTPUT_DIR "${CxxKitWrapAnyLite_ROOT_DIR}")
cxxkit_fetch_3rdparty(CxxKitWrapAnyLite URL "${CxxKitWrapAnyLite_URL_PATH}" OUTPUT_NAME "${CxxKitWrapAnyLite_DIR_NAME}")
if(NOT EXISTS "${CxxKitWrapAnyLite_STAMP_FILE_PATH}")
    if(NOT EXISTS ${CxxKitWrapAnyLite_SOURCE_DIR})
        message(FATAL_ERROR "${CxxKitWrapAnyLite_NAME} FetchContent failed.")
    endif()
    cxxkit_reset_dir(${CxxKitWrapAnyLite_BUILD_DIR})

    message(STATUS "Configure ${CxxKitWrapAnyLite_NAME} lib...")
    execute_process(
        COMMAND ${CMAKE_COMMAND}
        -Wno-deprecated
        --no-warn-unused-cli
        -G ${CMAKE_GENERATOR}
        -DANY_LITE_OPT_BUILD_TESTS=OFF
        -DCMAKE_INSTALL_PREFIX=${CxxKitWrapAnyLite_INSTALL_DIR}
        ${CxxKitWrapAnyLite_SOURCE_DIR}
        WORKING_DIRECTORY "${CxxKitWrapAnyLite_BUILD_DIR}"
        RESULT_VARIABLE CONFIGURE_RESULT)
    if(NOT CONFIGURE_RESULT MATCHES 0)
        message(FATAL_ERROR "${CxxKitWrapAnyLite_NAME} configure failed.")
    endif()
    message(STATUS "${CxxKitWrapAnyLite_NAME} configure success")

    execute_process(
        COMMAND ${CMAKE_COMMAND} --build ./ --parallel ${CXXKIT_NUMBER_OF_ASYNC_JOBS} --config Release --target install
        WORKING_DIRECTORY "${CxxKitWrapAnyLite_BUILD_DIR}"
        RESULT_VARIABLE BUILD_RESULT)
    if(NOT BUILD_RESULT MATCHES 0)
        message(FATAL_ERROR "${CxxKitWrapAnyLite_NAME} build failed.")
    endif()
    message(STATUS "${CxxKitWrapAnyLite_NAME} build success")

    execute_process(
        COMMAND ${CMAKE_COMMAND} --install ./ --config ${CMAKE_BUILD_TYPE}
        WORKING_DIRECTORY "${CxxKitWrapAnyLite_BUILD_DIR}"
        RESULT_VARIABLE INSTALL_RESULT)
    if(NOT INSTALL_RESULT MATCHES 0)
        message(FATAL_ERROR "${CxxKitWrapAnyLite_NAME} install failed.")
    endif()
    message(STATUS "${CxxKitWrapAnyLite_NAME} install success")
    cxxkit_make_stamp_file("${CxxKitWrapAnyLite_STAMP_FILE_PATH}")
endif()
# wrap lib
add_library(CxxKitWrapAnyLite::WrapAnyLite INTERFACE IMPORTED)
find_package(any-lite PATHS ${CxxKitWrapAnyLite_INSTALL_DIR} REQUIRED)
target_link_libraries(CxxKitWrapAnyLite::WrapAnyLite INTERFACE nonstd::any-lite)
set(CxxKitWrapAnyLite_FOUND ON)