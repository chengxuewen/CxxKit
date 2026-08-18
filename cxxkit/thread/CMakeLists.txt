########################################################################################################################
#
# Library: cxxkit
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
    ${PROJECT_SOURCE_DIR}/src/thread/concurrent.cpp
    ${PROJECT_SOURCE_DIR}/src/thread/context_checker.cpp
    ${PROJECT_SOURCE_DIR}/src/thread/event_loop_thread.cpp
    ${PROJECT_SOURCE_DIR}/src/thread/future.cpp
    ${PROJECT_SOURCE_DIR}/src/thread/platform_thread.cpp
    ${PROJECT_SOURCE_DIR}/src/thread/race_checker.cpp
    ${PROJECT_SOURCE_DIR}/src/thread/repeating_task.cpp
    ${PROJECT_SOURCE_DIR}/src/thread/task.cpp
    ${PROJECT_SOURCE_DIR}/src/thread/task_queue.cpp
    ${PROJECT_SOURCE_DIR}/src/thread/task_queue_factory.cpp
    ${PROJECT_SOURCE_DIR}/src/thread/task_queue_thread.cpp
    ${PROJECT_SOURCE_DIR}/src/thread/thread_pool.cpp)

if(NOT WIN32)
    list(APPEND CXXKIT_THREAD_SOURCES ${PROJECT_SOURCE_DIR}/src/thread/platform_thread_posix.cpp)
else()
    list(APPEND CXXKIT_THREAD_SOURCES ${PROJECT_SOURCE_DIR}/src/thread/platform_thread_win.cpp)
endif()

add_library(cxxkit_thread ${CXXKIT_THREAD_SOURCES})
add_library(cxxkit::thread ALIAS cxxkit_thread)

target_include_directories(cxxkit_thread PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(cxxkit_thread PUBLIC cxxkit::base cxxkit::kernel cxxkit::memory cxxkit::tools cxxkit::units cxxkit::time)
target_compile_features(cxxkit_thread PUBLIC cxx_std_11)
set_target_properties(cxxkit_thread PROPERTIES
    EXPORT_NAME thread
    CXX_STANDARD ${CXXKIT_FEATURE_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)

cxxkit_generate_pkg_config(cxxkit_thread thread)
cxxkit_configure_library_end(CxxKitThread)

set_property(TARGET cxxkit_thread PROPERTY FOLDER "cxxkit/libs")
if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_thread EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/thread/
        DESTINATION include/cxxkit/thread
        FILES_MATCHING PATTERN "*.hpp")
endif()
