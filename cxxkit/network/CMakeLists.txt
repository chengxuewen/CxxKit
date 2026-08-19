########################################################################################################################
#
# Library: CxxKit
#
# Copyright (C) 2025~Present ChengXueWen.
#
# License: MIT License
#
# cxxkit::network — HTTP sublibrary (cpr backend, vendored).
#
########################################################################################################################

cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

cxxkit_configure_library_begin(CxxKitNetwork)

cxxkit_find_package(Libcpr PROVIDED_TARGETS CXXKitWrapLibcpr::WrapLibcpr)

file(GLOB _cxxkit_headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/detail/*.hpp)
add_library(cxxkit_network
    ${_cxxkit_headers}
    http.cpp)
add_library(cxxkit::network ALIAS cxxkit_network)

target_include_directories(cxxkit_network PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(cxxkit_network PUBLIC cxxkit::base cxxkit::text cxxkit::thread cxxkit::memory cxxkit::tools CXXKitWrapLibcpr::WrapLibcpr)
target_compile_features(cxxkit_network PUBLIC cxx_std_11)
set_target_properties(cxxkit_network PROPERTIES
    EXPORT_NAME network
    CXX_STANDARD ${CXXKIT_FEATURE_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)

cxxkit_install_public_wrap_headers(cxxkit_network
    WRAPS
    CXXKitWrapLibcpr::WrapLibcpr|cpr
    CXXKitWrapLibcurl::WrapLibcurl|curl
    CXXKitWrapMbedTLS::WrapMbedTLS|mbedtls
)

cxxkit_generate_pkg_config(cxxkit_network network)
cxxkit_configure_library_end(CxxKitNetwork)

set_property(TARGET cxxkit_network PROPERTY FOLDER "CxxKit/libs")
if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_network EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/network/
        DESTINATION include/cxxkit/network
        FILES_MATCHING PATTERN "*.hpp")
endif()
