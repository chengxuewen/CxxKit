cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

cxxkit_configure_library_begin(CxxKitUnits)

add_library(cxxkit_units
    ${PROJECT_SOURCE_DIR}/src/units/data_rate.cpp
    ${PROJECT_SOURCE_DIR}/src/units/data_size.cpp
    ${PROJECT_SOURCE_DIR}/src/units/frequency.cpp
    ${PROJECT_SOURCE_DIR}/src/units/time_delta.cpp
    ${PROJECT_SOURCE_DIR}/src/units/timestamp.cpp)
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

set_property(TARGET cxxkit_units PROPERTY FOLDER "cxxkit/libs")
if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_units EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/units/
        DESTINATION include/cxxkit/units
        FILES_MATCHING PATTERN "*.hpp")
endif()
