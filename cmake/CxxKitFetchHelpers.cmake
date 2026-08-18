########################################################################################################################
#
# Library: cxxkit
#
# Copyright (C) 2025~Present ChengXueWen.
#
# License: MIT License
#
# Fetch helpers: extract vendored 3rdparty archives into the build tree with stamp-based
# "already built" detection. More robust than FetchContent (survives build dir deletion).
# Ported from OpenCTK OpenCTKCMakeHelpers.cmake (octk_fetch_3rdparty / stamp / reset_dir).
#
########################################################################################################################

function(cxxkit_reset_dir DIR)
    get_filename_component(WORKING_DIR ${DIR} DIRECTORY)
    while(NOT EXISTS "${WORKING_DIR}")
        get_filename_component(WORKING_DIR ${WORKING_DIR} DIRECTORY)
    endwhile()
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${DIR}"
        WORKING_DIRECTORY "${WORKING_DIR}"
        RESULT_VARIABLE RMDIR_RESULT)
    if(NOT (RMDIR_RESULT MATCHES 0))
        message(FATAL_ERROR "${DIR} dir remove failed.")
    endif()
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DIR}"
        WORKING_DIRECTORY "${WORKING_DIR}"
        RESULT_VARIABLE MKDIR_RESULT)
    if(NOT (MKDIR_RESULT MATCHES 0))
        message(FATAL_ERROR "${DIR} dir create failed.")
    endif()
endfunction()

function(cxxkit_stamp_file_info base_name)
    cxxkit_parse_all_arguments(arg "" "" "SUFFIX;OUTPUT_DIR" "" ${ARGN})
    if("${arg_OUTPUT_DIR}" STREQUAL "")
        set(arg_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
    endif()
    if("${arg_SUFFIX}" STREQUAL "")
        set("${base_name}_STAMP_FILE_NAME" "${base_name}-stamp.txt" PARENT_SCOPE)
        set("${base_name}_STAMP_FILE_PATH" "${arg_OUTPUT_DIR}/${base_name}-stamp.txt" PARENT_SCOPE)
    else()
        string(TOLOWER "${arg_SUFFIX}" LOWER_SUFFIX)
        set("${base_name}_${arg_SUFFIX}_STAMP_FILE_NAME" "${base_name}-${LOWER_SUFFIX}-stamp.txt" PARENT_SCOPE)
        set("${base_name}_${arg_SUFFIX}_STAMP_FILE_PATH" "${arg_OUTPUT_DIR}/${base_name}-${LOWER_SUFFIX}-stamp.txt" PARENT_SCOPE)
    endif()
endfunction()

function(cxxkit_make_stamp_file file_path)
    message(STATUS "Creating ${file_path} ...")
    string(TIMESTAMP CURRENT_TIMESTAMP "%Y-%m-%d %H:%M:%S")
    file(WRITE "${file_path}" "${CURRENT_TIMESTAMP}")
endfunction()

# cxxkit_fetch_3rdparty(name URL [OUTPUT_NAME name] [OUTPUT_DIR dir])
# Extracts a vendored archive from ${PROJECT_SOURCE_DIR}/3rdparty into the build tree.
function(cxxkit_fetch_3rdparty name)
    cxxkit_parse_all_arguments(arg "" "" "OUTPUT_NAME;OUTPUT_DIR" "URL" ${ARGN})
    if(NOT arg_URL)
        message(FATAL_ERROR "cxxkit_fetch_3rdparty ${name} failed: URL not given.")
    endif()
    if("${arg_OUTPUT_DIR}" STREQUAL "")
        set(arg_OUTPUT_DIR "${PROJECT_BINARY_DIR}/3rdparty")
    endif()
    if(NOT EXISTS "${arg_URL}")
        message(FATAL_ERROR "3rdparty ${name} fetch failed, url ${arg_URL} not exist.")
    endif()
    get_filename_component(url_base_name "${arg_URL}" NAME_WE)
    if("${arg_OUTPUT_NAME}" STREQUAL "")
        set(3rdparty_root_dir ${arg_OUTPUT_DIR}/${url_base_name})
    else()
        set(3rdparty_root_dir ${arg_OUTPUT_DIR}/${arg_OUTPUT_NAME})
    endif()
    set("${name}_FETCH_STAMP_FILE_PATH" "${3rdparty_root_dir}/${name}-fetch-stamp.txt" PARENT_SCOPE)
    if(NOT EXISTS "${3rdparty_root_dir}/source")
        message(STATUS "Fetching 3rdparty ${name} from ${arg_URL} ...")
        file(MAKE_DIRECTORY "${3rdparty_root_dir}/source")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xzf "${arg_URL}"
            WORKING_DIRECTORY "${3rdparty_root_dir}/source"
            RESULT_VARIABLE EXTRACT_RESULT)
        if(NOT (EXTRACT_RESULT MATCHES 0))
            message(FATAL_ERROR "3rdparty ${name} extract failed.")
        endif()
        # Handle single top-level directory (most archives) by flattening one level
        file(GLOB _entries "${3rdparty_root_dir}/source/*")
        list(LENGTH _entries _count)
        if(_count EQUAL 1)
            get_filename_component(_single "${_entries}" ABSOLUTE)
            if(IS_DIRECTORY "${_single}")
                file(GLOB _inner "${_single}/*")
                file(COPY ${_inner} DESTINATION "${3rdparty_root_dir}/source/")
                file(REMOVE_RECURSE "${_single}")
            endif()
        endif()
        file(WRITE "${${name}_FETCH_STAMP_FILE_PATH}" "${CMAKE_TIMESTAMP}")
    endif()
endfunction()
