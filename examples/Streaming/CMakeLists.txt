add_executable(gRPC-Examples-Streaming)

shuhai_grpc_add_proto_targets(
        CODEGEN_TARGET gRPC-Examples-Streaming-ProtoGen
        LIBRARY_TARGET gRPC-Examples-Streaming
        PROTO_FILES "${CMAKE_CURRENT_LIST_DIR}/proto/Streaming.proto"
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/src/Generated")

file(GLOB_RECURSE SOURCES src/*.cpp src/*.h)
target_sources(gRPC-Examples-Streaming
        PRIVATE ${SOURCES})

target_link_libraries(gRPC-Examples-Streaming
        PRIVATE gRPC-Examples-Common gRPC)
