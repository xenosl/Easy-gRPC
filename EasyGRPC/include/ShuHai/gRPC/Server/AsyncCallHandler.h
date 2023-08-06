#pragma once

#include "ShuHai/gRPC/Server/TypeTraits.h"

#include <grpcpp/completion_queue.h>
#include <google/protobuf/message.h>

namespace ShuHai::gRPC::Server
{
    class AsyncCallHandlerBase
    {
    public:
        explicit AsyncCallHandlerBase(grpc::ServerCompletionQueue* completionQueue)
            : _completionQueue(completionQueue)
        { }

        virtual ~AsyncCallHandlerBase() = default;

        virtual void shutdown() { }

    protected:
        grpc::ServerCompletionQueue* const _completionQueue;
    };

    template<typename RequestFuncType>
    class AsyncCallHandler : public AsyncCallHandlerBase
    {
    public:
        SHUHAI_GRPC_SERVER_EXPAND_AsyncRequestTraits(RequestFuncType);

    protected:
        AsyncCallHandler(grpc::ServerCompletionQueue* completionQueue, Service* service, RequestFunc requestFunc)
            : AsyncCallHandlerBase(completionQueue)
            , _service(service)
            , _requestFunc(requestFunc)
        { }

        Service* const _service;
        const RequestFunc _requestFunc;
    };
}
