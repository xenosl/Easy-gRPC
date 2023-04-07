add_library(ShuHai-gRPC INTERFACE)
add_library(ShuHai::gRPC ALIAS ShuHai-gRPC)

target_include_directories(ShuHai-gRPC
        INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include> $<INSTALL_INTERFACE:include>)

target_link_libraries(ShuHai-gRPC
        INTERFACE gRPC::grpc++)
