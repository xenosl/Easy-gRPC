add_executable(gRPC-Examples-Streaming)

shuhai_grpc_add_proto_targets(
        GENERATOR_TARGET gRPC-Examples-Streaming-Proto-Gen
        LIBRARY_TARGET gRPC-Examples-Streaming-Proto
        PROTO_FILES "${CMAKE_CURRENT_LIST_DIR}/proto/Streaming.proto"
        OUTPUT_DIR "${CMAKE_CURRENT_LIST_DIR}/src/Generated")

file(GLOB_RECURSE SOURCES src/*.cpp src/*.h)
target_sources(gRPC-Examples-Streaming
        PRIVATE ${SOURCES})

target_link_libraries(gRPC-Examples-Streaming
        PRIVATE gRPC-Examples-Common gRPC-Examples-Streaming-Proto gRPC)

add_dependencies(gRPC-Examples-Streaming gRPC-Examples-Streaming-Proto)
