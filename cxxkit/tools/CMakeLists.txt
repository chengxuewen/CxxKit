########################################################################################################################
#
# Library: cxxkit
#
# Copyright (C) 2025~Present ChengXueWen.
#
# License: MIT License
#
# cxxkit::tools — compiled tools sublibrary.
# Phase 1 scope: logging/random/assert cpp only; remaining cpp migrate in Phase 2.
#
########################################################################################################################

cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

cxxkit_configure_library_begin(CxxKitTools)

add_library(cxxkit_tools
    ${PROJECT_SOURCE_DIR}/src/tools/logging.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/random.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/assert.cpp)
add_library(cxxkit::tools ALIAS cxxkit_tools)

target_include_directories(cxxkit_tools PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(cxxkit_tools PUBLIC cxxkit::base cxxkit::text)
target_compile_features(cxxkit_tools PUBLIC cxx_std_11)
set_target_properties(cxxkit_tools PROPERTIES
    CXX_STANDARD ${CXXKIT_FEATURE_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)

cxxkit_configure_library_end(CxxKitTools)

if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_tools EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/tools/
        DESTINATION include/cxxkit/tools
        FILES_MATCHING PATTERN "*.hpp")
endif()
