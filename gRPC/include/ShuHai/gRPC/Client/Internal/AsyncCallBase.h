#pragma once

#include <grpcpp/client_context.h>

namespace ShuHai::gRPC::Client::Internal
{
    class AsyncCallBase
    {
    public:
        virtual ~AsyncCallBase() = default;

        grpc::ClientContext context;

    protected:
        grpc::Status _status;
    };
}
