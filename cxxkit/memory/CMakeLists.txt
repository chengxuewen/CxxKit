########################################################################################################################
#
# Library: cxxkit
#
# Copyright (C) 2025~Present ChengXueWen.
#
# License: MIT License
#
# cxxkit::memory — memory management sublibrary (aligned_malloc, shared_memory, smart pointers).
#
########################################################################################################################

cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

cxxkit_configure_library_begin(CxxKitMemory)

add_library(cxxkit_memory
    ${PROJECT_SOURCE_DIR}/src/memory/aligned_malloc.cpp
    ${PROJECT_SOURCE_DIR}/src/memory/shared_memory.cpp
    ${PROJECT_SOURCE_DIR}/src/memory/zero_memory.cpp)
add_library(cxxkit::memory ALIAS cxxkit_memory)

target_include_directories(cxxkit_memory PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(cxxkit_memory PUBLIC cxxkit::base)
target_compile_features(cxxkit_memory PUBLIC cxx_std_11)
set_target_properties(cxxkit_memory PROPERTIES
    EXPORT_NAME memory
    CXX_STANDARD ${CXXKIT_FEATURE_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)

cxxkit_configure_library_end(CxxKitMemory)

if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_memory EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/memory/
        DESTINATION include/cxxkit/memory
        FILES_MATCHING PATTERN "*.hpp")
endif()
