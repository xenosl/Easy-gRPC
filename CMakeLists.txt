cmake_minimum_required(VERSION 3.20)

project(EasyGRPC VERSION 0.3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

include(${PROJECT_SOURCE_DIR}/cmake/gRPC.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/Utilities.cmake)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules)

add_subdirectory(EasyGRPC)

option(SHUHAI_GRPC_BUILD_EXAMPLES "Whether to build examples." ON)
if(${SHUHAI_GRPC_BUILD_EXAMPLES})
    add_subdirectory(examples)
endif()

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()
