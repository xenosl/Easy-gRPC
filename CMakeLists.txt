cmake_minimum_required(VERSION 3.16)

project(gRPC-Quick)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

include(${PROJECT_SOURCE_DIR}/cmake/gRPC.cmake)

add_subdirectory(gRPC)

option(SHUHAI_BUILD_DEMO "Whether to build examples." ON)
if(${SHUHAI_BUILD_DEMO})
    add_subdirectory(examples)
endif()
