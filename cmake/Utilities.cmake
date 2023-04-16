function(shuhai_read_version_from_macro VAR)
    cmake_parse_arguments("ARG" # Variable prefix
            "VERBOSE" # Options
            "FILE;VERSION" # One-Value arguments
            "" # Multi-Value arguments
            ${ARGN})

    if(NOT ARG_FILE)
        message(FATAL_ERROR "Version file must be specified by 'FILE <path>' option.")
    endif()
    if(NOT EXISTS ${ARG_FILE})
        message(FATAL_ERROR "Version file '${ARG_FILE}' does not exists.")
    endif()

    if(NOT ARG_VERSION)
        message(FATAL_ERROR "Version macro must be specified by 'VERSION <macro>' option.")
    endif()

    if(ARG_VERBOSE)
        message(STATUS "Parsing version macro '${ARG_VERSION}' from file '${ARG_FILE}'.")
    endif()

    file(STRINGS ${ARG_FILE} _VER_LINE
            REGEX "^#define ${ARG_VERSION} *\"[0-9]+\\.[0-9]+\\.[0-9]+\""
            LIMIT_COUNT 1)
    string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+"
            VERSION "${_VER_LINE}")
    unset(_VER_LINE)

    set(${VAR} ${VERSION} PARENT_SCOPE)
endfunction()
