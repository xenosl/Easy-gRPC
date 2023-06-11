add_executable(gRPC-Examples-SimpleCall)

shuhai_grpc_add_proto_targets(
        GENERATOR_TARGET gRPC-Examples-SimpleCall-ProtoGen
        LIBRARY_TARGET gRPC-Examples-SimpleCall
        PROTO_FILES "${CMAKE_CURRENT_LIST_DIR}/proto/App.proto;${CMAKE_CURRENT_LIST_DIR}/proto/Task.proto"
        OUTPUT_DIR "${CMAKE_CURRENT_LIST_DIR}/src/Generated")

file(GLOB_RECURSE SOURCES src/*.cpp src/*.h)
target_sources(gRPC-Examples-SimpleCall
        PRIVATE ${SOURCES})

target_link_libraries(gRPC-Examples-SimpleCall
        PRIVATE gRPC-Examples-Common gRPC)
