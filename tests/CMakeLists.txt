########################################################################################################################
#
# Library: cxxkit
#
# Copyright (C) 2025~Present ChengXueWen.
#
# License: MIT License
#
# Tests: gtest, one executable per test source.
# Test targets compile with C++14 (gtest 1.12.1 requirement); libraries stay C++11.
#
########################################################################################################################

cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})
find_package(Threads REQUIRED)

cxxkit_find_package(GTest PROVIDED_TARGETS CXXKitWrapGTest::WrapGTest)

set(CXXKIT_TEST_LINK_LIBRARIES
    cxxkit::base
    cxxkit::containers
    cxxkit::functional
    cxxkit::numerics
    cxxkit::patterns
    cxxkit::text
    cxxkit::tools
    CXXKitWrapGTest::WrapGTest)

cxxkit_add_test(cxxkit_tst_array_view
    SOURCES tst_array_view.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

# tst_inlined_vector.cpp deferred to Phase 2 (depends on absl test_instance_tracker, not vendored in OpenCTK either)

cxxkit_add_test(cxxkit_tst_divide_round
    SOURCES tst_divide_round.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

cxxkit_add_test(cxxkit_tst_checks
    SOURCES tst_checks.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

cxxkit_add_test(cxxkit_tst_enum_flags
    SOURCES tst_enum_flags.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

cxxkit_add_test(cxxkit_tst_function_view
    SOURCES tst_function_view.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

