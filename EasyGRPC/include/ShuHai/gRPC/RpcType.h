#pragma once

namespace ShuHai::gRPC
{
    enum class RpcType
    {
        UnaryCall,
        ClientStream,
        ServerStream,
        BidiStream
    };
}
