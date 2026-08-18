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
# Supports .7z / .zip / .tar.gz / .tar.xz / .tar.bz2 and directory sources.
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

    get_filename_component(url_name "${arg_URL}" NAME)
    set(is_dir FALSE)
    string(REGEX MATCH "\\.7z$" is_7z ${url_name})
    string(REGEX MATCH "\\.zip$" is_zip ${url_name})
    string(REGEX MATCH "\\.tar\\.gz$" is_tar_gz ${url_name})
    string(REGEX MATCH "\\.tar\\.xz$" is_tar_xz ${url_name})
    string(REGEX MATCH "\\.tar\\.bz2$" is_tar_bz2 ${url_name})
    if(is_7z)
        string(REGEX REPLACE "\\.7z$" "" url_base_name ${url_name})
    elseif(is_zip)
        string(REGEX REPLACE "\\.zip$" "" url_base_name ${url_name})
    elseif(is_tar_gz)
        string(REGEX REPLACE "\\.tar\\.gz$" "" url_base_name ${url_name})
    elseif(is_tar_xz)
        string(REGEX REPLACE "\\.tar\\.xz$" "" url_base_name ${url_name})
    elseif(is_tar_bz2)
        string(REGEX REPLACE "\\.tar\\.bz2$" "" url_base_name ${url_name})
    elseif(IS_DIRECTORY "${arg_URL}")
        set(is_dir TRUE)
        set(url_base_name ${url_name})
    else()
        message(FATAL_ERROR "3rdparty ${name} fetch failed, url ${arg_URL} is an unknown format compressed package")
    endif()

    if("${arg_OUTPUT_NAME}" STREQUAL "")
        set(3rdparty_root_dir ${arg_OUTPUT_DIR}/${url_base_name})
    else()
        set(3rdparty_root_dir ${arg_OUTPUT_DIR}/${arg_OUTPUT_NAME})
    endif()
    set(_stamp_file_path "${3rdparty_root_dir}/${name}-fetch-stamp.txt")
    set("${name}_FETCH_STAMP_FILE_PATH" "${_stamp_file_path}" PARENT_SCOPE)
    if(NOT EXISTS "${_stamp_file_path}")
        message(STATUS "Fetching 3rdparty ${name} from ${arg_URL} ...")
        if(is_dir)
            execute_process(
                COMMAND ${CMAKE_COMMAND} -E copy_directory "${arg_URL}" "${3rdparty_root_dir}/source"
                RESULT_VARIABLE COPYDIR_RESULT)
            if(NOT COPYDIR_RESULT MATCHES 0)
                message(FATAL_ERROR "3rdparty ${name} fetch failed, source directory copy failed.")
            endif()
        else()
            execute_process(
                COMMAND ${CMAKE_COMMAND} -E remove_directory "${3rdparty_root_dir}/extract"
                RESULT_VARIABLE RMDIR_RESULT)
            execute_process(
                COMMAND ${CMAKE_COMMAND} -E make_directory "${3rdparty_root_dir}/extract"
                RESULT_VARIABLE MKDIR_RESULT)
            if(NOT MKDIR_RESULT MATCHES 0)
                message(FATAL_ERROR "3rdparty ${name} fetch failed, extract directory create failed.")
            endif()

            execute_process(
                COMMAND ${CMAKE_COMMAND} -E tar xzvf "${arg_URL}"
                WORKING_DIRECTORY "${3rdparty_root_dir}/extract"
                RESULT_VARIABLE EXTRACT_RESULT)
            if(NOT EXTRACT_RESULT EQUAL 0)
                message(FATAL_ERROR "3rdparty ${name} fetch failed, ${arg_URL} extract failed.")
            endif()

            # Handle top-level structure: if exactly one directory (most archives) use it,
            # otherwise keep extract contents as-is.
            file(GLOB extracted_dirs RELATIVE "${3rdparty_root_dir}/extract" "${3rdparty_root_dir}/extract/*")
            set(3rdparty_extracted_dir "${3rdparty_root_dir}/extract")
            set(dir_count 0)
            foreach(subdir ${extracted_dirs})
                if(IS_DIRECTORY "${3rdparty_root_dir}/extract/${subdir}")
                    math(EXPR dir_count "${dir_count} + 1")
                    set(last_dir "${3rdparty_root_dir}/extract/${subdir}")
                endif()
            endforeach()
            if(dir_count EQUAL 1)
                set(3rdparty_extracted_dir "${last_dir}")
            endif()

            execute_process(
                COMMAND ${CMAKE_COMMAND} -E rename "${3rdparty_extracted_dir}" "${3rdparty_root_dir}/source"
                WORKING_DIRECTORY "${3rdparty_root_dir}"
                RESULT_VARIABLE EXTRACT_RESULT)
            execute_process(
                COMMAND ${CMAKE_COMMAND} -E remove_directory "${3rdparty_root_dir}/extract"
                RESULT_VARIABLE RMDIR_RESULT)
            if(NOT EXTRACT_RESULT EQUAL 0)
                message(FATAL_ERROR "3rdparty ${name} fetch failed, ${arg_URL} rename failed.")
            endif()
        endif()
        cxxkit_make_stamp_file("${_stamp_file_path}")
    endif()
endfunction()
