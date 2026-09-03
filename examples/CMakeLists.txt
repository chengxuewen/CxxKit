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
cxxkit_add_executable(cxxkit_exp_units
    SOURCES exp_units.cpp
    LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES} cxxkit::units cxxkit::time
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    FOLDER "CxxKit/examples")
cxxkit_add_executable(cxxkit_exp_text
    SOURCES exp_text.cpp
    LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES}
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    FOLDER "CxxKit/examples")
cxxkit_add_executable(cxxkit_exp_base
    SOURCES exp_base.cpp
    LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES}
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    FOLDER "CxxKit/examples")
cxxkit_add_executable(cxxkit_exp_time
    SOURCES exp_time.cpp
    LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES} cxxkit::time
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    FOLDER "CxxKit/examples")
cxxkit_add_executable(cxxkit_exp_patterns
    SOURCES exp_patterns.cpp
    LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES} cxxkit::patterns
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    FOLDER "CxxKit/examples")
cxxkit_add_executable(cxxkit_exp_memory
    SOURCES exp_memory.cpp
    LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES} cxxkit::memory
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    FOLDER "CxxKit/examples")
cxxkit_add_executable(cxxkit_exp_functional
    SOURCES exp_functional.cpp
    LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES} cxxkit::functional
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    FOLDER "CxxKit/examples")

cxxkit_add_executable(cxxkit_exp_containers
    SOURCES exp_containers.cpp
    LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES} cxxkit::containers
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    FOLDER "CxxKit/examples")


cxxkit_add_executable(cxxkit_exp_thread
    SOURCES exp_thread.cpp
    LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES} cxxkit::thread
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    FOLDER "CxxKit/examples")

cxxkit_add_executable(cxxkit_exp_numerics
    SOURCES exp_numerics.cpp
    LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES} cxxkit::numerics
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

cxxkit_add_executable(cxxkit_exp_profiling
    SOURCES exp_profiling.cpp
    LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES} cxxkit::profiling
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    FOLDER "CxxKit/examples")
# cxxkit_profiling is an unconditional no-op INTERFACE target; with CXXKIT_ENABLE_LIB_TRACY=ON its
# INTERFACE compile definitions activate the Tracy backend in this example automatically.
