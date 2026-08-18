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
if(TARGET CXXKitWrapZLIB::WrapZLIB)
	set(CXXKitWrapZLIB_FOUND ON)
	return()
endif()

set(CXXKitWrapZLIB_NAME "zlib-1.3.1")
set(CXXKitWrapZLIB_DIR_NAME "${CXXKitWrapZLIB_NAME}")
set(CXXKitWrapZLIB_PKG_NAME "${CXXKitWrapZLIB_NAME}.tar.gz")
set(CXXKitWrapZLIB_URL_PATH "${PROJECT_SOURCE_DIR}/3rdparty/${CXXKitWrapZLIB_PKG_NAME}")
set(CXXKitWrapZLIB_ROOT_DIR "${PROJECT_BINARY_DIR}/3rdparty/${CXXKitWrapZLIB_DIR_NAME}")
set(CXXKitWrapZLIB_BUILD_DIR "${CXXKitWrapZLIB_ROOT_DIR}/build" CACHE INTERNAL "" FORCE)
set(CXXKitWrapZLIB_SOURCE_DIR "${CXXKitWrapZLIB_ROOT_DIR}/source" CACHE INTERNAL "" FORCE)
set(CXXKitWrapZLIB_INSTALL_DIR "${CXXKitWrapZLIB_ROOT_DIR}/install" CACHE INTERNAL "" FORCE)
cxxkit_stamp_file_info(CXXKitWrapZLIB OUTPUT_DIR "${CXXKitWrapZLIB_ROOT_DIR}")
cxxkit_fetch_3rdparty(CXXKitWrapZLIB URL "${CXXKitWrapZLIB_URL_PATH}")
if(NOT EXISTS "${CXXKitWrapZLIB_STAMP_FILE_PATH}")
	if(NOT EXISTS ${CXXKitWrapZLIB_SOURCE_DIR})
		message(FATAL_ERROR "${CXXKitWrapZLIB_DIR_NAME} FetchContent failed.")
	endif()
	cxxkit_reset_dir(${CXXKitWrapZLIB_BUILD_DIR})

	message(STATUS "Configure ${CXXKitWrapZLIB_DIR_NAME} lib...")
	execute_process(
		COMMAND ${CMAKE_COMMAND}
        -Wno-deprecated
        --no-warn-unused-cli
		-G ${CMAKE_GENERATOR}
		-DZLIB_BUILD_TESTING=OFF
		-DZLIB_BUILD_SHARED=OFF
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
		-DCMAKE_INSTALL_PREFIX=${CXXKitWrapZLIB_INSTALL_DIR}
		${CXXKitWrapZLIB_SOURCE_DIR}
		WORKING_DIRECTORY "${CXXKitWrapZLIB_BUILD_DIR}"
		RESULT_VARIABLE CONFIGURE_RESULT)
	if(NOT CONFIGURE_RESULT MATCHES 0)
		message(FATAL_ERROR "${CXXKitWrapZLIB_DIR_NAME} configure failed.")
	endif()
	message(STATUS "${CXXKitWrapZLIB_DIR_NAME} configure success")

	execute_process(
		COMMAND ${CMAKE_COMMAND} --build ./ --parallel ${CXXKIT_NUMBER_OF_ASYNC_JOBS} --config
		${CMAKE_BUILD_TYPE} --target install
		WORKING_DIRECTORY "${CXXKitWrapZLIB_BUILD_DIR}"
		RESULT_VARIABLE BUILD_RESULT)
	if(NOT BUILD_RESULT MATCHES 0)
		message(FATAL_ERROR "${CXXKitWrapZLIB_DIR_NAME} build failed.")
	endif()
	message(STATUS "${CXXKitWrapZLIB_DIR_NAME} build success")

	execute_process(
		COMMAND ${CMAKE_COMMAND} --install ./ --config ${CMAKE_BUILD_TYPE}
		WORKING_DIRECTORY "${CXXKitWrapZLIB_BUILD_DIR}"
		RESULT_VARIABLE INSTALL_RESULT)
	if(NOT INSTALL_RESULT MATCHES 0)
		message(FATAL_ERROR "${CXXKitWrapZLIB_DIR_NAME} install failed.")
	endif()
	message(STATUS "${CXXKitWrapZLIB_DIR_NAME} install success")
	cxxkit_make_stamp_file("${CXXKitWrapZLIB_STAMP_FILE_PATH}")
endif()
# wrap lib
add_library(CXXKitWrapZLIB::WrapZLIB INTERFACE IMPORTED)
cxxkit_pkgconf_check_modules(ZLIB REQUIRED
	PATH "${CXXKitWrapZLIB_INSTALL_DIR}/share/pkgconfig"
	IMPORTED_TARGET zlib)
target_link_libraries(CXXKitWrapZLIB::WrapZLIB INTERFACE PkgConfig::ZLIB)
set(CXXKitWrapZLIB_FOUND ON)