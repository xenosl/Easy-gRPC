#pragma once

#include <grpcpp/impl/rpc_method.h>

namespace ShuHai::gRPC
{
    using RpcType = grpc::internal::RpcMethod::RpcType;

    //enum class RpcType
    //
    //   SyncUnaryCall,
    //   SyncClientStream,
    //   SyncServerStream,
    //   SyncBidirectionalStream,
    //   AsyncUnaryCall,
    //   AsyncClientStream,
    //   AsyncServerStream,
    //   AsyncBidirectionalStream,
    //;
}
