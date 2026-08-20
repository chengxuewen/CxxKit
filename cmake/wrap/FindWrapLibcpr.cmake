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
if(TARGET CxxKitWrapLibcpr::WrapLibcpr)
	set(CxxKitWrapLibcpr_FOUND ON)
	return()
endif()

cxxkit_find_package(MbedTLS PROVIDED_TARGETS CxxKitWrapMbedTLS::WrapMbedTLS)
cxxkit_find_package(Libcurl PROVIDED_TARGETS CxxKitWrapLibcurl::WrapLibcurl)
set(CxxKitWrapLibcpr_NAME "cpr-1.9.9")
set(CxxKitWrapLibcpr_PKG_NAME "${CxxKitWrapLibcpr_NAME}.tar.gz")
set(CxxKitWrapLibcpr_DIR_NAME "${CxxKitWrapLibcpr_NAME}-${CXXKIT_LOWER_BUILD_TYPE}")
set(CxxKitWrapLibcpr_URL_PATH "${PROJECT_SOURCE_DIR}/3rdparty/${CxxKitWrapLibcpr_PKG_NAME}")
set(CxxKitWrapLibcpr_ROOT_DIR "${PROJECT_BINARY_DIR}/3rdparty/${CxxKitWrapLibcpr_DIR_NAME}")
set(CxxKitWrapLibcpr_BUILD_DIR "${CxxKitWrapLibcpr_ROOT_DIR}/build" CACHE INTERNAL "" FORCE)
set(CxxKitWrapLibcpr_SOURCE_DIR "${CxxKitWrapLibcpr_ROOT_DIR}/source" CACHE INTERNAL "" FORCE)
set(CxxKitWrapLibcpr_INSTALL_DIR "${CxxKitWrapLibcpr_ROOT_DIR}/install" CACHE INTERNAL "" FORCE)
cxxkit_stamp_file_info(CxxKitWrapLibcpr OUTPUT_DIR "${CxxKitWrapLibcpr_ROOT_DIR}")
cxxkit_fetch_3rdparty(CxxKitWrapLibcpr URL "${CxxKitWrapLibcpr_URL_PATH}" OUTPUT_NAME "${CxxKitWrapLibcpr_DIR_NAME}")
if(NOT EXISTS "${CxxKitWrapLibcpr_STAMP_FILE_PATH}")
	if(NOT EXISTS ${CxxKitWrapLibcpr_SOURCE_DIR})
		message(FATAL_ERROR "${CxxKitWrapLibcpr_DIR_NAME} FetchContent failed.")
	endif()
	cxxkit_reset_dir(${CxxKitWrapLibcpr_BUILD_DIR})

	message(STATUS "Configure ${CxxKitWrapLibcpr_DIR_NAME} lib...")
	execute_process(
		COMMAND ${CMAKE_COMMAND}
        -Wno-deprecated
        --no-warn-unused-cli
		-G ${CMAKE_GENERATOR}
		-DCPR_FORCE_MBEDTLS_BACKEND=ON
		-DCPR_FORCE_USE_SYSTEM_CURL=ON
		-DCURL_INCLUDE_DIR=${CxxKitWrapLibcurl_INCLUDE_DIR}
		-DCURL_LIBRARY=${CxxKitWrapLibcurl_LIBRARY}
		-DCMAKE_POSITION_INDEPENDENT_CODE=ON
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
		-DCMAKE_PREFIX_PATH=${CxxKitWrapLibcurl_INSTALL_DIR}
		-DCMAKE_INSTALL_PREFIX=${CxxKitWrapLibcpr_INSTALL_DIR}
		${CxxKitWrapLibcpr_SOURCE_DIR}
		WORKING_DIRECTORY "${CxxKitWrapLibcpr_BUILD_DIR}"
		RESULT_VARIABLE CONFIGURE_RESULT)
	if(NOT CONFIGURE_RESULT MATCHES 0)
		message(FATAL_ERROR "${CxxKitWrapLibcpr_DIR_NAME} configure failed.")
	endif()
	message(STATUS "${CxxKitWrapLibcpr_DIR_NAME} configure success")

	execute_process(
		COMMAND ${CMAKE_COMMAND} --build ./ --parallel ${CXXKIT_NUMBER_OF_ASYNC_JOBS} --config
		${CMAKE_BUILD_TYPE} --target install
		WORKING_DIRECTORY "${CxxKitWrapLibcpr_BUILD_DIR}"
		RESULT_VARIABLE BUILD_RESULT)
	if(NOT BUILD_RESULT MATCHES 0)
		message(FATAL_ERROR "${CxxKitWrapLibcpr_DIR_NAME} build failed.")
	endif()
	message(STATUS "${CxxKitWrapLibcpr_DIR_NAME} build success")

	execute_process(
		COMMAND ${CMAKE_COMMAND} --install ./ --config ${CMAKE_BUILD_TYPE}
		WORKING_DIRECTORY "${CxxKitWrapLibcpr_BUILD_DIR}"
		RESULT_VARIABLE INSTALL_RESULT)
	if(NOT INSTALL_RESULT MATCHES 0)
		message(FATAL_ERROR "${CxxKitWrapLibcpr_DIR_NAME} install failed.")
	endif()
	message(STATUS "${CxxKitWrapLibcpr_DIR_NAME} install success")
	cxxkit_make_stamp_file("${CxxKitWrapLibcpr_STAMP_FILE_PATH}")
endif()
# wrap lib
add_library(CxxKitWrapLibcpr::WrapLibcpr INTERFACE IMPORTED)
set(CURL_INCLUDE_DIR ${CxxKitWrapLibcurl_INCLUDE_DIR})
set(CURL_LIBRARY ${CxxKitWrapLibcurl_LIBRARY})
find_package(cpr PATHS ${CxxKitWrapLibcpr_INSTALL_DIR} NO_DEFAULT_PATH REQUIRED)
target_link_libraries(CxxKitWrapLibcpr::WrapLibcpr INTERFACE cpr::cpr CxxKitWrapMbedTLS::WrapMbedTLS)
set(CxxKitWrapLibcpr_FOUND ON)