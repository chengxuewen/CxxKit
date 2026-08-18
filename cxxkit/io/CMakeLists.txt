cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

cxxkit_configure_library_begin(CxxKitIo)
add_library(cxxkit_io ${PROJECT_SOURCE_DIR}/src/io/file_wrapper.cpp)
add_library(cxxkit::io ALIAS cxxkit_io)
target_include_directories(cxxkit_io PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(cxxkit_io PUBLIC cxxkit::base)
target_compile_features(cxxkit_io PUBLIC cxx_std_11)
set_target_properties(cxxkit_io PROPERTIES
    CXX_STANDARD ${CXXKIT_FEATURE_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)
cxxkit_configure_library_end(CxxKitIo)

if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_io EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/io/
        DESTINATION include/cxxkit/io
        FILES_MATCHING PATTERN "*.hpp")
endif()
