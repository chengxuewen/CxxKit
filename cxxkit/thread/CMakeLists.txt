########################################################################################################################
#
# Library: CxxKit
#
# Copyright (C) 2025~Present ChengXueWen.
#
# License: MIT License
#
# cxxkit::thread — threading sublibrary (thread_pool, task_queue, event_loop, future, semaphore).
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

file(GLOB _cxxkit_headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/detail/*.hpp)
add_library(cxxkit_thread ${_cxxkit_headers} ${CXXKIT_THREAD_SOURCES})
add_library(cxxkit::thread ALIAS cxxkit_thread)

target_include_directories(cxxkit_thread PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(cxxkit_thread PUBLIC cxxkit::base cxxkit::functional cxxkit::kernel cxxkit::memory cxxkit::tools cxxkit::units cxxkit::time)
target_compile_features(cxxkit_thread PUBLIC cxx_std_11)
set_target_properties(cxxkit_thread PROPERTIES
    EXPORT_NAME thread
    CXX_STANDARD ${CXXKIT_FEATURE_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)

cxxkit_generate_pkg_config(cxxkit_thread thread)
cxxkit_configure_library_end(CxxKitThread)

set_property(TARGET cxxkit_thread PROPERTY FOLDER "CxxKit/libs")
if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_thread EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/thread/
        DESTINATION include/cxxkit/thread
        FILES_MATCHING PATTERN "*.hpp")
endif()
