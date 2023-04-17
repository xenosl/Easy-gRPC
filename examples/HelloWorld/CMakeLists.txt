add_executable(gRPC-Examples-HelloWorld)

shuhai_grpc_add_proto_targets(
        GENERATOR_TARGET gRPC-Examples-HelloWorld-Proto-Gen
        LIBRARY_TARGET gRPC-Examples-HelloWorld-Proto
        PROTO_FILES "${CMAKE_CURRENT_LIST_DIR}/proto/helloworld.proto"
        OUTPUT_DIR "${CMAKE_CURRENT_LIST_DIR}/src/Generated")

file(GLOB_RECURSE SOURCES src/*.cpp src/*.h)
target_sources(gRPC-Examples-HelloWorld
        PRIVATE ${SOURCES})

target_link_libraries(gRPC-Examples-HelloWorld
        PRIVATE gRPC-Examples-Common gRPC-Examples-HelloWorld-Proto gRPC)

add_dependencies(gRPC-Examples-HelloWorld gRPC-Examples-HelloWorld-Proto)
