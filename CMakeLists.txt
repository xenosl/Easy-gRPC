cmake_minimum_required(VERSION 3.16)

project(gRPC-Quick)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(${PROJECT_SOURCE_DIR}/cmake/gRPC.cmake)


add_library(ShuHai-gRPC INTERFACE)
add_library(ShuHai::gRPC ALIAS ShuHai-gRPC)

target_include_directories(ShuHai-gRPC
        INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include> $<INSTALL_INTERFACE:include>)

target_link_libraries(ShuHai-gRPC
        INTERFACE gRPC::grpc++)


option(SHUHAI_BUILD_DEMO "Whether to build examples." ON)
if(${SHUHAI_BUILD_DEMO})
    add_subdirectory(examples)
endif()
