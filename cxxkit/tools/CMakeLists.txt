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
    ${PROJECT_SOURCE_DIR}/src/tools/assert.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/clock.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/status.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/error.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/once_flag.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/id_registry.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/shared_buffer.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/metrics.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ntp_time.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/fake_clock.cpp)
add_library(cxxkit::tools ALIAS cxxkit_tools)

target_include_directories(cxxkit_tools PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(cxxkit_tools PUBLIC cxxkit::base cxxkit::text)
target_compile_features(cxxkit_tools PUBLIC cxx_std_11)
set_target_properties(cxxkit_tools PROPERTIES
    EXPORT_NAME tools
    CXX_STANDARD ${CXXKIT_FEATURE_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)

cxxkit_generate_pkg_config(cxxkit_tools tools)
cxxkit_configure_library_end(CxxKitTools)

set_property(TARGET cxxkit_tools PROPERTY FOLDER "cxxkit/libs")
if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_tools EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/tools/
        DESTINATION include/cxxkit/tools
        FILES_MATCHING PATTERN "*.hpp")
endif()
