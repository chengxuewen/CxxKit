########################################################################################################################
#
# Library: CxxKit
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

file(GLOB _cxxkit_headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/detail/*.hpp)
add_library(cxxkit_tools
    ${_cxxkit_headers}
    logging.cpp
    random.cpp
    assert.cpp
    clock.cpp
    status.cpp
    error.cpp
    once_flag.cpp
    id_registry.cpp
    shared_buffer.cpp
    metrics.cpp
    ntp_time.cpp
    fake_clock.cpp)
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

set_property(TARGET cxxkit_tools PROPERTY FOLDER "CxxKit/libs")
if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_tools EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/tools/
        DESTINATION include/cxxkit/tools
        FILES_MATCHING PATTERN "*.hpp")
endif()
