########################################################################################################################
#
# Library: CxxKit
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
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
# OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
########################################################################################################################

cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

set(CXXKIT_EXAMPLE_LINK_LIBRARIES
    cxxkit::base cxxkit::tools cxxkit::text)

cxxkit_add_executable(cxxkit_exp_core_version
    SOURCES exp_core_version.cpp
    LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES}
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    FOLDER "CxxKit/examples")
cxxkit_add_executable(cxxkit_exp_logging
    SOURCES exp_logging.cpp
    LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES}
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    FOLDER "CxxKit/examples")

if(CXXKIT_ENABLE_LIB_NETWORK)
    cxxkit_add_executable(cxxkit_exp_network_version
        SOURCES exp_network_version.cpp
        LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES}
        INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
        FOLDER "CxxKit/examples")
    target_link_libraries(cxxkit_exp_network_version PRIVATE cxxkit::network)
endif()

if(CXXKIT_ENABLE_LIB_TRACY)
    cxxkit_add_executable(cxxkit_exp_profiling
        SOURCES exp_profiling.cpp
        LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES}
        INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
        FOLDER "CxxKit/examples")
    target_link_libraries(cxxkit_exp_profiling PRIVATE cxxkit::profiling)
    # VM-friendly: skip invariant TSC check (real hardware unaffected)
    target_compile_definitions(cxxkit_exp_profiling PRIVATE TRACY_NO_INVARIANT_CHECK=1)
endif()
