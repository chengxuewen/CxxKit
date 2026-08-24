########################################################################################################################
#
# Library: CxxKit
#
# Copyright (C) 2026~Present ChengXueWen.
#
# License: MIT License
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
# documentation files (the "Software"), to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
# to permit persons to whom the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies or substantial portions
# of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
# OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
########################################################################################################################

cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})
find_package(Threads REQUIRED)

cxxkit_find_package(GTest PROVIDED_TARGETS CxxKitWrapGTest::WrapGTest)
cxxkit_find_package(Benchmark PROVIDED_TARGETS CxxKitWrapBenchmark::WrapBenchmark)

set(CXXKIT_TEST_LINK_LIBRARIES
    cxxkit::base
    cxxkit::containers
    cxxkit::functional
    cxxkit::numerics
    cxxkit::patterns
    cxxkit::memory
    cxxkit::units
    cxxkit::time
    cxxkit::kernel
    cxxkit::thread
    cxxkit::text
    cxxkit::tools
    CxxKitWrapGTest::WrapGTest
    CxxKitWrapBenchmark::WrapBenchmark)

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

# Phase 2: remaining OpenCTK-enabled tests (30 total, 28 added here)
cxxkit_add_test(cxxkit_tst_bit_buffer
    SOURCES tst_bit_buffer.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_concurrent
    SOURCES tst_concurrent.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_context_checker
    SOURCES tst_context_checker.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_ctordtor
    SOURCES tst_ctordtor.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_elapsed_timer
    SOURCES tst_elapsed_timer.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_error
    SOURCES tst_error.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_exception
    SOURCES tst_exception.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_filesystem
    SOURCES tst_filesystem.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_move_wrapper
    SOURCES tst_move_wrapper.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_mutex
    SOURCES tst_mutex.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_mutex_benchmark
    SOURCES tst_mutex_benchmark.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_platform_thread
    SOURCES tst_platform_thread.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_repeating_task
    SOURCES tst_repeating_task.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_result
    SOURCES tst_result.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_safe_minmax
    SOURCES tst_safe_minmax.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_sanitizer
    SOURCES tst_sanitizer.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_scope_guard
    SOURCES tst_scope_guard.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_semaphore
    SOURCES tst_semaphore.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_shared_data
    SOURCES tst_shared_data.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_shared_pointer
    SOURCES tst_shared_pointer.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_source_location
    SOURCES tst_source_location.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_status
    SOURCES tst_status.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_string_builder
    SOURCES tst_string_builder.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_string_utils
    SOURCES tst_string_utils.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_task_queue_thread
    SOURCES tst_task_queue_thread.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_thread_pool
    SOURCES tst_thread_pool.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_type_traits
    SOURCES tst_type_traits.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_unique_function
    SOURCES tst_unique_function.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

cxxkit_add_test(cxxkit_tst_boundary
    SOURCES tst_boundary.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

# Coverage: low-coverage library .cpp files (raise library coverage toward 80%)
cxxkit_add_test(cxxkit_tst_ascii
    SOURCES tst_ascii.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_string
    SOURCES tst_string.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_datetime
    SOURCES tst_datetime.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_clock
    SOURCES tst_clock.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_logging
    SOURCES tst_logging.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
cxxkit_add_test(cxxkit_tst_assert
    SOURCES tst_assert.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

cxxkit_add_test(cxxkit_tst_once_flag
    SOURCES tst_once_flag.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

cxxkit_add_test(cxxkit_tst_task_queue_factory
    SOURCES tst_task_queue_factory.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

cxxkit_add_test(cxxkit_tst_race_checker
    SOURCES tst_race_checker.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

cxxkit_add_test(cxxkit_tst_fake_clock
    SOURCES tst_fake_clock.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

cxxkit_add_test(cxxkit_tst_id_registry
    SOURCES tst_id_registry.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

cxxkit_add_test(cxxkit_tst_shared_buffer
    SOURCES tst_shared_buffer.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

cxxkit_add_test(cxxkit_tst_string_encode
    SOURCES tst_string_encode.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

cxxkit_add_test(cxxkit_tst_base64
    SOURCES tst_base64.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

cxxkit_add_test(cxxkit_tst_metrics
    SOURCES tst_metrics.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

cxxkit_add_test(cxxkit_tst_random
    SOURCES tst_random.cpp
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})


if(CXXKIT_ENABLE_LIB_CRASH)
    # Standalone crash fixture: installs the handler and raises SIGSEGV in its own process.
    # Built without gtest; the parent tst_crash spawns it via fork+exec (never death tests —
    # breakpad intercepts the very signals death tests rely on).
    add_executable(crash_fixture crash_fixture.cpp)
    target_link_libraries(crash_fixture PRIVATE cxxkit::crash)
    target_include_directories(crash_fixture PRIVATE ${PROJECT_SOURCE_DIR})
    set_target_properties(crash_fixture PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF)
    set_property(TARGET crash_fixture PROPERTY FOLDER "CxxKit/tests")

    cxxkit_add_test(cxxkit_tst_crash
        SOURCES tst_crash.cpp
        INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
        LIBRARIES ${CXXKIT_TEST_LINK_LIBRARIES} cxxkit::crash ${CMAKE_THREAD_LIBS_INIT})
    target_compile_definitions(cxxkit_tst_crash PRIVATE TEST_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}")
    if(NOT APPLE AND NOT WIN32)
        # Give the spawned fixture a deterministic libdw.so.1 search path for the stack-trace test
        # (backward dlopens it by name; no system elfutils dependency assumed).
        target_compile_definitions(cxxkit_tst_crash PRIVATE
            TEST_ELFUTILS_LIB_DIR="${CxxKitWrapBackward_ElfUtils_INSTALL_DIR}/lib")
    endif()
endif()
