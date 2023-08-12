set(PROTO_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include)
set(OUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include)

file(GLOB_RECURSE LIB_PROTO_FILES ${PROTO_DIRECTORY}/FooLib/*.proto)
file(GLOB_RECURSE APP_PROTO_FILES ${PROTO_DIRECTORY}/FooApp/*.proto)

shuhai_grpc_add_proto_targets(
        CODEGEN_TARGET gRPC-Examples-ProtoTargets-FooLib-ProtoGen
        LIBRARY_TARGET gRPC-Examples-ProtoTargets-FooLib-Proto
        PROTO_FILES ${LIB_PROTO_FILES}
        OUTPUT_DIRECTORY ${OUT_DIRECTORY}
        proto_path ${PROTO_DIRECTORY}
        INCLUDE_INSTALL_INTERFACE include)

shuhai_grpc_add_proto_targets(
        CODEGEN_TARGET gRPC-Examples-ProtoTargets-FooApp-ProtoGen
        LIBRARY_TARGET gRPC-Examples-ProtoTargets-FooApp-Proto
        PROTO_FILES ${APP_PROTO_FILES}
        OUTPUT_DIRECTORY ${OUT_DIRECTORY}
        proto_path ${PROTO_DIRECTORY}
        INCLUDE_INSTALL_INTERFACE include)
add_dependencies(gRPC-Examples-ProtoTargets-FooApp-ProtoGen gRPC-Examples-ProtoTargets-FooLib-ProtoGen)
add_dependencies(gRPC-Examples-ProtoTargets-FooApp-Proto gRPC-Examples-ProtoTargets-FooLib-Proto)
