# Print elements of a list line by line.
function(shuhai_print_list)
    foreach(VALUE ${ARGN})
        message(${VALUE})
    endforeach()
endfunction()

# Print all cmake variable name and its value if no arguments provided.
# Print all cmake variable that matches the specified name.
function(shuhai_print_cmake_variables)
    get_cmake_property(VAR_NAMES VARIABLES)
    list(SORT VAR_NAMES)
    foreach(VAR_NAME ${VAR_NAMES})
        if(ARGV0)
            unset(MATCHED)
            string(REGEX MATCH ${ARGV0} MATCHED ${VAR_NAME})
            if(NOT MATCHED)
                continue()
            endif()
        endif()
        message(STATUS "${VAR_NAME}=${${VAR_NAME}}")
    endforeach()
endfunction()

# Print the specified property of given target.
function(shuhai_print_target_property TARGET PROPERTY)
    get_property(PROP_EXIST TARGET ${TARGET} PROPERTY ${PROPERTY} SET)
    if(NOT PROP_EXIST)
        return()
    endif()

    get_target_property(PROP_VALUE ${TARGET} ${PROPERTY})
    message(STATUS "${TARGET} ${PROPERTY}=${PROP_VALUE}")
endfunction()

# Print all properties of the given target.
function(shuhai_print_target_properties TARGET)
    if(NOT TARGET ${TARGET})
        message(FATAL_ERROR "There is no target named '${TARGET}'")
    endif()

    get_target_property(IMPORTED ${TARGET} IMPORTED)
    shuhai_get_cmake_property_list(PROPERTY_LIST)
    foreach(PROPERTY ${PROPERTY_LIST})
        if(NOT ${IMPORTED} AND (PROPERTY STREQUAL "LOCATION" OR PROPERTY MATCHES "^LOCATION_" OR PROPERTY MATCHES "_LOCATION$"))
            continue()
        endif()

        string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_UPPER)
        string(REPLACE "<CONFIG>" "${CMAKE_BUILD_TYPE_UPPER}" PROPERTY ${PROPERTY})

        string(FIND ${PROPERTY} "<LANG>" LANG_POS)
        if(${LANG_POS} GREATER_EQUAL 0)
            get_property(LANGUAGES GLOBAL PROPERTY ENABLED_LANGUAGES)
            foreach(LANG ${LANGUAGES})
                string(REPLACE "<LANG>" ${LANG} PROPERTY_WITH_LANGUAGE ${PROPERTY})
                shuhai_print_target_property(${TARGET} ${PROPERTY_WITH_LANGUAGE})
            endforeach()
        else()
            shuhai_print_target_property(${TARGET} ${PROPERTY})
        endif()
    endforeach()
endfunction()

# Get a list that contains all properties of the given target.
function(shuhai_get_cmake_property_list VAR)
    execute_process(COMMAND cmake --help-property-list OUTPUT_VARIABLE PROPERTY_LIST)
    string(REGEX REPLACE ";" "\\\\;" PROPERTY_LIST "${PROPERTY_LIST}")
    string(REGEX REPLACE "\n" ";" PROPERTY_LIST "${PROPERTY_LIST}")
    set(${VAR} ${PROPERTY_LIST} PARENT_SCOPE)
endfunction()
