########################################################################################################################
#
# Library: cxxkit
#
# Copyright (C) 2025~Present ChengXueWen.
#
# License: MIT License
#
# Examples: one executable per example source, FOLDER cxxkit/examples.
#
########################################################################################################################

cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

set(CXXKIT_EXAMPLE_LINK_LIBRARIES
    cxxkit::base cxxkit::tools cxxkit::text)

function(cxxkit_add_example name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE ${CXXKIT_EXAMPLE_LINK_LIBRARIES})
    target_include_directories(${name} PRIVATE ${PROJECT_SOURCE_DIR})
    set_target_properties(${name} PROPERTIES
        FOLDER "cxxkit/examples"
        CXX_STANDARD ${CXXKIT_FEATURE_CXX_STANDARD}
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF)
endfunction()

cxxkit_add_example(cxxkit_exp_core_version exp_core_version.cpp)
cxxkit_add_example(cxxkit_exp_logging exp_logging.cpp)

if(CXXKIT_ENABLE_LIB_NETWORK)
    cxxkit_add_example(cxxkit_exp_network_version exp_network_version.cpp)
    target_link_libraries(cxxkit_exp_network_version PRIVATE cxxkit::network)
endif()
