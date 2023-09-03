add_executable(gRPC-Examples-Streaming)

set(PROTO_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/src/proto)
shuhai_grpc_add_proto_targets(
        CODEGEN_TARGET gRPC-Examples-Streaming-ProtoGen
        LIBRARY_TARGET gRPC-Examples-Streaming
        PROTO_FILES "${CMAKE_CURRENT_LIST_DIR}/proto/Streaming.proto"
        OUTPUT_DIRECTORY ${PROTO_SOURCE_DIR})

file(GLOB_RECURSE SOURCES src/*.cpp src/*.h)
file(GLOB_RECURSE PROTO_SOURCES ${PROTO_SOURCE_DIR}/*.cc ${PROTO_SOURCE_DIR}/*.h)
target_sources(gRPC-Examples-Streaming
        PRIVATE ${SOURCES} ${PROTO_SOURCES})

target_include_directories(gRPC-Examples-Streaming
        PRIVATE ${PROTO_SOURCE_DIR})

target_link_libraries(gRPC-Examples-Streaming
        PRIVATE gRPC-Examples-Common gRPC)
