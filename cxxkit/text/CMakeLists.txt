########################################################################################################################
#
# Library: CxxKit
#
# Copyright (C) 2025~Present ChengXueWen.
#
# License: MIT License
#
# cxxkit::text — text processing sublibrary.
# Phase 1 scope: string_utils/ascii cpp only; remaining cpp migrate in Phase 2.
#
########################################################################################################################

cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

cxxkit_configure_library_begin(CxxKitText)

file(GLOB _cxxkit_headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/detail/*.hpp)
add_library(cxxkit_text
    ${_cxxkit_headers}
    string_utils.cpp
    ascii.cpp
    string.cpp
    base64.cpp
    bit_buffer.cpp
    string_builder.cpp
    string_encode.cpp)
add_library(cxxkit::text ALIAS cxxkit_text)

target_include_directories(cxxkit_text PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(cxxkit_text PUBLIC cxxkit::base cxxkit::numerics)
target_compile_features(cxxkit_text PUBLIC cxx_std_11)
set_target_properties(cxxkit_text PROPERTIES
    EXPORT_NAME text
    CXX_STANDARD ${CXXKIT_FEATURE_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)

cxxkit_generate_pkg_config(cxxkit_text text)
cxxkit_configure_library_end(CxxKitText)

set_property(TARGET cxxkit_text PROPERTY FOLDER "CxxKit/libs")
if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_text EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/text/
        DESTINATION include/cxxkit/text
        FILES_MATCHING PATTERN "*.hpp")
endif()
