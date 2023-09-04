function(asio_extract_version VAR VERSION_FILE)
    file(READ ${VERSION_FILE} _VERSION_HPP)
    string(REGEX MATCH "#define ASIO_VERSION ([0-9]+)" _VERSION_DEFINE ${_VERSION_HPP})

    set(ASIO_VERSION_NUMBER ${CMAKE_MATCH_1})
    math(EXPR ASIO_PATCH_VERSION ${ASIO_VERSION_NUMBER}%100)
    math(EXPR ASIO_MINOR_VERSION ${ASIO_VERSION_NUMBER}/100%1000)
    math(EXPR ASIO_MAYOR_VERSION ${ASIO_VERSION_NUMBER}/100000)
    set(${VAR} "${ASIO_MAYOR_VERSION}.${ASIO_MINOR_VERSION}.${ASIO_PATCH_VERSION}" PARENT_SCOPE)

    unset(_VERSION_HPP)
    unset(_VERSION_DEFINE)
endfunction()

find_path(ASIO_INCLUDE_DIR asio.hpp
        PATH_SUFFIXES include)

if(ASIO_INCLUDE_DIR)
    cmake_path(GET ASIO_INCLUDE_DIR PARENT_PATH ASIO_ROOT)
    asio_extract_version(ASIO_VERSION ${ASIO_INCLUDE_DIR}/asio/version.hpp)
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(asio
            REQUIRED_VARS ASIO_ROOT
            VERSION_VAR ASIO_VERSION)
endif()

if(asio_FOUND)
    if(NOT TARGET asio::asio)
        add_library(asio::asio INTERFACE IMPORTED)
        set_target_properties(asio::asio PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES ${ASIO_INCLUDE_DIR})
    endif()
endif()
