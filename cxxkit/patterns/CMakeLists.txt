cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

cxxkit_configure_library_begin(CxxKitPatterns)
file(GLOB _cxxkit_headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/detail/*.hpp)
add_library(cxxkit_patterns INTERFACE ${_cxxkit_headers})
add_library(cxxkit::patterns ALIAS cxxkit_patterns)
set_property(TARGET cxxkit_patterns PROPERTY EXPORT_NAME patterns)
target_include_directories(cxxkit_patterns INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(cxxkit_patterns INTERFACE cxxkit::base)
target_compile_features(cxxkit_patterns INTERFACE cxx_std_11)
cxxkit_generate_pkg_config(cxxkit_patterns patterns)
cxxkit_configure_library_end(CxxKitPatterns)

set_property(TARGET cxxkit_patterns PROPERTY FOLDER "CxxKit/libs")
if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_patterns EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/patterns/
        DESTINATION include/cxxkit/patterns
        FILES_MATCHING PATTERN "*.hpp")
endif()
