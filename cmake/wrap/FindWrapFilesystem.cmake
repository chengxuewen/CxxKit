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
if(TARGET CxxKitWrapFilesystem::WrapFilesystem)
	set(CxxKitWrapFilesystem_FOUND ON)
	return()
endif()

set(CxxKitWrapFilesystem_NAME "filesystem-1.5.14")
set(CxxKitWrapFilesystem_DIR_NAME "${CxxKitWrapFilesystem_NAME}")
set(CxxKitWrapFilesystem_PKG_NAME "${CxxKitWrapFilesystem_NAME}.tar.gz")
set(CxxKitWrapFilesystem_URL_PATH "${PROJECT_SOURCE_DIR}/3rdparty/${CxxKitWrapFilesystem_PKG_NAME}")
set(CxxKitWrapFilesystem_ROOT_DIR "${PROJECT_BINARY_DIR}/3rdparty/${CxxKitWrapFilesystem_DIR_NAME}")
set(CxxKitWrapFilesystem_BUILD_DIR "${CxxKitWrapFilesystem_ROOT_DIR}/build" CACHE INTERNAL "" FORCE)
set(CxxKitWrapFilesystem_SOURCE_DIR "${CxxKitWrapFilesystem_ROOT_DIR}/source" CACHE INTERNAL "" FORCE)
set(CxxKitWrapFilesystem_INSTALL_DIR "${CxxKitWrapFilesystem_ROOT_DIR}/install" CACHE INTERNAL "" FORCE)
cxxkit_stamp_file_info(CxxKitWrapFilesystem OUTPUT_DIR "${CxxKitWrapFilesystem_ROOT_DIR}")
cxxkit_fetch_3rdparty(CxxKitWrapFilesystem URL "${CxxKitWrapFilesystem_URL_PATH}")
if(NOT EXISTS "${CxxKitWrapFilesystem_STAMP_FILE_PATH}")
	if(NOT EXISTS ${CxxKitWrapFilesystem_SOURCE_DIR})
		message(FATAL_ERROR "${CxxKitWrapFilesystem_DIR_NAME} FetchContent failed.")
	endif()
	cxxkit_reset_dir(${CxxKitWrapFilesystem_BUILD_DIR})

	message(STATUS "Configure ${CxxKitWrapFilesystem_DIR_NAME} lib...")
	execute_process(
		COMMAND ${CMAKE_COMMAND}
        -Wno-deprecated
        --no-warn-unused-cli
		-G ${CMAKE_GENERATOR}
		-DGHC_FILESYSTEM_BUILD_TESTING=OFF
		-DGHC_FILESYSTEM_BUILD_EXAMPLES=OFF
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
		-DCMAKE_INSTALL_PREFIX=${CxxKitWrapFilesystem_INSTALL_DIR}
		${CxxKitWrapFilesystem_SOURCE_DIR}
		WORKING_DIRECTORY "${CxxKitWrapFilesystem_BUILD_DIR}"
		RESULT_VARIABLE CONFIGURE_RESULT)
	if(NOT CONFIGURE_RESULT MATCHES 0)
		message(FATAL_ERROR "${CxxKitWrapFilesystem_DIR_NAME} configure failed.")
	endif()
	message(STATUS "${CxxKitWrapFilesystem_DIR_NAME} configure success")

	execute_process(
		COMMAND ${CMAKE_COMMAND} --build ./ --parallel ${CXXKIT_NUMBER_OF_ASYNC_JOBS} --config
		${CMAKE_BUILD_TYPE} --target install
		WORKING_DIRECTORY "${CxxKitWrapFilesystem_BUILD_DIR}"
		RESULT_VARIABLE BUILD_RESULT)
	if(NOT BUILD_RESULT MATCHES 0)
		message(FATAL_ERROR "${CxxKitWrapFilesystem_DIR_NAME} build failed.")
	endif()
	message(STATUS "${CxxKitWrapFilesystem_DIR_NAME} build success")

	execute_process(
		COMMAND ${CMAKE_COMMAND} --install ./ --config ${CMAKE_BUILD_TYPE}
		WORKING_DIRECTORY "${CxxKitWrapFilesystem_BUILD_DIR}"
		RESULT_VARIABLE INSTALL_RESULT)
	if(NOT INSTALL_RESULT MATCHES 0)
		message(FATAL_ERROR "${CxxKitWrapFilesystem_DIR_NAME} install failed.")
	endif()
	message(STATUS "${CxxKitWrapFilesystem_DIR_NAME} install success")
	cxxkit_make_stamp_file("${CxxKitWrapFilesystem_STAMP_FILE_PATH}")
endif()
# wrap lib
add_library(CxxKitWrapFilesystem::WrapFilesystem INTERFACE IMPORTED)
find_package(ghc_filesystem PATHS ${CxxKitWrapFilesystem_INSTALL_DIR} NO_DEFAULT_PATH REQUIRED)
target_link_libraries(CxxKitWrapFilesystem::WrapFilesystem INTERFACE ghcFilesystem::ghc_filesystem)
set(CxxKitWrapFilesystem_FOUND ON)