add_executable(ShuHai-gRPC-Examples-SimpleCall)

shuhai_grpc_add_proto_targets(
        GENERATOR_TARGET ShuHai-gRPC-Examples-SimpleCall-Proto-Gen
        LIBRARY_TARGET ShuHai-gRPC-Examples-SimpleCall-Proto
        PROTO_FILES "${CMAKE_CURRENT_LIST_DIR}/proto/App.proto;${CMAKE_CURRENT_LIST_DIR}/proto/Task.proto"
        OUTPUT_DIR "${CMAKE_CURRENT_LIST_DIR}/src/Generated")

file(GLOB_RECURSE SOURCES src/*.cpp src/*.h)
target_sources(ShuHai-gRPC-Examples-SimpleCall
        PRIVATE ${SOURCES})

target_link_libraries(ShuHai-gRPC-Examples-SimpleCall
        PRIVATE ShuHai-gRPC-Examples-Common ShuHai-gRPC-Examples-SimpleCall-Proto ShuHai::gRPC)

add_dependencies(ShuHai-gRPC-Examples-SimpleCall ShuHai-gRPC-Examples-SimpleCall-Proto)
