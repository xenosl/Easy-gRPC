include_guard(GLOBAL)

# Get specified target property or its corresponding configuration property with '_<CONFIG>' suffix,
function(shuhai_get_target_property VAR TARGET_NAME PROPERTY_NAME)
    get_target_property(PROP ${TARGET_NAME} ${PROPERTY_NAME})
    if(PROP)
        set(${VAR} ${PROP} PARENT_SCOPE)
        return()
    endif()

    string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE_UPPER)
    set(PROPERTY_NAME_CONFIG ${PROPERTY_NAME}_${BUILD_TYPE_UPPER})
    get_target_property(PROP ${TARGET_NAME} ${PROPERTY_NAME_CONFIG})
    if(PROP)
        set(${VAR} ${PROP} PARENT_SCOPE)
        return()
    endif()
endfunction()

# Get runtime file path of the specified target.
function(shuhai_get_target_runtime VAR TARGET)
    get_target_property(TYPE ${TARGET} TYPE)
    if(${TYPE} STREQUAL STATIC_LIBRARY OR ${TYPE} STREQUAL OBJECT_LIBRARY OR ${TYPE} STREQUAL INTERFACE_LIBRARY)
        return()
    endif()

    get_target_property(IMPORTED ${TARGET} IMPORTED)
    if(${IMPORTED})
        shuhai_get_target_property(PATH ${TARGET} IMPORTED_LOCATION)
        if(NOT PATH)
            shuhai_get_target_property(PATH ${TARGET} LOCATION)
        endif()
    else()
        shuhai_get_target_property(DIR ${TARGET} RUNTIME_OUTPUT_DIRECTORY)
        if(NOT DIR)
            get_target_property(BINARY_DIR ${TARGET} BINARY_DIR)
            set(DIR ${BINARY_DIR})
        endif()

        shuhai_get_target_property(FILENAME ${TARGET} RUNTIME_OUTPUT_NAME)
        if(NOT FILENAME)
            get_target_property(NAME ${TARGET} NAME) # Non-Alias name of the target
            set(FILENAME ${NAME})
        endif()

        get_target_property(PREFIX ${TARGET} PREFIX)
        if(${TYPE} STREQUAL SHARED_LIBRARY)
            if(NOT PREFIX)
                set(PREFIX ${CMAKE_STATIC_LIBRARY_PREFIX})
            endif()
        endif()
        if(PREFIX)
            set(FILENAME ${PREFIX}${FILENAME})
        endif()

        set(PATH ${DIR}/${FILENAME})
    endif()

    # Get all files with same name to ensure we got symbol links of so files such as *.so.xx.x on linux or *.pdb file on windows.
    if(PATH)
        get_filename_component(DIRECTORY ${PATH} DIRECTORY)
        get_filename_component(FILENAME ${PATH} NAME_WE)
        file(GLOB PATHS ${DIRECTORY}/${FILENAME}.*)
    endif()
    list(FILTER PATHS EXCLUDE REGEX "\\.a$")
    list(FILTER PATHS EXCLUDE REGEX "\\.lib$")

    set(${VAR} ${PATHS} PARENT_SCOPE)
endfunction()

function(shuhai_copy_after_build TARGET)
    cmake_parse_arguments("ARG" # Variable prefix
            "VERBOSE" # Options
            "DESTINATION" # One-Value arguments
            "PATH" # Multi-Value arguments
            ${ARGN})

    if(NOT ARG_PATH)
        message(FATAL_ERROR "Missing file/directory path(s) to copy.")
    endif()

    if(NOT ARG_DESTINATION)
        message(FATAL_ERROR "Missing destination directory.")
    endif()

    if(EXISTS ${ARG_DESTINATION} AND NOT IS_DIRECTORY ${ARG_DESTINATION})
        message(FATAL_ERROR "Destination path does not refer to a directory.")
    endif()

    foreach(PATH ${ARG_PATH})
        if(EXISTS ${PATH})
            list(APPEND PATHS ${PATH})
        endif()
    endforeach()

    if(PATHS)
        if(ARG_VERBOSE)
            message(STATUS "Copy after build for ${TARGET}:")
        endif()
        foreach(PATH ${PATHS})
            if(ARG_VERBOSE)
                message(STATUS "  ${PATH}")
            endif()
            if(IS_DIRECTORY ${PATH})
                get_filename_component(DIR_NAME ${PATH} NAME)
                set(COPY_DST ${ARG_DESTINATION}/${DIR_NAME})
                set(COPY_CMD "copy_directory")
            else()
                set(COPY_DST ${ARG_DESTINATION})
                set(COPY_CMD "copy")
            endif()
            add_custom_command(TARGET ${TARGET} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E ${COPY_CMD} ${PATH} ${COPY_DST}
                    COMMENT "Copy '${PATH}' to ${COPY_DST}")
        endforeach()
    endif()
endfunction()
