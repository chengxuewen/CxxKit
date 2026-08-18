########################################################################################################################
#
# Library: cxxkit
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

add_library(cxxkit_text
    ${PROJECT_SOURCE_DIR}/src/text/string_utils.cpp
    ${PROJECT_SOURCE_DIR}/src/text/ascii.cpp
    ${PROJECT_SOURCE_DIR}/src/text/string.cpp)
add_library(cxxkit::text ALIAS cxxkit_text)

target_include_directories(cxxkit_text PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(cxxkit_text PUBLIC cxxkit::base)
target_compile_features(cxxkit_text PUBLIC cxx_std_11)
set_target_properties(cxxkit_text PROPERTIES
    CXX_STANDARD ${CXXKIT_FEATURE_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)

cxxkit_configure_library_end(CxxKitText)

if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_text EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/text/
        DESTINATION include/cxxkit/text
        FILES_MATCHING PATTERN "*.hpp")
endif()
