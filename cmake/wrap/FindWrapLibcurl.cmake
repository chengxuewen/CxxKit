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
if(TARGET CxxKitWrapLibcurl::WrapLibcurl)
    set(CxxKitWrapLibcurl_FOUND ON)
    return()
endif()

set(CxxKitWrapLibcurl_NAME "curl-8.16.0")
set(CxxKitWrapLibcurl_PKG_NAME "${CxxKitWrapLibcurl_NAME}.tar.xz")
set(CxxKitWrapLibcurl_DIR_NAME "${CxxKitWrapLibcurl_NAME}-${CXXKIT_LOWER_BUILD_TYPE}")
set(CxxKitWrapLibcurl_URL_PATH "${PROJECT_SOURCE_DIR}/3rdparty/${CxxKitWrapLibcurl_PKG_NAME}")
set(CxxKitWrapLibcurl_ROOT_DIR "${PROJECT_BINARY_DIR}/3rdparty/${CxxKitWrapLibcurl_DIR_NAME}")
set(CxxKitWrapLibcurl_BUILD_DIR "${CxxKitWrapLibcurl_ROOT_DIR}/build" CACHE INTERNAL "" FORCE)
set(CxxKitWrapLibcurl_SOURCE_DIR "${CxxKitWrapLibcurl_ROOT_DIR}/source" CACHE INTERNAL "" FORCE)
set(CxxKitWrapLibcurl_INSTALL_DIR "${CxxKitWrapLibcurl_ROOT_DIR}/install" CACHE INTERNAL "" FORCE)
cxxkit_stamp_file_info(CxxKitWrapLibcurl OUTPUT_DIR "${CxxKitWrapLibcurl_ROOT_DIR}")
cxxkit_fetch_3rdparty(CxxKitWrapLibcurl URL "${CxxKitWrapLibcurl_URL_PATH}" OUTPUT_NAME "${CxxKitWrapLibcurl_DIR_NAME}")
if(NOT EXISTS "${CxxKitWrapLibcurl_STAMP_FILE_PATH}")
    if(NOT EXISTS ${CxxKitWrapLibcurl_SOURCE_DIR})
        message(FATAL_ERROR "${CxxKitWrapLibcurl_NAME} FetchContent failed.")
    endif()
    cxxkit_reset_dir(${CxxKitWrapLibcurl_BUILD_DIR})

    cxxkit_find_package(MbedTLS PROVIDED_TARGETS CxxKitWrapMbedTLS::WrapMbedTLS)
    message(STATUS "Configure ${CxxKitWrapLibcurl_NAME} lib...")
    execute_process(
        COMMAND ${CMAKE_COMMAND}
        -Wno-deprecated
        --no-warn-unused-cli
        -G ${CMAKE_GENERATOR}
		-DUSE_LIBIDN2=OFF
        -DCURL_USE_MBEDTLS=ON
        -DUSE_LIBSSH2=OFF
        -DUSE_NGHTTP2=OFF
        -DUSE_OPENSSL=OFF
        -DCURL_DISABLE_LDAP=ON
        -DCURL_DISABLE_LDAPS=ON
        -DCURL_ZLIB=OFF
        -DCURL_BROTLI=OFF
        -DCURL_ZSTD=OFF
        -DCURL_USE_LIBSSH=OFF
        -DCURL_USE_LIBSSH2=OFF
        -DCURL_DISABLE_SSH=ON
        -DCURL_USE_LIBPSL=OFF
        -DCURL_DISABLE_LDAP=ON
        -DCURL_DISABLE_LDAPS=ON
        -DBUILD_STATIC_CURL=ON
        -DBUILD_SHARED_LIBS=OFF
        -DBUILD_FOR_MT=${CXXKIT_MSVC_STATIC_RUNTIME}
        -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_PREFIX_PATH=${CxxKitWrapMbedTLS_INSTALL_DIR}
        -DCMAKE_INSTALL_PREFIX=${CxxKitWrapLibcurl_INSTALL_DIR}
        ${CxxKitWrapLibcurl_SOURCE_DIR}
        WORKING_DIRECTORY "${CxxKitWrapLibcurl_BUILD_DIR}"
        RESULT_VARIABLE CONFIGURE_RESULT)
    if(NOT CONFIGURE_RESULT MATCHES 0)
        message(FATAL_ERROR "${CxxKitWrapLibcurl_NAME} configure failed.")
    endif()
    message(STATUS "${CxxKitWrapLibcurl_NAME} configure success")

    execute_process(
        COMMAND ${CMAKE_COMMAND} --build ./ --parallel ${CXXKIT_NUMBER_OF_ASYNC_JOBS} 
        --config ${CMAKE_BUILD_TYPE} --target install
        WORKING_DIRECTORY "${CxxKitWrapLibcurl_BUILD_DIR}"
        RESULT_VARIABLE BUILD_RESULT)
    if(NOT BUILD_RESULT MATCHES 0)
        message(FATAL_ERROR "${CxxKitWrapLibcurl_NAME} build failed.")
    endif()
    message(STATUS "${CxxKitWrapLibcurl_NAME} build success")

    execute_process(
        COMMAND ${CMAKE_COMMAND} --install ./ --config ${CMAKE_BUILD_TYPE}
        WORKING_DIRECTORY "${CxxKitWrapLibcurl_BUILD_DIR}"
        RESULT_VARIABLE INSTALL_RESULT)
    if(NOT INSTALL_RESULT MATCHES 0)
        message(FATAL_ERROR "${CxxKitWrapLibcurl_NAME} install failed.")
    endif()
    message(STATUS "${CxxKitWrapLibcurl_NAME} install success")
    cxxkit_make_stamp_file("${CxxKitWrapLibcurl_STAMP_FILE_PATH}")
endif()
# wrap lib
add_library(CxxKitWrapLibcurl::WrapLibcurl INTERFACE IMPORTED)
find_package(CURL PATHS ${CxxKitWrapLibcurl_INSTALL_DIR} NO_DEFAULT_PATH REQUIRED)
target_link_libraries(CxxKitWrapLibcurl::WrapLibcurl INTERFACE CURL::libcurl_static)
target_link_libraries(CxxKitWrapLibcurl::WrapLibcurl INTERFACE CxxKitWrapMbedTLS::WrapMbedTLS)
string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE_UPPER)
get_target_property(CURL_STATIC_LIB_PATH CURL::libcurl_static IMPORTED_LOCATION_${BUILD_TYPE_UPPER})
set(CxxKitWrapLibcurl_INCLUDE_DIR "${CxxKitWrapLibcurl_INSTALL_DIR}/include" CACHE INTERNAL "" FORCE)
set(CxxKitWrapLibcurl_LIBRARY "${CURL_STATIC_LIB_PATH}" CACHE INTERNAL "" FORCE)
set(CxxKitWrapLibcurl_FOUND ON)
