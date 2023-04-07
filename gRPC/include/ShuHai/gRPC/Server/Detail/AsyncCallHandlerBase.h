#pragma once

#include <grpcpp/grpcpp.h>


namespace ShuHai::gRPC::Server::Detail
{
    class AsyncCallHandlerBase
    {
    public:
        virtual ~AsyncCallHandlerBase() = default;
        virtual void proceed(grpc::ServerCompletionQueue* queue) = 0;
        virtual void exit() = 0;
    };
}
