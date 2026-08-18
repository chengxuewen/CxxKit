cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

cxxkit_configure_library_begin(CxxKitTime)

add_library(cxxkit_time
    ${PROJECT_SOURCE_DIR}/src/time/date_time.cpp
    ${PROJECT_SOURCE_DIR}/src/time/elapsed_timer.cpp)
add_library(cxxkit::time ALIAS cxxkit_time)

target_include_directories(cxxkit_time PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(cxxkit_time PUBLIC cxxkit::base cxxkit::text cxxkit::tools cxxkit::units)
target_compile_features(cxxkit_time PUBLIC cxx_std_11)
set_target_properties(cxxkit_time PROPERTIES
    EXPORT_NAME time
    CXX_STANDARD ${CXXKIT_FEATURE_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)

cxxkit_generate_pkg_config(cxxkit_time time)
cxxkit_configure_library_end(CxxKitTime)

if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_time EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/time/
        DESTINATION include/cxxkit/time
        FILES_MATCHING PATTERN "*.hpp")
endif()
