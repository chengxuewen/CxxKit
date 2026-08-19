cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

cxxkit_configure_library_begin(CxxKitUnits)

file(GLOB _cxxkit_headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/detail/*.hpp)
add_library(cxxkit_units
    ${_cxxkit_headers}
    data_rate.cpp
    data_size.cpp
    frequency.cpp
    time_delta.cpp
    timestamp.cpp)
add_library(cxxkit::units ALIAS cxxkit_units)

target_include_directories(cxxkit_units PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(cxxkit_units PUBLIC cxxkit::base cxxkit::numerics cxxkit::containers cxxkit::tools)
target_compile_features(cxxkit_units PUBLIC cxx_std_11)
set_target_properties(cxxkit_units PROPERTIES
    EXPORT_NAME units
    CXX_STANDARD ${CXXKIT_FEATURE_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)

cxxkit_generate_pkg_config(cxxkit_units units)
cxxkit_configure_library_end(CxxKitUnits)

set_property(TARGET cxxkit_units PROPERTY FOLDER "CxxKit/libs")
if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_units EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/units/
        DESTINATION include/cxxkit/units
        FILES_MATCHING PATTERN "*.hpp" PATTERN "detail" EXCLUDE)
endif()
