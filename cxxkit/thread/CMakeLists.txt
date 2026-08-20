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

cxxkit_configure_library_begin(CxxKitThread)

set(CXXKIT_THREAD_SOURCES
    concurrent.cpp
    context_checker.cpp
    event_loop_thread.cpp
    future.cpp
    platform_thread.cpp
    race_checker.cpp
    repeating_task.cpp
    task.cpp
    task_queue.cpp
    task_queue_factory.cpp
    task_queue_thread.cpp
    thread_pool.cpp)

if(NOT WIN32)
    list(APPEND CXXKIT_THREAD_SOURCES platform_thread_posix.cpp)
else()
    list(APPEND CXXKIT_THREAD_SOURCES platform_thread_win.cpp)
endif()

cxxkit_add_library(cxxkit_thread
    STATIC
    NO_ALIAS
    SOURCES ${CXXKIT_THREAD_SOURCES}
    PUBLIC_LIBRARIES cxxkit::base cxxkit::functional cxxkit::kernel cxxkit::memory cxxkit::tools cxxkit::units cxxkit::time
    EXPORT_NAME thread
    FOLDER "CxxKit/libs")
add_library(cxxkit::thread ALIAS cxxkit_thread)

cxxkit_generate_pkg_config(cxxkit_thread thread)
cxxkit_configure_library_end(CxxKitThread)

if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_thread EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/thread/
        DESTINATION include/cxxkit/thread
        FILES_MATCHING PATTERN "*.hpp")
endif()
