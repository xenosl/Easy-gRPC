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

# Workaround for copying symbolic link file which "cmake -E copy" do not support.
function(shuhai_copy_after_build TARGET SOURCES DESTINATION_DIR)
    if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
        foreach(SOURCE ${SOURCES})
            if(IS_SYMLINK ${SOURCE})
                #set(SYMLINK_LIST "${SYMLINK_LIST} \"${SOURCE}\"")
                add_custom_command(TARGET ${TARGET} POST_BUILD COMMAND cp -P ${SOURCE} ${DESTINATION_DIR})
            else()
                list(APPEND SOURCE_LIST ${SOURCE})
            endif()
        endforeach()
        add_custom_command(TARGET ${TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${SOURCE_LIST} ${DESTINATION_DIR})
    else()
        add_custom_command(TARGET ${TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${SOURCES} ${DESTINATION_DIR})
    endif()
endfunction()
