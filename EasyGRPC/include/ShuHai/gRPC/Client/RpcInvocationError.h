#pragma once

#include "ShuHai/gRPC/RpcType.h"

#include <stdexcept>

namespace ShuHai::gRPC::Client
{
    class RpcInvocationError : public std::runtime_error
    {
    public:
        explicit RpcInvocationError(RpcType rpcType)
            : std::runtime_error("The RPC is not going to the wire, the channel is either permanently broken or "
                                 "transiently broken but with the fail-fast option")
            , _rpcType(rpcType)
        { }

        [[nodiscard]] RpcType rpcType() const { return _rpcType; }

    private:
        RpcType _rpcType;
    };
}
