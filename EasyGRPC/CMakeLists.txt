add_library(gRPC INTERFACE)

target_include_directories(gRPC
        INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include> $<INSTALL_INTERFACE:include>)

find_package(asio REQUIRED)
target_link_libraries(gRPC
        INTERFACE gRPC::grpc++ asio::asio)


shuhai_read_version_from_macro(VERSION
        FILE ${CMAKE_CURRENT_LIST_DIR}/include/ShuHai/gRPC/Version.h
        VERSION SHUHAI_GRPC_VERSION)
if(NOT VERSION)
    message(FATAL_ERROR "Version definition not found.")
endif()
set(CMAKE_PROJECT_VERSION ${VERSION})


set(gRPC_INSTALL_DESTINATION cmake)
set(gRPC_PACKAGE_CONFIG_FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake)
set(gRPC_PACKAGE_CONFIG_VERSION_FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}VersionConfig.cmake)
set(EasyGRPC_PACKAGE_FIND_DEPENDENCY_MODE "CONFIG" CACHE STRING "The search mode of 'find_dependency' for installed package of Easy-gRPC.")

install(TARGETS gRPC
        EXPORT ${PROJECT_NAME}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ShuHai
        TYPE INCLUDE)

install(EXPORT ${PROJECT_NAME}
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE ShuHai::
        DESTINATION ${gRPC_INSTALL_DESTINATION})

configure_package_config_file(${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
        ${gRPC_PACKAGE_CONFIG_FILE}
        INSTALL_DESTINATION ${gRPC_INSTALL_DESTINATION})
write_basic_package_version_file("${gRPC_PACKAGE_CONFIG_VERSION_FILE}"
        VERSION "${VERSION}"
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT)
install(FILES "${gRPC_PACKAGE_CONFIG_FILE}" "${gRPC_PACKAGE_CONFIG_VERSION_FILE}"
        DESTINATION ${gRPC_INSTALL_DESTINATION})

install(FILES "${PROJECT_SOURCE_DIR}/cmake/TargetUtilities.cmake" "${PROJECT_SOURCE_DIR}/cmake/gRPC.cmake"
        DESTINATION ${gRPC_INSTALL_DESTINATION})
